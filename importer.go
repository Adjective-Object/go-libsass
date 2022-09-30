package libsass

import (
	"errors"
	"io"
	"sync"
	"time"

	"github.com/wellington/go-libsass/libs"
)

var (
	ErrImportNotFound = errors.New("Import unreachable or not found")
)

// Import contains Rel and Abs path and a string of the contents
// representing an import.
type Import struct {
	Body  io.ReadCloser
	bytes []byte
	mod   time.Time
	Prev  string
	Path  string
}

// ModTime returns modification time
func (i Import) ModTime() time.Time {
	return i.mod
}

type importOverrides struct {
	sync.RWMutex
	m map[string]Import
}

// Init sets up a new Imports map
func (p *importOverrides) Init() {
	p.m = make(map[string]Import)
}

// Add registers an import in the context.Imports
func (p *importOverrides) Add(prev string, path string, bs []byte) error {
	p.Lock()
	defer p.Unlock()

	// TODO: align these with libsass name "stdin"
	if len(prev) == 0 || prev == "string" {
		prev = "stdin"
	}
	im := Import{
		bytes: bs,
		mod:   time.Now(),
		Prev:  prev,
		Path:  path,
	}

	p.m[prev+":"+path] = im
	return nil
}

// Del removes the import from the context.Imports
func (p *importOverrides) Del(path string) {
	p.RLock()
	defer p.RUnlock()

	delete(p.m, path)
}

// Get retrieves import bytes by path
func (p *importOverrides) Get(prev, path string) ([]byte, error) {
	p.RLock()
	defer p.RUnlock()
	for _, imp := range p.m {
		if imp.Prev == prev && imp.Path == path {
			return imp.bytes, nil
		}
	}
	return nil, ErrImportNotFound
}

// Wraps a resolver against this ImportOverrides' map of imports
func (p *importOverrides) wrapResolver(
	fn libs.AdvancedImportResolver,
) libs.AdvancedImportResolver {
	return func(url string, prev string) libs.ResolverResult {
		// failed to resolve
		if fn != nil {
			result := fn(url, prev)
			if result.Resolved {
				return result
			}
		}

		// fallback: check the imports in the override map
		entries := p.EntriesList()
		entry, err := libs.GetEntry(entries, prev, url)

		if err == nil {
			return libs.ResolverResult{
				NewUrl:   url,
				Source:   entry,
				Resolved: true,
			}
		}
		return libs.ResolverResult{}
	}
}

// Len counts the number of entries in the import overrides map
func (p *importOverrides) Len() int {
	return len(p.m)
}

// Gets the map of imports as a slice.
// the slice will not have consistent ordering.
func (p *importOverrides) EntriesList() []libs.ImportEntry {
	p.RLock()
	defer p.RUnlock()

	i := 0
	entries := make([]libs.ImportEntry, len(p.m))
	for _, ent := range p.m {
		bs := ent.bytes
		entries[i] = libs.ImportEntry{
			Parent: ent.Prev,
			Path:   ent.Path,
			Source: string(bs),
		}
		i++
	}

	return entries
}

// Imports is a map with key of "path/to/file"
type Imports struct {
	importOverrides
	wg      sync.WaitGroup
	closing chan struct{}
	sync.RWMutex
	resolverOptions libs.ResolverOptions
	// Handle to the C-side idx and pointer associated
	// with this object
	resolver       libs.AdvancedImportResolver
	resolverCookie *libs.ResolverCookie
}

func NewImports() *Imports {
	return NewImportsWithResolverOption(
		libs.ResolverOptions{},
		nil,
	)
}

func wrapLegacyResolver(legacyResolver libs.ImportResolver) libs.AdvancedImportResolver {
	return func(url string, prev string) libs.ResolverResult {
		newUrl, body, resolved := legacyResolver(url, prev)
		return libs.ResolverResult{
			NewUrl:      newUrl,
			Source:      body,
			Resolved:    resolved,
			ShouldCache: false,
		}
	}
}

// Deprecated, prefer NewImportsWithResolverOption
func NewImportsWithResolver(resolver libs.ImportResolver) *Imports {
	return NewImportsWithResolverOption(
		libs.ResolverOptions{
			ResolverMode: libs.ResolverModeImporterUrl,
		},
		wrapLegacyResolver(resolver),
	)
}

// Deprecated, prefer NewImportsWithResolverOption
func NewImportsWithAbsResolver(resolver libs.ImportResolver) *Imports {
	return NewImportsWithResolverOption(
		libs.ResolverOptions{
			ResolverMode: libs.ResolverModeImporterAbsPath,
		},
		wrapLegacyResolver(resolver),
	)
}

func NewImportsWithResolverOption(options libs.ResolverOptions, resolver libs.AdvancedImportResolver) *Imports {
	return &Imports{
		importOverrides: importOverrides{},
		closing:         make(chan struct{}),
		resolverOptions: options,
		resolver:        resolver,
	}
}

func (i *Imports) Close() {
	close(i.closing)
	i.wg.Wait()
}

// Bind accepts a SassOptions and adds the registered
// importers in the context.
func (p *Imports) Bind(opts libs.SassOptions) {
	// if the cookie hasn't been created in C-land yet,
	// create it and save it on this Imports object for
	// reuse in future compilation
	if p.resolverCookie == nil {
		p.resolverCookie = libs.CreateImportsResolverCookie(
			p.resolverOptions,
			p.importOverrides.wrapResolver(p.resolver),
		)
	}

	libs.BindImporter(
		opts,
		p.resolverCookie,
		p.resolverOptions,
	)
}

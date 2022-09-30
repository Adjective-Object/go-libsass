package libsass

import (
	"errors"
	"fmt"
	"io"
	"sync"
	"time"

	"github.com/wellington/go-libsass/libs"
)

// #include "resolutioncache.h"
import "C"

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

// Imports is a map with key of "path/to/file"
type Imports struct {
	wg      sync.WaitGroup
	closing chan struct{}
	sync.RWMutex
	m        map[string]Import
	resolver ResolverCallback
	idx      int
	// Handle to the imports cache associated with
	// this imports object
	importsCachePtr *(C.GoLibsass_ResolverCache)
}

type ResolverCallback struct {
	resolverOptions libs.ResolverOptions
	resolverFn      libs.AdvancedImportResolver
}

func NewImports() *Imports {
	return &Imports{
		closing: make(chan struct{}),
	}
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
	return &Imports{
		closing: make(chan struct{}),
		resolver: ResolverCallback{
			resolverOptions: libs.ResolverOptions{
				ResolverMode: libs.ResolverModeImporterUrl,
			},
			resolverFn: wrapLegacyResolver(resolver),
		},
	}
}

// Deprecated, prefer NewImportsWithResolverOption
func NewImportsWithAbsResolver(resolver libs.ImportResolver) *Imports {
	return &Imports{
		closing: make(chan struct{}),
		resolver: ResolverCallback{
			resolverOptions: libs.ResolverOptions{
				ResolverMode: libs.ResolverModeImporterAbsPath,
			},
			resolverFn: wrapLegacyResolver(resolver),
		},
	}
}

func NewImportsWithResolverOption(options libs.ResolverOptions, resolver libs.AdvancedImportResolver) *Imports {
	return &Imports{
		closing: make(chan struct{}),
		resolver: ResolverCallback{
			resolverOptions: options,
			resolverFn:      resolver,
		},
	}
}

func (i *Imports) Close() {
	close(i.closing)
	i.wg.Wait()
}

// Init sets up a new Imports map
func (p *Imports) Init() {
	p.m = make(map[string]Import)
}

// Add registers an import in the context.Imports
func (p *Imports) Add(prev string, path string, bs []byte) error {
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
func (p *Imports) Del(path string) {
	p.Lock()
	defer p.Unlock()

	delete(p.m, path)
}

// Get retrieves import bytes by path
func (p *Imports) Get(prev, path string) ([]byte, error) {
	p.RLock()
	defer p.RUnlock()
	for _, imp := range p.m {
		if imp.Prev == prev && imp.Path == path {
			return imp.bytes, nil
		}
	}
	return nil, ErrImportNotFound
}

// Update attempts to create a fresh Body from the given path
// Files last modified stamps are compared against import timestamp
func (p *Imports) Update(name string) {
	p.Lock()
	defer p.Unlock()

}

// Len counts the number of entries in context.Imports
func (p *Imports) Len() int {
	return len(p.m)
}

// Bind accepts a SassOptions and adds the registered
// importers in the context.
func (p *Imports) Bind(opts libs.SassOptions) {
	entries := make([]libs.ImportEntry, p.Len())
	i := 0

	p.RLock()
	for _, ent := range p.m {
		bs := ent.bytes
		entries[i] = libs.ImportEntry{
			Parent: ent.Prev,
			Path:   ent.Path,
			Source: string(bs),
		}
		i++
	}
	p.RUnlock()

	wrappedResolverFn := func(url string, prev string) libs.ResolverResult {
		// failed to resolve
		fmt.Println("!!!ressolver!!!!!!")
		if p.resolver.resolverFn != nil {
			result := p.resolver.resolverFn(url, prev)
			if result.Resolved {
				return result
			}
		}
		// fallback: check the other imports
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

	// set entries somewhere so GC doesn't collect it
	p.idx = libs.BindImporter(opts, p.resolver.resolverOptions, wrappedResolverFn)
}

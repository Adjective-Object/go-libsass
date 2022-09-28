#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sass/context.h"
#include "./resolutioncache.h"
#include "importerhandler.h"

typedef struct {
    Sass_Import_List r0;
    uint8_t r1;
} ImporterBridge_return;

extern ImporterBridge_return ImporterBridge(
    const char* url,
    const char* prev,
    uintptr_t idx
);

Sass_Import_List sassCommonHandler(
  const char* cur_path,
  Sass_Importer_Entry cb,
  struct Sass_Compiler* comp,
  bool useAbsPath
) {
  GoLibsass_InternalCookie* cookie = (GoLibsass_InternalCookie*) sass_importer_get_cookie(cb);
  Sass_Import_Entry cached = golibsass_resolution_cache_get(cookie->cache, cur_path);
  if (cached != NULL) {
    // return the single entry from cache in a new list
    Sass_Import_List list = sass_make_import_list(1);
    sass_import_set_list_entry(list, 0, cached);
    return list;
  }

  // No entry in the cache, fetch an entry from go
  struct Sass_Import* previous = sass_compiler_get_last_import(comp);
  const char* prev_path = useAbsPath
    ? sass_import_get_imp_path(previous)
    : sass_import_get_abs_path(previous);
  ImporterBridge_return bridgeResult = ImporterBridge(cur_path, prev_path, cookie->idx);
  Sass_Import_List list = bridgeResult.r0;
  bool shouldCache = bridgeResult.r1 != 0;

  if (shouldCache) {
    // ImporterBridge will always return a single-entry list so this index
    // and access not checked behind a NULL guard should be safe.
    Sass_Import_Entry new_import = sass_import_get_list_entry(list, 0);
    if (new_import != NULL) {
      // If we resolved some import, save it in the cache
      golibsass_resolution_cache_insert(cookie->cache, new_import);
    }
  }

  return list;
}

Sass_Import_List SassImporterPathsHandler(const char* cur_path, Sass_Importer_Entry cb, struct Sass_Compiler* comp) {
  return sassCommonHandler(
      cur_path,
      cb,
      comp,
      false // useAbsPath
  );
}

Sass_Import_List SassImporterAbsHandler(const char* cur_path, Sass_Importer_Entry cb, struct Sass_Compiler* comp) {
  return sassCommonHandler(
      cur_path,
      cb,
      comp,
      true // useAbsPath
  );
}


#ifndef UINTMAX_MAX
#  ifdef __UINTMAX_MAX__
#    define UINTMAX_MAX __UINTMAX_MAX__
#  endif
#endif

size_t max_size = UINTMAX_MAX;
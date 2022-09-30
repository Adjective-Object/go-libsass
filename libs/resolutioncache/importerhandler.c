#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sass/context.h"
#include "resolvercookie.h"
#include "resolutioncache.h"
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
  bool use_abs_path
) {
  printf("sassCommonHandler path=%s, useAbsPath=%i\n", cur_path, use_abs_path);
  GoLibsass_ResolverCookie* cookie = (GoLibsass_ResolverCookie*) sass_importer_get_cookie(cb);
  printf("cookie!=%p\n", cookie);
  bool hasCache = golibsass_resolvercookie_hascache(cookie);
  printf("hasCache?=%d\n", hasCache);
  if (hasCache) {
    Sass_Import_Entry cached = golibsass_resolution_cache_get(cookie->cache, cur_path);
    printf("cached=%p\n", cached);
    if (cached != NULL) {
    printf("found entry from cache\n");
      // return the single entry from cache in a new list
      Sass_Import_List list = sass_make_import_list(1);
      sass_import_set_list_entry(list, 0, cached);
      return list;
    }
  }

  printf("no entry in cache, going to resolver\n");
  // No entry in the cache, fetch an entry from go
  struct Sass_Import* previous = sass_compiler_get_last_import(comp);
  const char* prev_path = use_abs_path
    ? sass_import_get_abs_path(previous)
    : sass_import_get_imp_path(previous);
  printf("previous.. abs_path=%s imp_path==%s\n", sass_import_get_abs_path(previous), sass_import_get_imp_path(previous));
  ImporterBridge_return bridgeResult = ImporterBridge(cur_path, prev_path, cookie->idx);
  Sass_Import_List list = bridgeResult.r0;
  printf("bridgeResult!! list=%p, shouldCache=%d .. hasCache=%d\n", bridgeResult.r0, bridgeResult.r1, hasCache);

  if (hasCache & (bridgeResult.r1 != 0) ) {
    // ImporterBridge will always return a single-entry list so this index
    // and access not checked behind a NULL guard should be safe.
    Sass_Import_Entry new_import = sass_import_get_list_entry(list, 0);
    printf("new_import=%p cookie.entries=%p\n", new_import, cookie->cache.entries);
    if (new_import != NULL) {
      // If we resolved some import, save it in the cache
      printf("calling golibsass_resolution_cache_insert...\n");
      golibsass_resolution_cache_insert(cookie->cache, new_import);
    }
  }

  printf("resolving to: %p\n", list);

  return list;
}

Sass_Import_List SassImporterPathsHandler(const char* cur_path, Sass_Importer_Entry cb, struct Sass_Compiler* comp) {
  return sassCommonHandler(
      cur_path,
      cb,
      comp,
      false // use_abs_path
  );
}

Sass_Import_List SassImporterAbsHandler(const char* cur_path, Sass_Importer_Entry cb, struct Sass_Compiler* comp) {
  return sassCommonHandler(
      cur_path,
      cb,
      comp,
      true // use_abs_path
  );
}


#ifndef UINTMAX_MAX
#  ifdef __UINTMAX_MAX__
#    define UINTMAX_MAX __UINTMAX_MAX__
#  endif
#endif

size_t max_size = UINTMAX_MAX;
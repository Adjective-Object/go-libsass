#ifndef GO_LIBSASS_IMPORTER_HANDLER_H
#define GO_LIBSASS_IMPORTER_HANDLER_H
#include <stdint.h>
#include <stdlib.h>
#include "sass/functions.h"


Sass_Import_List SassImporterPathsHandler(const char* cur_path, Sass_Importer_Entry cb, struct Sass_Compiler* comp);
Sass_Import_List SassImporterAbsHandler(const char* cur_path, Sass_Importer_Entry cb, struct Sass_Compiler* comp);

typedef struct {
    bool shouldCache;
    Sass_Import_List list;
} BridgeResult;

#endif
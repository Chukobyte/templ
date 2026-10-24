#pragma once

#include <stdbool.h>
#include "seika/defines.h"

#define TPL_PATH_MAX 512

// Path utilities — all paths use '/' internally.

void tpl_path_join(char* out, usize out_size, const char* base, const char* rel);
const char* tpl_path_ext(const char* path);      // returns pointer into path, e.g. ".html", or ""
void tpl_path_strip_ext(char* out, usize out_size, const char* path);
void tpl_path_basename(char* out, usize out_size, const char* path);
void tpl_path_dirname(char* out, usize out_size, const char* path);
bool tpl_mkdir_p(const char* path);
bool tpl_rmdir_recursive(const char* path);

typedef struct TplFileEntry {
    char path[TPL_PATH_MAX];
    bool is_dir;
} TplFileEntry;

typedef struct TplFileList {
    TplFileEntry* entries;
    usize count;
    usize cap;
} TplFileList;

TplFileList tpl_file_list_create(void);
void tpl_file_list_append(TplFileList* list, const char* path, bool is_dir);
void tpl_file_list_free(TplFileList* list);

// Recursively lists all entries under dir, appending to out (non-recursive dirs included too).
void tpl_dir_list_recursive(const char* dir, TplFileList* out);

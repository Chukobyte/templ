#include "platform.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "seika/defines.h"
#include "seika/file_system.h"

#ifdef PLATFORM_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <direct.h>
#else
#  include <sys/stat.h>
#  include <dirent.h>
#  include <unistd.h>
#endif

void tpl_path_join(char* out, usize out_size, const char* base, const char* rel) {
    usize blen = strlen(base);
    bool base_has_slash = blen > 0 && (base[blen - 1] == '/' || base[blen - 1] == '\\');
    bool rel_has_slash = rel[0] == '/' || rel[0] == '\\';

    if (base_has_slash || rel_has_slash) {
        snprintf(out, out_size, "%s%s", base, rel);
    } else {
        snprintf(out, out_size, "%s/%s", base, rel);
    }
    for (char* p = out; *p; p++) {
        if (*p == '\\') *p = '/';
    }
}

const char* tpl_path_ext(const char* path) {
    const char* dot = strrchr(path, '.');
    const char* slash = strrchr(path, '/');
    if (!slash) slash = strrchr(path, '\\');
    if (dot && (!slash || dot > slash)) return dot;
    return "";
}

void tpl_path_strip_ext(char* out, usize out_size, const char* path) {
    const char* ext = tpl_path_ext(path);
    if (ext && ext[0]) {
        usize base_len = (usize)(ext - path);
        if (base_len >= out_size) base_len = out_size - 1;
        memcpy(out, path, base_len);
        out[base_len] = '\0';
    } else {
        snprintf(out, out_size, "%s", path);
    }
}

void tpl_path_basename(char* out, usize out_size, const char* path) {
    const char* p = strrchr(path, '/');
    if (!p) p = strrchr(path, '\\');
    snprintf(out, out_size, "%s", p ? p + 1 : path);
}

void tpl_path_dirname(char* out, usize out_size, const char* path) {
    const char* p = strrchr(path, '/');
    if (!p) p = strrchr(path, '\\');
    if (!p) { snprintf(out, out_size, "."); return; }
    usize len = (usize)(p - path);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, path, len);
    out[len] = '\0';
}

bool tpl_mkdir_p(const char* path) {
    char tmp[TPL_PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char* p = tmp; *p; p++) {
        if (*p == '\\') *p = '/';
    }
    usize len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/') tmp[--len] = '\0';

    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
#ifdef PLATFORM_WINDOWS
            _mkdir(tmp);
#else
            mkdir(tmp, 0755);
#endif
            *p = '/';
        }
    }
#ifdef PLATFORM_WINDOWS
    int r = _mkdir(tmp);
#else
    int r = mkdir(tmp, 0755);
#endif
    return r == 0 || errno == EEXIST;
}

bool tpl_rmdir_recursive(const char* path) {
#ifdef PLATFORM_WINDOWS
    char pattern[TPL_PATH_MAX];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    for (char* p = pattern; *p; p++) { if (*p == '/') *p = '\\'; }

    WIN32_FIND_DATAA ffd;
    HANDLE h = FindFirstFileA(pattern, &ffd);
    if (h == INVALID_HANDLE_VALUE) return true;
    do {
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) continue;
        char child[TPL_PATH_MAX];
        snprintf(child, sizeof(child), "%s/%s", path, ffd.cFileName);
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            tpl_rmdir_recursive(child);
        } else {
            char win_path[TPL_PATH_MAX];
            snprintf(win_path, sizeof(win_path), "%s", child);
            for (char* p = win_path; *p; p++) { if (*p == '/') *p = '\\'; }
            DeleteFileA(win_path);
        }
    } while (FindNextFileA(h, &ffd));
    FindClose(h);

    char win_path[TPL_PATH_MAX];
    snprintf(win_path, sizeof(win_path), "%s", path);
    for (char* p = win_path; *p; p++) { if (*p == '/') *p = '\\'; }
    return RemoveDirectoryA(win_path) != 0;
#else
    // Simple approach: build a shell command if available, else manual
    TplFileList list = tpl_file_list_create();
    tpl_dir_list_recursive(path, &list);
    // Remove files first (deepest entries last in the list are deeper dirs)
    for (usize i = list.count; i-- > 0;) {
        if (!list.entries[i].is_dir) {
            remove(list.entries[i].path);
        }
    }
    for (usize i = list.count; i-- > 0;) {
        if (list.entries[i].is_dir) {
            rmdir(list.entries[i].path);
        }
    }
    tpl_file_list_free(&list);
    return rmdir(path) == 0;
#endif
}

TplFileList tpl_file_list_create(void) {
    TplFileList list;
    list.count = 0;
    list.cap = 32;
    list.entries = malloc(list.cap * sizeof(TplFileEntry));
    return list;
}

void tpl_file_list_append(TplFileList* list, const char* path, bool is_dir) {
    if (list->count >= list->cap) {
        list->cap *= 2;
        list->entries = realloc(list->entries, list->cap * sizeof(TplFileEntry));
    }
    TplFileEntry* e = &list->entries[list->count++];
    snprintf(e->path, sizeof(e->path), "%s", path);
    e->is_dir = is_dir;
}

void tpl_file_list_free(TplFileList* list) {
    free(list->entries);
    list->entries = NULL;
    list->count = 0;
    list->cap = 0;
}

void tpl_dir_list_recursive(const char* dir, TplFileList* out) {
#ifdef PLATFORM_WINDOWS
    char pattern[TPL_PATH_MAX];
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);
    for (char* p = pattern; *p; p++) { if (*p == '/') *p = '\\'; }

    WIN32_FIND_DATAA ffd;
    HANDLE h = FindFirstFileA(pattern, &ffd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) continue;
        char child[TPL_PATH_MAX];
        snprintf(child, sizeof(child), "%s/%s", dir, ffd.cFileName);
        for (char* p = child; *p; p++) { if (*p == '\\') *p = '/'; }
        bool is_dir = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        tpl_file_list_append(out, child, is_dir);
        if (is_dir) tpl_dir_list_recursive(child, out);
    } while (FindNextFileA(h, &ffd));
    FindClose(h);
#else
    DIR* d = opendir(dir);
    if (!d) return;
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char child[TPL_PATH_MAX];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);
        bool is_dir = (ent->d_type == DT_DIR);
        tpl_file_list_append(out, child, is_dir);
        if (is_dir) tpl_dir_list_recursive(child, out);
    }
    closedir(d);
#endif
}

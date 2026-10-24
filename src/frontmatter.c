#include "frontmatter.h"

#include <string.h>
#include <ctype.h>

static void trim_end(char* s) {
    usize len = strlen(s);
    while (len > 0 && (s[len - 1] == '\r' || s[len - 1] == '\n' || s[len - 1] == ' ' || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }
}

bool tpl_frontmatter_parse(const char* source, TplFrontmatter* out) {
    memset(out, 0, sizeof(*out));
    out->body_start = source;

    // Frontmatter must start with "---"
    if (strncmp(source, "---", 3) != 0) return true; // no frontmatter is fine
    const char* p = source + 3;
    // Skip optional whitespace / \r after opening ---
    while (*p == ' ' || *p == '\t' || *p == '\r') p++;
    if (*p != '\n') { return true; } // malformed — treat as no frontmatter
    p++;

    while (*p) {
        // Check for closing ---
        if (strncmp(p, "---", 3) == 0) {
            p += 3;
            while (*p == ' ' || *p == '\t' || *p == '\r') p++;
            if (*p == '\n') p++;
            out->body_start = p;
            return true;
        }

        // Parse "key: value" line (skip blank lines and comment lines)
        if (*p == '\n') { p++; continue; }
        if (*p == '#')  { while (*p && *p != '\n') p++; continue; }

        const char* line_start = p;
        while (*p && *p != '\n') p++;
        // line is [line_start, p)

        if (out->field_count >= TPL_FM_MAX_FIELDS) {
            if (*p == '\n') p++;
            continue;
        }

        TplFMField* f = &out->fields[out->field_count];
        const char* colon = NULL;
        for (const char* c = line_start; c < p; c++) {
            if (*c == ':') { colon = c; break; }
        }
        if (!colon) {
            if (*p == '\n') p++;
            continue;
        }

        // key
        usize key_len = (usize)(colon - line_start);
        if (key_len >= TPL_FM_KEY_MAX) key_len = TPL_FM_KEY_MAX - 1;
        memcpy(f->key, line_start, key_len);
        f->key[key_len] = '\0';
        trim_end(f->key);

        // value (skip leading space after colon)
        const char* val_start = colon + 1;
        while (*val_start == ' ' || *val_start == '\t') val_start++;
        usize val_len = (usize)(p - val_start);
        if (val_len >= TPL_FM_VAL_MAX) val_len = TPL_FM_VAL_MAX - 1;
        memcpy(f->value, val_start, val_len);
        f->value[val_len] = '\0';
        trim_end(f->value);

        // Strip surrounding quotes from value if present
        usize vlen = strlen(f->value);
        if (vlen >= 2 && f->value[0] == '"' && f->value[vlen - 1] == '"') {
            memmove(f->value, f->value + 1, vlen - 2);
            f->value[vlen - 2] = '\0';
        }

        if (f->key[0] != '\0') out->field_count++;
        if (*p == '\n') p++;
    }

    // Reached end of source without closing ---
    out->body_start = p;
    return true;
}

const char* tpl_fm_get(const TplFrontmatter* fm, const char* key) {
    for (usize i = 0; i < fm->field_count; i++) {
        if (strcmp(fm->fields[i].key, key) == 0) return fm->fields[i].value;
    }
    return NULL;
}

#include "theme.h"

#include <string.h>

#define TPL_THEME_REGISTRY_MAX 32

static const TplTheme* s_registry[TPL_THEME_REGISTRY_MAX];
static usize s_registry_count = 0;

void tpl_theme_register(const TplTheme* theme) {
    if (!theme || s_registry_count >= TPL_THEME_REGISTRY_MAX) return;
    s_registry[s_registry_count++] = theme;
}

const TplTheme* tpl_theme_find(const char* name) {
    if (!name || name[0] == '\0') return NULL;
    for (usize i = 0; i < s_registry_count; i++) {
        if (strcmp(s_registry[i]->name, name) == 0) return s_registry[i];
    }
    return NULL;
}

const char* tpl_theme_get_layout(const TplTheme* theme, const char* layout_name) {
    if (!theme || !layout_name) return NULL;
    for (usize i = 0; i < theme->layout_count; i++) {
        if (strcmp(theme->layouts[i].name, layout_name) == 0) return theme->layouts[i].html;
    }
    // Fall back to "base" if the specific layout isn't defined in this theme
    if (strcmp(layout_name, "base") != 0) {
        return tpl_theme_get_layout(theme, "base");
    }
    return NULL;
}

/* ---- bare theme (raw HTML fragment, no wrapper, no CSS) ------------------ */

static const char s_bare_base[] = "{{{body}}}";
static const TplThemeLayout s_bare_layouts[] = { { "base", s_bare_base } };
static const TplTheme s_theme_bare = {
    .name         = "bare",
    .description  = "Raw HTML fragment — no wrapper, no CSS",
    .layouts      = s_bare_layouts,
    .layout_count = 1,
    .css          = NULL,
};

// Declared in theme_default.c and theme_modern.c
extern const TplTheme tpl_builtin_theme_default;
extern const TplTheme tpl_builtin_theme_modern;

void tpl_theme_register_builtins(void) {
    tpl_theme_register(&s_theme_bare);
    tpl_theme_register(&tpl_builtin_theme_default);
    tpl_theme_register(&tpl_builtin_theme_modern);
}

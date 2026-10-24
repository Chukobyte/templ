#include <unity.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
#  define strdup _strdup
#endif

#include "template.h"
#include "seika/string.h"

void setUp(void) {}
void tearDown(void) {}

static TplRenderOpts no_fs_opts(void) {
    // Tests that don't use partials or extends can pass dummy dirs.
    TplRenderOpts opts = { .templates_dir = ".", .layouts_dir = ".", .depth = 0 };
    return opts;
}

// Render src with the given ctx, return malloc'd result string. Caller must free().
static char* render_with(const char* src, TplContext* ctx) {
    SkaString out;
    ska_string_init_empty(&out);
    TplRenderOpts opts = no_fs_opts();
    tpl_render(src, strlen(src), ctx, &opts, &out);
    // ska_string heap buffers are not null-terminated; copy with explicit length
    usize rlen = out.len;
    char* result = (char*)malloc(rlen + 1);
    memcpy(result, ska_string_as_slice(&out), rlen);
    result[rlen] = '\0';
    ska_string_deinit(&out);
    return result;
}

void test_ctx_set_get(void);
void test_variable_substitution(void);
void test_unescaped_variable(void);
void test_html_escaping(void);
void test_unknown_variable_is_empty(void);
void test_dot_path_variable(void);
void test_literal_text_passthrough(void);
void test_if_true(void);
void test_if_false(void);
void test_unless_true(void);
void test_unless_false(void);
void test_mixed_template(void);

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ctx_set_get);
    RUN_TEST(test_variable_substitution);
    RUN_TEST(test_unescaped_variable);
    RUN_TEST(test_html_escaping);
    RUN_TEST(test_unknown_variable_is_empty);
    RUN_TEST(test_dot_path_variable);
    RUN_TEST(test_literal_text_passthrough);
    RUN_TEST(test_if_true);
    RUN_TEST(test_if_false);
    RUN_TEST(test_unless_true);
    RUN_TEST(test_unless_false);
    RUN_TEST(test_mixed_template);
    return UNITY_END();
}

void test_ctx_set_get(void) {
    TplContext* ctx = tpl_ctx_create();
    TEST_ASSERT_NOT_NULL(ctx);

    tpl_ctx_set(ctx, "name", "Alice");
    tpl_ctx_set(ctx, "age",  "30");

    TEST_ASSERT_EQUAL_STRING("Alice", tpl_ctx_get(ctx, "name"));
    TEST_ASSERT_EQUAL_STRING("30",    tpl_ctx_get(ctx, "age"));
    TEST_ASSERT_NULL(tpl_ctx_get(ctx, "missing"));

    tpl_ctx_free(ctx);
}

void test_variable_substitution(void) {
    TplContext* ctx = tpl_ctx_create();
    tpl_ctx_set(ctx, "title", "Hello World");

    char* html = render_with("<h1>{{title}}</h1>", ctx);
    TEST_ASSERT_EQUAL_STRING("<h1>Hello World</h1>", html);

    free(html);
    tpl_ctx_free(ctx);
}

void test_unescaped_variable(void) {
    TplContext* ctx = tpl_ctx_create();
    tpl_ctx_set(ctx, "body", "<p>Some content</p>");

    char* html = render_with("{{{body}}}", ctx);
    TEST_ASSERT_EQUAL_STRING("<p>Some content</p>", html);

    free(html);
    tpl_ctx_free(ctx);
}

void test_html_escaping(void) {
    TplContext* ctx = tpl_ctx_create();
    tpl_ctx_set(ctx, "val", "<script>alert('xss')</script>");

    char* html = render_with("{{val}}", ctx);
    // Should be escaped, no raw <script>
    TEST_ASSERT_NULL(strstr(html, "<script>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "&lt;script&gt;"));

    free(html);
    tpl_ctx_free(ctx);
}

void test_unknown_variable_is_empty(void) {
    TplContext* ctx = tpl_ctx_create();

    char* html = render_with("before{{unknown}}after", ctx);
    TEST_ASSERT_EQUAL_STRING("beforeafter", html);

    free(html);
    tpl_ctx_free(ctx);
}

void test_dot_path_variable(void) {
    TplContext* ctx = tpl_ctx_create();
    tpl_ctx_set(ctx, "site.title", "My Blog");

    char* html = render_with("{{site.title}}", ctx);
    TEST_ASSERT_EQUAL_STRING("My Blog", html);

    free(html);
    tpl_ctx_free(ctx);
}

void test_literal_text_passthrough(void) {
    TplContext* ctx = tpl_ctx_create();

    const char* tmpl = "<!DOCTYPE html>\n<html>\n<body>\n</body>\n</html>\n";
    char* html = render_with(tmpl, ctx);
    TEST_ASSERT_EQUAL_STRING(tmpl, html);

    free(html);
    tpl_ctx_free(ctx);
}

void test_if_true(void) {
    TplContext* ctx = tpl_ctx_create();
    tpl_ctx_set(ctx, "show", "yes");

    char* html = render_with("A{{#if show}}B{{/if}}C", ctx);
    TEST_ASSERT_EQUAL_STRING("ABC", html);

    free(html);
    tpl_ctx_free(ctx);
}

void test_if_false(void) {
    TplContext* ctx = tpl_ctx_create();
    // "show" not in context

    char* html = render_with("A{{#if show}}B{{/if}}C", ctx);
    TEST_ASSERT_EQUAL_STRING("AC", html);

    free(html);
    tpl_ctx_free(ctx);
}

void test_unless_true(void) {
    TplContext* ctx = tpl_ctx_create();
    // "flag" not set → unless renders

    char* html = render_with("{{#unless flag}}shown{{/unless}}", ctx);
    TEST_ASSERT_EQUAL_STRING("shown", html);

    free(html);
    tpl_ctx_free(ctx);
}

void test_unless_false(void) {
    TplContext* ctx = tpl_ctx_create();
    tpl_ctx_set(ctx, "flag", "1");

    char* html = render_with("{{#unless flag}}shown{{/unless}}", ctx);
    TEST_ASSERT_EQUAL_STRING("", html);

    free(html);
    tpl_ctx_free(ctx);
}

void test_mixed_template(void) {
    TplContext* ctx = tpl_ctx_create();
    tpl_ctx_set(ctx, "name",  "World");
    tpl_ctx_set(ctx, "extra", "yes");

    const char* tmpl =
        "<p>Hello, {{name}}!</p>"
        "{{#if extra}}<span>bonus</span>{{/if}}"
        "{{#if missing}}<span>nope</span>{{/if}}";

    char* html = render_with(tmpl, ctx);
    TEST_ASSERT_NOT_NULL(strstr(html, "Hello, World!"));
    TEST_ASSERT_NOT_NULL(strstr(html, "<span>bonus</span>"));
    TEST_ASSERT_NULL(strstr(html, "nope"));

    free(html);
    tpl_ctx_free(ctx);
}

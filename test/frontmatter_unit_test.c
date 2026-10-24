#include <unity.h>
#include <string.h>

#include "frontmatter.h"

void setUp(void) {}
void tearDown(void) {}

void test_no_frontmatter(void);
void test_single_field(void);
void test_multiple_fields(void);
void test_body_start(void);
void test_get_field(void);
void test_missing_field(void);
void test_quoted_values(void);
void test_crlf_line_endings(void);

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_no_frontmatter);
    RUN_TEST(test_single_field);
    RUN_TEST(test_multiple_fields);
    RUN_TEST(test_body_start);
    RUN_TEST(test_get_field);
    RUN_TEST(test_missing_field);
    RUN_TEST(test_quoted_values);
    RUN_TEST(test_crlf_line_endings);
    return UNITY_END();
}

void test_no_frontmatter(void) {
    const char* src = "# Hello\n\nSome content.\n";
    TplFrontmatter fm;
    bool ok = tpl_frontmatter_parse(src, &fm);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT(0, fm.field_count);
    TEST_ASSERT_EQUAL_PTR(src, fm.body_start);
}

void test_single_field(void) {
    const char* src = "---\ntitle: My Post\n---\n\nBody here.\n";
    TplFrontmatter fm;
    bool ok = tpl_frontmatter_parse(src, &fm);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT(1, fm.field_count);
    TEST_ASSERT_EQUAL_STRING("title", fm.fields[0].key);
    TEST_ASSERT_EQUAL_STRING("My Post", fm.fields[0].value);
}

void test_multiple_fields(void) {
    const char* src =
        "---\n"
        "title: Hello World\n"
        "date: 2025-01-15\n"
        "layout: post\n"
        "---\n"
        "\nBody.\n";
    TplFrontmatter fm;
    tpl_frontmatter_parse(src, &fm);
    TEST_ASSERT_EQUAL_UINT(3, fm.field_count);
    TEST_ASSERT_EQUAL_STRING("title",  fm.fields[0].key);
    TEST_ASSERT_EQUAL_STRING("Hello World", fm.fields[0].value);
    TEST_ASSERT_EQUAL_STRING("date",   fm.fields[1].key);
    TEST_ASSERT_EQUAL_STRING("2025-01-15", fm.fields[1].value);
    TEST_ASSERT_EQUAL_STRING("layout", fm.fields[2].key);
    TEST_ASSERT_EQUAL_STRING("post",   fm.fields[2].value);
}

void test_body_start(void) {
    const char* src = "---\ntitle: Test\n---\n\n# Heading\n";
    TplFrontmatter fm;
    tpl_frontmatter_parse(src, &fm);
    // body_start should point to the content after the closing ---\n
    TEST_ASSERT_NOT_NULL(fm.body_start);
    TEST_ASSERT_EQUAL_STRING("\n# Heading\n", fm.body_start);
}

void test_get_field(void) {
    const char* src = "---\ntitle: Test Title\nauthor: Alice\n---\n";
    TplFrontmatter fm;
    tpl_frontmatter_parse(src, &fm);
    TEST_ASSERT_EQUAL_STRING("Test Title", tpl_fm_get(&fm, "title"));
    TEST_ASSERT_EQUAL_STRING("Alice",      tpl_fm_get(&fm, "author"));
}

void test_missing_field(void) {
    const char* src = "---\ntitle: Test\n---\n";
    TplFrontmatter fm;
    tpl_frontmatter_parse(src, &fm);
    TEST_ASSERT_NULL(tpl_fm_get(&fm, "nonexistent"));
    TEST_ASSERT_NULL(tpl_fm_get(&fm, "date"));
}

void test_quoted_values(void) {
    const char* src = "---\ntitle: \"Quoted Title\"\ndescription: plain\n---\n";
    TplFrontmatter fm;
    tpl_frontmatter_parse(src, &fm);
    // Surrounding quotes should be stripped
    TEST_ASSERT_EQUAL_STRING("Quoted Title", tpl_fm_get(&fm, "title"));
    TEST_ASSERT_EQUAL_STRING("plain",        tpl_fm_get(&fm, "description"));
}

void test_crlf_line_endings(void) {
    const char* src = "---\r\ntitle: CRLF Test\r\ndate: 2025-06-01\r\n---\r\nBody.\r\n";
    TplFrontmatter fm;
    bool ok = tpl_frontmatter_parse(src, &fm);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("CRLF Test",  tpl_fm_get(&fm, "title"));
    TEST_ASSERT_EQUAL_STRING("2025-06-01", tpl_fm_get(&fm, "date"));
}

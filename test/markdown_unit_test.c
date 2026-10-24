#include <unity.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
#  define strdup _strdup
#endif

#include "markdown.h"
#include "seika/string.h"

void setUp(void) {}
void tearDown(void) {}

// Helper: render md and return a malloc'd copy of the result.
// Caller must free().
static char* render(const char* md) {
    SkaString out;
    ska_string_init_empty(&out);
    tpl_markdown_to_html(md, strlen(md), &out);
    // ska_string heap buffers are not null-terminated; copy with explicit length
    usize rlen = out.len;
    char* result = (char*)malloc(rlen + 1);
    memcpy(result, ska_string_as_slice(&out), rlen);
    result[rlen] = '\0';
    ska_string_deinit(&out);
    return result;
}

void test_paragraph(void);
void test_heading_h1(void);
void test_heading_h2_to_h6(void);
void test_heading_id_slug(void);
void test_heading_permalink(void);
void test_heading_slug_dedup(void);
void test_toc_extract(void);
void test_bold(void);
void test_italic(void);
void test_inline_code(void);
void test_link(void);
void test_image(void);
void test_unordered_list(void);
void test_ordered_list(void);
void test_fenced_code_block(void);
void test_fenced_code_block_with_lang(void);
void test_hr(void);
void test_blockquote(void);
void test_html_escaping_in_paragraph(void);
void test_multiple_paragraphs(void);
void test_raw_html_block(void);

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_paragraph);
    RUN_TEST(test_heading_h1);
    RUN_TEST(test_heading_h2_to_h6);
    RUN_TEST(test_heading_id_slug);
    RUN_TEST(test_heading_permalink);
    RUN_TEST(test_heading_slug_dedup);
    RUN_TEST(test_toc_extract);
    RUN_TEST(test_bold);
    RUN_TEST(test_italic);
    RUN_TEST(test_inline_code);
    RUN_TEST(test_link);
    RUN_TEST(test_image);
    RUN_TEST(test_unordered_list);
    RUN_TEST(test_ordered_list);
    RUN_TEST(test_fenced_code_block);
    RUN_TEST(test_fenced_code_block_with_lang);
    RUN_TEST(test_hr);
    RUN_TEST(test_blockquote);
    RUN_TEST(test_html_escaping_in_paragraph);
    RUN_TEST(test_multiple_paragraphs);
    RUN_TEST(test_raw_html_block);
    return UNITY_END();
}

void test_paragraph(void) {
    char* html = render("Hello world.\n");
    TEST_ASSERT_NOT_NULL(html);
    TEST_ASSERT_NOT_NULL(strstr(html, "<p>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "Hello world."));
    TEST_ASSERT_NOT_NULL(strstr(html, "</p>"));
    free(html);
}

void test_heading_h1(void) {
    char* html = render("# Title\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<h1 id=\"title\">"));
    TEST_ASSERT_NOT_NULL(strstr(html, "Title"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</h1>"));
    free(html);
}

void test_heading_h2_to_h6(void) {
    char* h2 = render("## H2\n");
    TEST_ASSERT_NOT_NULL(strstr(h2, "<h2 id=\"h2\">")); free(h2);
    char* h3 = render("### H3\n");
    TEST_ASSERT_NOT_NULL(strstr(h3, "<h3 id=\"h3\">")); free(h3);
    char* h4 = render("#### H4\n");
    TEST_ASSERT_NOT_NULL(strstr(h4, "<h4 id=\"h4\">")); free(h4);
    char* h5 = render("##### H5\n");
    TEST_ASSERT_NOT_NULL(strstr(h5, "<h5 id=\"h5\">")); free(h5);
    char* h6 = render("###### H6\n");
    TEST_ASSERT_NOT_NULL(strstr(h6, "<h6 id=\"h6\">")); free(h6);
}

void test_heading_id_slug(void) {
    char* html = render("## IAnimatedSprite\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<h2 id=\"ianimatedsprite\">"));
    free(html);

    char slug[64];
    const char* text = "Constructing a custom INode type";
    tpl_markdown_slugify(text, strlen(text), slug, sizeof(slug));
    TEST_ASSERT_EQUAL_STRING("constructing-a-custom-inode-type", slug);
}

void test_heading_permalink(void) {
    char* html = render("## IAnimatedSprite\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<a class=\"heading-anchor\" href=\"#ianimatedsprite\">#</a>"));
    free(html);
}

void test_heading_slug_dedup(void) {
    char* html = render("## Overview\n\nfirst\n\n## Overview\n\nsecond\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "id=\"overview\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "id=\"overview-2\""));
    free(html);
}

void test_toc_extract(void) {
    const char* md =
        "# Title (skipped, h1)\n\n"
        "## INode\n\nbody\n\n"
        "### Constructing a custom INode type\n\nbody\n\n"
        "## IAnimatedSprite\n\nbody\n";
    TplMarkdownTocEntry toc[TPL_MARKDOWN_TOC_MAX];
    usize count = 0;
    tpl_markdown_extract_toc(md, strlen(md), toc, TPL_MARKDOWN_TOC_MAX, &count);

    TEST_ASSERT_EQUAL_UINT(3, count);
    TEST_ASSERT_EQUAL_STRING("INode", toc[0].text);
    TEST_ASSERT_EQUAL_STRING("inode", toc[0].slug);
    TEST_ASSERT_EQUAL(2, toc[0].level);
    TEST_ASSERT_EQUAL_STRING("Constructing a custom INode type", toc[1].text);
    TEST_ASSERT_EQUAL_STRING("constructing-a-custom-inode-type", toc[1].slug);
    TEST_ASSERT_EQUAL(3, toc[1].level);
    TEST_ASSERT_EQUAL_STRING("IAnimatedSprite", toc[2].text);
    TEST_ASSERT_EQUAL_STRING("ianimatedsprite", toc[2].slug);
    TEST_ASSERT_EQUAL(2, toc[2].level);
}

void test_bold(void) {
    char* html = render("**bold text**\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<strong>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "bold text"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</strong>"));
    free(html);
}

void test_italic(void) {
    char* html = render("*italic text*\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<em>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "italic text"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</em>"));
    free(html);
}

void test_inline_code(void) {
    char* html = render("Use `printf()` here.\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<code>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "printf()"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</code>"));
    free(html);
}

void test_link(void) {
    char* html = render("[Click here](https://example.com)\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<a href=\"https://example.com\">"));
    TEST_ASSERT_NOT_NULL(strstr(html, "Click here"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</a>"));
    free(html);
}

void test_image(void) {
    char* html = render("![Alt text](photo.jpg)\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<img src=\"photo.jpg\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "alt=\"Alt text\""));
    free(html);
}

void test_unordered_list(void) {
    char* html = render("- Alpha\n- Beta\n- Gamma\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<ul>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "<li>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "Alpha"));
    TEST_ASSERT_NOT_NULL(strstr(html, "Beta"));
    TEST_ASSERT_NOT_NULL(strstr(html, "Gamma"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</li>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</ul>"));
    free(html);
}

void test_ordered_list(void) {
    char* html = render("1. First\n2. Second\n3. Third\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<ol>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "<li>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "First"));
    TEST_ASSERT_NOT_NULL(strstr(html, "Second"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</li>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</ol>"));
    free(html);
}

void test_fenced_code_block(void) {
    char* html = render("```\nint x = 0;\n```\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<pre><code>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "int x = 0;"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</code></pre>"));
    free(html);
}

void test_fenced_code_block_with_lang(void) {
    char* html = render("```c\nvoid foo(void) {}\n```\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "language-c"));
    TEST_ASSERT_NOT_NULL(strstr(html, "void foo"));
    free(html);
}

void test_hr(void) {
    char* html = render("---\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<hr>"));
    free(html);
}

void test_blockquote(void) {
    char* html = render("> Quoted text\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "<blockquote>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "Quoted text"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</blockquote>"));
    free(html);
}

void test_html_escaping_in_paragraph(void) {
    char* html = render("A < B & C > D\n");
    TEST_ASSERT_NOT_NULL(strstr(html, "&lt;"));
    TEST_ASSERT_NOT_NULL(strstr(html, "&amp;"));
    TEST_ASSERT_NOT_NULL(strstr(html, "&gt;"));
    // Raw characters should NOT appear outside of the escaped forms
    TEST_ASSERT_NULL(strstr(html, "A < B"));
    free(html);
}

void test_multiple_paragraphs(void) {
    char* html = render("First paragraph.\n\nSecond paragraph.\n");
    // Should contain two <p> tags
    char* first  = strstr(html, "<p>");
    TEST_ASSERT_NOT_NULL(first);
    char* second = strstr(first + 1, "<p>");
    TEST_ASSERT_NOT_NULL(second);
    free(html);
}

void test_raw_html_block(void) {
    char* html = render("<div class=\"foo\">\n  <p>Hello</p>\n</div>\n");
    // Tags must pass through verbatim, not escaped
    TEST_ASSERT_NOT_NULL(strstr(html, "<div class=\"foo\">"));
    TEST_ASSERT_NOT_NULL(strstr(html, "<p>Hello</p>"));
    TEST_ASSERT_NOT_NULL(strstr(html, "</div>"));
    TEST_ASSERT_NULL(strstr(html, "&lt;div"));
    free(html);
}

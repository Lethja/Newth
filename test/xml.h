#ifndef NEW_DL_TEST_XML_H
#define NEW_DL_TEST_XML_H

#include "../src/client/xml.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

#pragma region CMocka headers

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>

#pragma endregion

const char *HtmlBody = "<!DOCTYPE html>\n"
                       "<html>\n"
                       "<head>\n"
                       "\t<title>Directory Listing</title>\n"
                       "</head>\n"
                       "<body>\n"
                       "\t<h1>Directory Listing</h1>\n"
                       "\t<img href=\"Logo.png\" alt=\"Logo\">\n"
                       "\t<ul>\n"
                       "\t\t<li><a href=\"file1.txt\">file1.txt</a></li>\n"
                       "\t\t<li><a href=\"file2%2Etxt\">file2.txt</a></li>\n"
                       "\t\t<li><a href=\"/file3.txt\">file3.txt</a></li>\n"
                       "\t\t<li><a href=\"../file4.txt\">file4.txt</a></li>\n"
                       "\t\t<li><a href=\"/foo/file5.txt\">file5.txt</a></li>\n"
                       "\t\t<li><a href=\"http://example.com\">file6.txt</a></li>\n"
                       "\t</ul>\n"
                       "</body>\n"
                       "</html>";

static void XmlExtractHtmlAttributeValue(void **state) {
    const char *element1 = "A", *good = "href";
    char *element = XmlFindElement(HtmlBody, element1), *attribute, *value;

    assert_non_null(element);
    assert_non_null((attribute = XmlFindAttribute(element, good)));
    assert_non_null((value) = XmlExtractAttributeValue(attribute));
    assert_string_equal(value, "file1.txt"), free(value);
}

static void XmlFindExtractAttribute(void **state) {
    const char *element1 = "BODY", *element2 = "a", *bad = "FOOBAR", *good = "HREF";
    char *element = XmlFindElement(HtmlBody, element1), *attribute;
    assert_non_null(element);

    assert_null((attribute = XmlExtractAttribute(element, good))); /* <body> has no attributes, null should return */
    assert_null((attribute = XmlExtractAttribute(element, bad)));
    assert_non_null((element = XmlFindElement(element, element2)));
    assert_null((attribute = XmlExtractAttribute(element, bad)));
    assert_non_null((attribute = XmlExtractAttribute(element, good)));
    assert_string_equal(attribute, "href=\"file1.txt\"");
    free(attribute);
}

static void XmlFindExtractElement(void **state) {
    const char *good1 = "BODY", *good2 = "a", *bad1 = "FOOBAR", *bad2 = "HREF";
    char *element;

    assert_null(XmlExtractElement(HtmlBody, bad1));
    assert_null(XmlExtractElement(HtmlBody, bad2));
    assert_non_null((element = XmlExtractElement(HtmlBody, good1)));
    assert_string_equal(element, "<body>"), free(element);
    assert_non_null((element = XmlExtractElement(&HtmlBody[72], good2)));
    assert_string_equal(element, "<a href=\"file1.txt\">"), free(element);
    assert_non_null((element = XmlExtractElement(&HtmlBody[154], good2)));
    assert_string_equal(element, "<a href=\"file2%2Etxt\">"), free(element);
}

static void XmlFindHtmlAttributePtr(void **state) {
    const char *element1 = "BODY", *element2 = "a", *bad = "FOOBAR", *good = "HREF";
    char *element = XmlFindElement(HtmlBody, element1), *attribute;
    assert_non_null(element);

    assert_null((attribute = XmlFindAttribute(element, good))); /* <body> has no attributes, null should return */
    assert_null((attribute = XmlFindAttribute(element, bad)));
    assert_non_null((element = XmlFindElement(element, element2)));
    assert_null((attribute = XmlFindAttribute(element, bad)));
    assert_non_null((attribute = XmlFindAttribute(element, good)));
    assert_ptr_equal(&element[3], attribute);
}

static void XmlFindHtmlElementPtr(void **state) {
    const char *good1 = "BODY", *good2 = "a", *bad1 = "FOOBAR", *bad2 = "HREF";
    char *element;

    assert_null(XmlFindElement(HtmlBody, bad1));
    assert_null(XmlFindElement(HtmlBody, bad2));
    assert_non_null((element = XmlFindElement(HtmlBody, good1)));
    assert_ptr_equal(element, &HtmlBody[72]);
    assert_non_null((element = XmlFindElement(&HtmlBody[72], good2)));
    assert_ptr_equal(element, &HtmlBody[153]);
    assert_ptr_equal(XmlFindElement(HtmlBody, good2), element);
    assert_non_null((element = XmlFindElement(&HtmlBody[154], good2)));
    assert_ptr_equal(element, &HtmlBody[198]);
}

#pragma clang diagnostic pop

const struct CMUnitTest xmlTest[] = {
    cmocka_unit_test(XmlFindHtmlElementPtr),
    cmocka_unit_test(XmlFindHtmlAttributePtr),
    cmocka_unit_test(XmlExtractHtmlAttributeValue),
    cmocka_unit_test(XmlFindExtractElement),
    cmocka_unit_test(XmlFindExtractAttribute)
};

#endif /* NEW_DL_TEST_XML_H */

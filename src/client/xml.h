#ifndef NEW_DL_XML_H
#define NEW_DL_XML_H

#include "../platform/platform.h"

/**
 * Get an element in a XML document as its own allocation
 * @param xml The xml document to scan
 * @param element The element name to retrieve
 * @return String with the contents between <> of the first instance of element found or NULL
 * @remark Returned string should be freed before leaving scope
 */
char *XmlExtractElement(const char *xml, const char *element);

/**
 * Get an attribute in element
 * @param element Element pointer returned by XmlFindElement
 * @param attribute The tag name to retrieve
 * @return String with the contents of the attribute element found or NULL
 * @remark Returned string should be freed before leaving scope
 */
char *XmlExtractAttribute(const char *element, const char *attribute);

/**
 * Get the value of an attribute if it has one
 * @param attribute Attribute pointer return by XmlFindAttribute
 * @return String with the value or NULL
 * @remark Returned string should be freed before leaving scope
 */
char *XmlExtractAttributeValue(const char *attribute);

/**
 * Get an element in a XML document
 * @param xml The xml document to scan
 * @param element The element name to retrieve
 * @return Pointer within xml to the first instance of element found or NULL
 */
char *XmlFindElement(const char *xml, const char *element);

/**
 * Get an attribute in element
 * @param element Element pointer returned by XmlFindElement
 * @param attribute The tag name to retrieve
 * @return Pointer within xml to the first attribute found or NULL
 */
char *XmlFindAttribute(const char *element, const char *attribute);

#endif /* NEW_DL_XML_H */

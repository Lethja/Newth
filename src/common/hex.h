#ifndef NEW_COMMON_URI_H
#define NEW_COMMON_URI_H

/**
 * Convert any and all hex values in a typical uri string to their ascii equivalents. %20 becomes a space for example
 * @param url In-Out: The string to convert
 * @remark The output will be <= the input.
 * On long lived strings, check string length after conversion and reallocate as appropriate to free memory
 */
void hexConvertStringToAscii(char *url);

#endif /* NEW_COMMON_URI_H */

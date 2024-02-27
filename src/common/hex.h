#ifndef NEW_COMMON_URI_H
#define NEW_COMMON_URI_H

#include "../platform/platform.h"

/**
 * Convert any and all hex values in a typical uri string to their ascii equivalents. %20 becomes a space for example
 * @param url In-Out: The string to convert
 * @remark The output will be <= the input.
 * On long lived strings, check string length after conversion and reallocate as appropriate to free memory
 */
void hexConvertStringToAscii(char *url);

/**
 * Calculate the amount of character in a string needed to represent a number as a hexadecimal
 * @param number The number get the hexadecimal string length of
 * @return The length required in a string to fit the number as a hexadecimal string
 * @remark Does not include a leading 0x in calculations.
 * If you would like to display a string this way add 2 to the result
 */
size_t hexGetStringLength(size_t number);

#endif /* NEW_COMMON_URI_H */

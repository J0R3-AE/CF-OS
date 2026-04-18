/**
 * @file string.h
 * @brief Minimal C‑library string routines for the kernel.
 *
 * This header provides a lightweight subset of common libc string
 * functions, implemented internally by the kernel. These routines
 * operate on NUL‑terminated byte strings and avoid dependencies on
 * the standard C library.
 */

#ifndef STRING_H
#define STRING_H

#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* Basic String Operations                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Compute the length of a NUL‑terminated string.
 *
 * @param s Input string.
 * @return Number of characters before the terminating NUL.
 */
usize strlen(const char *s);

/**
 * @brief Copy a NUL‑terminated string.
 *
 * Copies @p src into @p dest. Behavior is undefined if the buffers
 * overlap. The destination must be large enough to hold the result.
 *
 * @param dest Destination buffer.
 * @param src  Source string.
 * @return dest
 */
char *strcpy(char *dest, const char *src);

/**
 * @brief Copy at most @p n characters from @p src to @p dest.
 *
 * If @p src is shorter than @p n, the remainder of @p dest is padded
 * with NUL bytes.
 *
 * @param dest Destination buffer.
 * @param src  Source string.
 * @param n    Maximum number of characters to copy.
 * @return dest
 */
char *strncpy(char *dest, const char *src, usize n);

/* -------------------------------------------------------------------------- */
/* Character Search                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Find the first occurrence of character @p c in @p s.
 *
 * @param s Input string.
 * @param c Character to search for.
 * @return Pointer to the first occurrence, or NULL if not found.
 */
char *strchr(const char *s, int c);

/**
 * @brief Find the last occurrence of character @p c in @p s.
 *
 * @param s Input string.
 * @param c Character to search for.
 * @return Pointer to the last occurrence, or NULL if not found.
 */
char *strrchr(const char *s, int c);

/* -------------------------------------------------------------------------- */
/* String Comparison                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Compare two NUL‑terminated strings.
 *
 * @param a First string.
 * @param b Second string.
 * @return 0 if equal, <0 if a<b, >0 if a>b.
 */
int strcmp(const char *a, const char *b);

/**
 * @brief Compare up to @p n characters of two strings.
 *
 * @param a First string.
 * @param b Second string.
 * @param n Maximum number of characters to compare.
 * @return 0 if equal, <0 if a<b, >0 if a>b.
 */
int strncmp(const char *a, const char *b, usize n);

/**
 * @brief Case‑insensitive string comparison (ASCII only).
 *
 * @param a First string.
 * @param b Second string.
 * @return 0 if equal ignoring case.
 */
int strcasecmp(const char *a, const char *b);

/* -------------------------------------------------------------------------- */
/* Substring Search                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Find the first occurrence of @p needle in @p haystack.
 *
 * @param haystack String to search within.
 * @param needle   Substring to search for.
 * @return Pointer to the first match, or NULL if not found.
 */
char *strstr(const char *haystack, const char *needle);

/* -------------------------------------------------------------------------- */
/* Concatenation                                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Append @p src to @p dest.
 *
 * The destination buffer must be large enough to hold the result.
 *
 * @param dest Destination buffer.
 * @param src  Source string.
 * @return dest
 */
char *strcat(char *dest, const char *src);

/**
 * @brief Append at most @p n characters of @p src to @p dest.
 *
 * @param dest Destination buffer.
 * @param src  Source string.
 * @param n    Maximum characters to append.
 * @return dest
 */
char *strncat(char *dest, const char *src, usize n);

/* -------------------------------------------------------------------------- */
/* Span Functions                                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Length of the initial segment of @p s containing only characters in @p accept.
 *
 * @param s      Input string.
 * @param accept Set of accepted characters.
 * @return Length of the segment.
 */
usize strspn(const char *s, const char *accept);

/**
 * @brief Length of the initial segment of @p s containing no characters in @p reject.
 *
 * @param s      Input string.
 * @param reject Set of rejected characters.
 * @return Length of the segment.
 */
usize strcspn(const char *s, const char *reject);

/* -------------------------------------------------------------------------- */
/* Allocation Helpers                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Duplicate a NUL‑terminated string using kmalloc().
 *
 * @param src Source string.
 * @return Newly allocated copy, or NULL on failure.
 */
char *strdup(const char *src);

/**
 * @brief Duplicate up to @p maxlen characters of @p src.
 *
 * The resulting string is always NUL‑terminated.
 *
 * @param src     Source string.
 * @param maxlen  Maximum characters to copy.
 * @return Newly allocated copy, or NULL on failure.
 */
char *strndup(const char *src, usize maxlen);

/* -------------------------------------------------------------------------- */
/* Tokenization                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Tokenize a string using delimiters in @p delim.
 *
 * Behaves similarly to the standard C strtok(), but implemented
 * internally for kernel use. Modifies the input string in place.
 *
 * @param s     String to tokenize, or NULL to continue tokenizing.
 * @param delim Set of delimiter characters.
 * @return Pointer to next token, or NULL if no more tokens exist.
 */
char *strtok(char *s, const char *delim);

#endif /* STRING_H */

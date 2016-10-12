#ifndef BKD_STRING_H_EFHP8VH0
#define BKD_STRING_H_EFHP8VH0

#include "bkd.h"

/* Returns a substring of string in the same memory location */
struct bkd_string bkd_strsub(struct bkd_string string, int32_t index1, int32_t index2);

/* Returns a copy (newly alloacated) substring */
struct bkd_string bkd_strsub_new(struct bkd_string string, int32_t index1, int32_t index2);

/* Returns a clone of string */
struct bkd_string bkd_str_new(struct bkd_string string);

/* Creates a string from a c string */
struct bkd_string bkd_cstr(const char * cstr);

/* Creates a new string from a c string */
struct bkd_string bkd_cstr_new(const char * cstr);

/* Concatenates two strings. */
struct bkd_string bkd_strconcat_new(struct bkd_string str1, struct bkd_string str2);

/* Find a codepoint. Returns if point found, and if so places the index in index. */
int bkd_strfind(struct bkd_string string, uint32_t codepoint, uint32_t * index);

/* Gets the indentation of a string. Whitespace is 1 indentation level, with
 * the exception of tabs, which are 4. Other unicode whitespace in the high
 * ranges should probably be avoided. */
uint32_t bkd_strindent(struct bkd_string string);

/* Checks if a string contains only whitespace. */
int bkd_strempty(struct bkd_string string);

/* Strip n units of whitespace from front of string. Allocates a new string
 * for cases in which a single tab must be replaced with multiple characters.
 */
struct bkd_string bkd_strstripn_new(struct bkd_string string, uint32_t n);

/* Trims whitespace from beginning and end of string. */
struct bkd_string bkd_strtrim(struct bkd_string string, int front, int back);

#define bkd_strtrim_front(S) bkd_strtrim((S), 1, 0)
#define bkd_strtrim_back(S) bkd_strtrim((S), 0, 1)
#define bkd_strtrim_both(S) bkd_strtrim((S), 1, 1)

/* Trims character from beginning and end of string. */
struct bkd_string bkd_strtrimc(struct bkd_string string, uint32_t codepoint, int front, int back);

#define bkd_strtrimc_front(S, C) bkd_strtrimc((S), (C), 1, 0)
#define bkd_strtrimc_back(S, C) bkd_strtrimc((S), (C), 0, 1)
#define bkd_strtrimc_both(S, C) bkd_strtrimc((S), (C), 1, 1)

/* Frees a string. */
void bkd_strfree(struct bkd_string string);

/* Create a new buffer */
struct bkd_buffer bkd_bufnew(uint32_t capacity);

/* Free a buffer */
void bkd_buffree(struct bkd_buffer buffer);

/* Push a string onto the buffer */
struct bkd_buffer bkd_bufpush(struct bkd_buffer buffer, struct bkd_string string);

struct bkd_buffer bkd_bufpushc(struct bkd_buffer buffer, uint32_t codepoint);

struct bkd_buffer bkd_bufpushb(struct bkd_buffer buffer, uint8_t byte);

#endif /* end of include guard: BKD_STRING_H_EFHP8VH0 */

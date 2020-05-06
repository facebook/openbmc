#ifndef __STRLIB_H__
#define __STRLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

int index_of(const char *str, const char *c);
int from_index_of(int from_index, const char *str, const char *c);
int start_with(const char *str, const char *c);
int end_with(const char *str, const char *c);
int trim(char *dst, char *str);
int split(char **dst, char *src, char *delim);
int is_number(const char *p);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __STRLIB_H__ */

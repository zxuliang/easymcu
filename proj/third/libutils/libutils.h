#ifndef __LIB_UTILS__
#define __LIB_UTILS__

#include <sys/types.h>
#include <stdint.h>

#define va_list			__builtin_va_list
#define va_end				__builtin_va_end
#define va_arg				__builtin_va_arg
#define va_start(v,l)	__builtin_va_start((v),l)

#define __SECTION(x)__				__attribute__((section(x)))
#define __UNUSED__ 					__attribute__((unused))
#define __USED__						__attribute__((used))
#define __ALIGN(n)__					__attribute__((aligned(n)))
#define __WEAK__						__attribute__((weak))


void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char *strstr(const char *s1, const char *s2);
int strcasecmp(const char *a, const char *b);
char *strncpy(char *dest, const char *src, size_t n);
int strncmp(const char *cs, const char *ct, size_t count);

void kprint(const char *fmt, ...);

int sprintf(char *buf, const char *format, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsprintf(char *buf, const char *format, va_list arg_ptr);

#define isdigit(c)  ((unsigned)((c) - '0') < 10)

#endif

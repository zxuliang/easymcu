#ifndef __LIB_UTILS__
#define __LIB_UTILS__


/* COPY FROM UBOOT AND MODIFY SOMETHING */

#include <sys/types.h>
#include <stdint.h>

#define va_list			__builtin_va_list
#define va_end			__builtin_va_end
#define va_arg			__builtin_va_arg
#define va_start(v,l)		__builtin_va_start((v),l)

#define SECTION(x)		__attribute__((section(x)))
#define UNUSED 			__attribute__((unused))
#define USED			__attribute__((used))
#define ALIGN(n)		__attribute__((aligned(n)))
#define WEAK			__attribute__((weak))

int divide(long *n, int base);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char *strstr(const char *s1, const char *s2);
char * strchr(const char * s, int c);
int strcasecmp(const char *a, const char *b);
char * strcpy(char * dest,const char *src);
char *strncpy(char *dest, const char *src, size_t n);
int strcmp(const char *cs, const char *ct);
int strncmp(const char *cs, const char *ct, size_t count);

void printk(const char *fmt, ...);
int sprintf(char *buf, const char *format, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsprintf(char *buf, const char *format, va_list arg_ptr);

int putchar(int c);
int putc(unsigned char c);
int getc(void);
int puts(const char *s);

extern unsigned long jiffies;
void delayms(uint32_t ticks);


/*
 * NOTE! This ctype does not handle EOF like the standard C
 * library is required to.
 */

#define _U	0x01	/* upper */
#define _L	0x02	/* lower */
#define _D	0x04	/* digit */
#define _C	0x08	/* cntrl */
#define _P	0x10	/* punct */
#define _S	0x20	/* white space (space/lf/tab) */
#define _X	0x40	/* hex digit */
#define _SP	0x80	/* hard space (0x20) */

extern unsigned char _ctype[];

#define __ismask(x) (_ctype[(int)(unsigned char)(x)])

#define isalnum(c)	((__ismask(c)&(_U|_L|_D)) != 0)
#define isalpha(c)	((__ismask(c)&(_U|_L)) != 0)
#define iscntrl(c)	((__ismask(c)&(_C)) != 0)
#define isdigit(c)	((__ismask(c)&(_D)) != 0)
#define isgraph(c)	((__ismask(c)&(_P|_U|_L|_D)) != 0)
#define islower(c)	((__ismask(c)&(_L)) != 0)
#define isprint(c)	((__ismask(c)&(_P|_U|_L|_D|_SP)) != 0)
#define ispunct(c)	((__ismask(c)&(_P)) != 0)
#define isspace(c)	((__ismask(c)&(_S)) != 0)
#define isupper(c)	((__ismask(c)&(_U)) != 0)
#define isxdigit(c)	((__ismask(c)&(_D|_X)) != 0)

#define isascii(c) (((unsigned char)(c))<=0x7f)
#define toascii(c) (((unsigned char)(c))&0x7f)

static inline unsigned char __tolower(unsigned char c)
{
	if (isupper(c))
		c -= 'A'-'a';
	return c;
}

static inline unsigned char __toupper(unsigned char c)
{
	if (islower(c))
		c -= 'a'-'A';
	return c;
}

#define tolower(c) __tolower(c)
#define toupper(c) __toupper(c)

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base);


#endif

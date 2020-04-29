#ifndef __UBOOT_SHELL__
#define __UBOOT_SHELL__

#include <ushell/command.h>
#include <libutils/utils.h>

extern char console_buffer[CFG_CBSIZE];

extern void console_init(void);
extern void console_puts(const char *str);
extern void console_putchar(char c);
extern unsigned char console_getchar(void);


#endif

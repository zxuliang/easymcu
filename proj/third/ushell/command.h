#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <libutils/utils.h>

/* COPY FORM UBOOT */

#define CFG_CBSIZE (256)
#define CFG_MAXARGS (16)

struct cmd_tbl_s {
	char		*name;		/* Command Name			*/
	int		maxargs;	/* maximum number of arguments	*/
	int		repeatable;	/* autorepeat allowed?		*/
					/* Implementation function	*/
	int		(*cmd)(struct cmd_tbl_s *, int, int, char *[]);
	char		*usage;		/* Usage message	(short)	*/
	char		*help;		/* Help  message	(long)	*/
};

typedef struct cmd_tbl_s	cmd_tbl_t;

extern int readline (const char *const prompt);
extern int run_command (const char *cmd, int flag);

extern cmd_tbl_t  __u_boot_cmd_start;
extern cmd_tbl_t  __u_boot_cmd_end;

#define Struct_Section  __attribute__ ((unused,section (".u_boot_cmd")))

#define U_BOOT_CMD(name,maxargs,rep,cmd,usage,help) \
cmd_tbl_t __u_boot_cmd_##name Struct_Section = {#name, maxargs, rep, cmd, usage, help}

#endif /* __COMMAND_H__ */


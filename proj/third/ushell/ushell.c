#include <ushell/command.h>

char console_buffer[CFG_CBSIZE];		/* console I/O buffer	*/
static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen);
static char erase_seq[] = "\b \b";		/* erase sequence	*/
static char   tab_seq[] = "        ";		/* used to expand TABs	*/
static int ctrlc_was_pressed = 0;
/*
* Prompt for input and read a line.
* If CONFIG_BOOT_RETRY_TIME is defined and retry_time >= 0,
* time out when time goes past endtime (timebase time in ticks).
* Return: number of read characters
* -1 if break
* -2 if timed out
*/
int readline (const char *const prompt)
{
	char *p = console_buffer;
	int n = 0; /* buffer index */
	int plen = 0; /* prompt length */
	int col; /* output column cnt */
	char c;

	/* print prompt */
	if (prompt) {
		plen = strlen (prompt);
		puts(prompt);
	}
	col = plen;

	for (;;) {
		c = getc();
		/*
		* Special character handling
		*/
		switch (c) {
			case '\r': /* Enter */
			case '\n':
						*p = '\0';
						puts ("\r\n");
						return (p - console_buffer);

			case '\0': /* nul */
						continue;

			case 0x03: /* ^C - break */
						console_buffer[0] = '\0'; /* discard input */
						return (-1);

			case 0x15: /* ^U - erase line */
						while (col > plen) {
							puts (erase_seq);
							--col;
						}
						p = console_buffer;
						n = 0;
						continue;

			case 0x17: /* ^W - erase word */
						p=delete_char(console_buffer, p, &col, &n, plen);
						while ((n > 0) && (*p != ' ')) {
							p=delete_char(console_buffer, p, &col, &n, plen);
						}
						continue;

			case 0x08: /* ^H - backspace */
			case 0x7F: /* DEL - backspace */
						p=delete_char(console_buffer, p, &col, &n, plen);
						continue;

			default:
						/*
						* Must be a normal character then
						*/
						if (n < CFG_CBSIZE-2) {
							if (c == '\t') { /* expand TABs */
								puts (tab_seq+(col&07));
								col += 8 - (col&07);
							}
							else {
										++col; /* echo input */
										putc (c);
							}
							*p++ = c;
							++n;
						}
						else { /* Buffer full */
							putc ('\a');
						}
		}
	}
}

static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen)
{
	char *s;

	if (*np == 0) {
		return (p);
	}

	if (*(--p) == '\t') {			/* will retype the whole line	*/
		while (*colp > plen) {
			puts (erase_seq);
			(*colp)--;
		}
		for (s=buffer; s<p; ++s) {
			if (*s == '\t') {
				puts (tab_seq+((*colp) & 07));
				*colp += 8 - ((*colp) & 07);
			} else {
				++(*colp);
				putc (*s);
			}
		}
	} else {
		puts (erase_seq);
		(*colp)--;
	}
	(*np)--;
	return (p);
}

static int had_ctrlc (void)
{
	return ctrlc_was_pressed;
}

static void clear_ctrlc (void)
{
	ctrlc_was_pressed = 0;
}

int ctrlc (void)
{
	switch (getc()) {
	case 0x03:		/* ^C - Control C */
		ctrlc_was_pressed = 1;
		return 1;
	default:
		break;
	}

	return 0;
}


/****************************************************************************/

static void process_macros (const char *input, char *output)
{
	char c, prev;
	int inputcnt = strlen (input);
	int outputcnt = CFG_CBSIZE;
	int state = 0;		/* 0 = waiting for '$'  */

	/* 1 = waiting for '(' or '{' */
	/* 2 = waiting for ')' or '}' */
	/* 3 = waiting for '''  */

	prev = '\0';		/* previous character   */

	while (inputcnt && outputcnt) {
		c = *input++;
		inputcnt--;

		if (state != 3) {
			/* remove one level of escape characters */
			if ((c == '\\') && (prev != '\\')) {
				if (inputcnt-- == 0)
					break;
				prev = c;
				c = *input++;
			}
		}

		switch (state) {
		case 0:	/* Waiting for (unescaped) $    */
			if ((c == '\'') && (prev != '\\')) {
				state = 3;
				break;
			}
			if ((c == '$') && (prev != '\\')) {
				state++;
			} else {
				*(output++) = c;
				outputcnt--;
			}
			break;
		case 1:	/* Waiting for (        */
			if (c == '(' || c == '{') {
				state++;
			} else {
				state = 0;
				*(output++) = '$';
				outputcnt--;

				if (outputcnt) {
					*(output++) = c;
					outputcnt--;
				}
			}
			break;
		case 2:	/* Waiting for )        */
			if (c == ')' || c == '}') {
				char *envval = NULL;
				/* Copy into the line if it exists */
				if (envval != NULL)
					while ((*envval) && outputcnt) {
						*(output++) = *(envval++);
						outputcnt--;
					}
				/* Look for another '$' */
				state = 0;
			}
			break;
		case 3:	/* Waiting for '        */
			if ((c == '\'') && (prev != '\\')) {
				state = 0;
			} else {
				*(output++) = c;
				outputcnt--;
			}
			break;
		}
		prev = c;
	}

	if (outputcnt)
		*output = 0;
}

/****************************************************************************/

static int parse_line (char *line, char *argv[])
{
	int nargs = 0;

	while (nargs < CFG_MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return (nargs);
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return (nargs);
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	printk("** Too many args (max. %d) **\n", CFG_MAXARGS);
	return (nargs);
}

/***************************************************************************
 * find command table entry for a command
 */
static cmd_tbl_t *find_cmd (const char *cmd)
{
	cmd_tbl_t *cmdtp;
	cmd_tbl_t *cmdtp_temp = &__u_boot_cmd_start;	/*Init value */
	const char *p;
	int len;
	int n_found = 0;

	/*
	 * Some commands allow length modifiers (like "cp.b");
	 * compare command name only until first dot.
	 */
	len = ((p = strchr(cmd, '.')) == NULL) ? strlen (cmd) : (p - cmd);

	for (cmdtp = &__u_boot_cmd_start;
	     cmdtp != &__u_boot_cmd_end;
	     cmdtp++) {
		if (strncmp (cmd, cmdtp->name, len) == 0) {
			if (len == strlen (cmdtp->name))
				return cmdtp;	/* full match */

			cmdtp_temp = cmdtp;	/* abbreviated command ? */
			n_found++;
		}
	}
	if (n_found == 1) {			/* exactly one match */
		return cmdtp_temp;
	}

	return NULL;	/* not found or ambiguous command */
}

/****************************************************************************
 * returns:
 *	1  - command executed, repeatable
 *	0  - command executed but not repeatable, interrupted commands are
 *	     always considered not repeatable
 *	-1 - not executed (unrecognized, bootd recursion or too many args)
 *           (If cmd is NULL or "" or longer than CFG_CBSIZE-1 it is
 *           considered unrecognized)
 *
 * WARNING:
 *
 * We must create a temporary copy of the command since the command we get
 * may be the result from getenv(), which returns a pointer directly to
 * the environment data, which may change magicly when the command we run
 * creates or modifies environment variables (like "bootp" does).
 */

int run_command (const char *cmd, int flag)
{
	cmd_tbl_t *cmdtp;
	char cmdbuf[CFG_CBSIZE];	/* working copy of cmd		*/
	char *token;			/* start of token in cmdbuf	*/
	char *sep;			/* end of token (separator) in cmdbuf */
	char finaltoken[CFG_CBSIZE];
	char *str = cmdbuf;
	char *argv[CFG_MAXARGS + 1];	/* NULL terminated	*/
	int argc, inquotes;
	int repeatable = 1;
	int rc = 0;

	clear_ctrlc();		/* forget any previous Control C */

	if (!cmd || !*cmd) {
		return -1;	/* empty command */
	}

	if (strlen(cmd) >= CFG_CBSIZE) {
		puts ("## Command too long!\n");
		return -1;
	}

	strcpy (cmdbuf, cmd);

	/* Process separators and check for invalid
	 * repeatable commands
	 */
	while (*str) {

		/*
		 * Find separator, or string end
		 * Allow simple escape of ';' by writing "\;"
		 */
		for (inquotes = 0, sep = str; *sep; sep++) {
			if ((*sep=='\'') &&
			    (*(sep-1) != '\\'))
				inquotes=!inquotes;

			if (!inquotes &&
			    (*sep == ';') &&	/* separator		*/
			    ( sep != str) &&	/* past string start	*/
			    (*(sep-1) != '\\'))	/* and NOT escaped	*/
				break;
		}

		/*
		 * Limit the token to data between separators
		 */
		token = str;
		if (*sep) {
			str = sep + 1;	/* start of command for next pass */
			*sep = '\0';
		}
		else
			str = sep;	/* no more commands for next pass */

		/* find macros in this token and replace them */
		process_macros (token, finaltoken);

		/* Extract arguments */
		if ((argc = parse_line (finaltoken, argv)) == 0) {
			rc = -1;	/* no command at all */
			continue;
		}

		/* Look up command in command table */
		if ((cmdtp = find_cmd(argv[0])) == NULL) {
			printk ("Unknown command '%s' - try 'help'\n", argv[0]);
			rc = -1;	/* give up after bad command */
			continue;
		}

		/* found - check max args */
		if (argc > cmdtp->maxargs) {
			printk ("Usage:\n%s\n", cmdtp->usage);
			rc = -1;
			continue;
		}

		/* OK - call function to do the command */
		if ((cmdtp->cmd) (cmdtp, flag, argc, argv) != 0) {
			rc = -1;
		}

		repeatable &= cmdtp->repeatable;

		/* Did the user stop this? */
		if (had_ctrlc ())
			return 0;	/* if stopped then not repeatable */
	}

	return rc ? rc : repeatable;
}






/*
 * Use puts() instead of printf() to avoid printf buffer overflow
 * for long help messages
 */
int do_help (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int i;
	int rcode = 0;

	if (argc == 1) {	/*show list of commands */

		int cmd_items = &__u_boot_cmd_end -
				&__u_boot_cmd_start;	/* pointer arith! */
		cmd_tbl_t *cmd_array[cmd_items];
		int i, j, swaps;

		/* Make array of commands from .uboot_cmd section */
		cmdtp = &__u_boot_cmd_start;
		for (i = 0; i < cmd_items; i++) {
			cmd_array[i] = cmdtp++;
		}

		/* Sort command list (trivial bubble sort) */
		for (i = cmd_items - 1; i > 0; --i) {
			swaps = 0;
			for (j = 0; j < i; ++j) {
				if (strcmp (cmd_array[j]->name,
					    cmd_array[j + 1]->name) > 0) {
					cmd_tbl_t *tmp;
					tmp = cmd_array[j];
					cmd_array[j] = cmd_array[j + 1];
					cmd_array[j + 1] = tmp;
					++swaps;
				}
			}
			if (!swaps)
				break;
		}

		/* print short help (usage) */
		for (i = 0; i < cmd_items; i++) {
			const char *usage = cmd_array[i]->usage;

			/* allow user abort */
			if (ctrlc ())
				return 1;

			if (usage == NULL)
				continue;
			puts (usage);
		}
		return 0;
	}
	/*
	 * command help (long version)
	 */
	for (i = 1; i < argc; ++i) {
		if ((cmdtp = find_cmd (argv[i])) != NULL) {
			/* found - print (long) help info */
			puts (cmdtp->name);
			putc (' ');
			if (cmdtp->help) {
				puts (cmdtp->help);
			} else {
				puts ("- No help available.\n");
				rcode = 1;
			}
			putc ('\n');
		} else {
			printk ("Unknown command '%s' - try 'help'"
				" without arguments for list of all"
				" known commands\n\n", argv[i]
					);
			rcode = 1;
		}
	}
	return rcode;
}


U_BOOT_CMD(
	help,	CFG_MAXARGS,	1,	do_help,
 	"help    - print online help\n",
 	"[command ...]\n"
 	"    - show help information (for 'command')\n"
 	"'help' prints online help for the monitor commands.\n\n"
 	"Without arguments, it prints a short usage message for all commands.\n\n"
 	"To get detailed help information for specific commands you can type\n"
  "'help' with one or more command names as arguments.\n"
);

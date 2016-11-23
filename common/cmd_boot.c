/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>

#if defined(CONFIG_I386) || defined(CONFIG_MIPS)
DECLARE_GLOBAL_DATA_PTR;
#endif

int do_go (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr, rc;
	int     rcode = 0;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	addr = simple_strtoul(argv[1], NULL, 16);

	printf ("## Starting application at 0x%08lX ...\n", addr);

	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
#if defined(CONFIG_I386)
	/*
	 * x86 does not use a dedicated register to pass the pointer
	 * to the global_data
	 */
	argv[0] = (char *)gd;
#endif
#if !defined(CONFIG_NIOS)
	if (argc > 2 && argv[2][0] == 'b') {
		printf ("## Board info at 0x%08lX ...\n", gd->bd);
		rc = ((ulong (*)(int, int, int, int))addr)(gd->bd, 0, 0, 0);
	} else {
		rc = ((ulong (*)(int, char *[]))addr) (--argc, &argv[1]);
	}
#else
	/*
	 * Nios function pointers are address >> 1
	 */
	rc = ((ulong (*)(int, char *[]))(addr>>1)) (--argc, &argv[1]);
#endif
	if (rc != 0) rcode = 1;

	printf ("## Application terminated, rc = 0x%lX\n", rc);
	return rcode;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	go, CFG_MAXARGS, 1,	do_go,
	"go      - start application at address 'addr'\n",
	"addr [arg ...]\n    - start application at address 'addr'\n"
	"      passing 'arg' as arguments\n"
);

extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

U_BOOT_CMD(
	reset, 1, 0,	do_reset,
	"reset   - Perform RESET of the CPU\n",
	NULL
);

#ifdef CONFIG_BUFFALO

#ifdef CONFIG_STATUS_LED
#include <status_led.h>
#endif

int do_led_blink (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
//	ulong	addr, rc;
//	int     rcode = 0;

	int status = STATUS_LED_ON;

	if(argc == 2 && strcmp(argv[1], "0")==0){
		status = STATUS_LED_OFF;
	}

	printf ("## LED TEST blink(%d)\n",status);

	status_led_blink_set(STATUS_LED_ROUTER, status);
	status_led_blink_set(STATUS_LED_DIAG, status);
	status_led_blink_set(STATUS_LED_SECURITY, status);

	return 0;
}

int do_led_on (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
//	ulong	addr, rc;
//	int     rcode = 0;

	printf ("## LED TEST on\n");

	if(argc == 2 && argv[1][0]>='0' && argv[1][0]<'9'){
		status_led_set(argv[1][0] - '0', STATUS_LED_ON);
	}else{
		status_led_set(STATUS_LED_ROUTER, STATUS_LED_ON);
		status_led_set(STATUS_LED_DIAG, STATUS_LED_ON);
		status_led_set(STATUS_LED_SECURITY, STATUS_LED_ON);
	}

	return 0;
}

int do_led_off (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
//	ulong	addr, rc;
//	int     rcode = 0;

	printf ("## LED TEST off\n");

	if(argc == 2 && argv[1][0]>='0' && argv[1][0]<'9'){
		status_led_set(argv[1][0] - '0', STATUS_LED_OFF);
	}else{
		status_led_set(STATUS_LED_ROUTER, STATUS_LED_OFF);
		status_led_set(STATUS_LED_DIAG, STATUS_LED_OFF);
		status_led_set(STATUS_LED_SECURITY, STATUS_LED_OFF);
	}

	return 0;
}

int do_led_toggle (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
//	ulong	addr, rc;
//	int     rcode = 0;

	if(argc == 3 && argv[1][0]>='0' && argv[1][0]<'9' && argv[2][0]>'0' && argv[2][0]<='9'){
		printf ("## LED TEST blink(%d,%d)\n", argv[1][0] - '0',argv[2][0]-'0');
		status_led_blink_num_set( argv[1][0] - '0', argv[2][0]-'0');
	}else{
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	ledb, 2, 1,	do_led_blink,
	"ledb    - LED test blink\n",
	NULL
);
U_BOOT_CMD(
	ledon, 2, 1,	do_led_on,
	"ledon   - LED test on\n",
	NULL
);
U_BOOT_CMD(
	ledoff, 2, 1,	do_led_off,
	"ledoff  - LED test off\n",
	NULL
);
U_BOOT_CMD(
	ledt, 3, 1,	do_led_toggle,
	"ledt    - LED test toggle\n",
	NULL
);

int do_set_inspection (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
//	ulong	addr, rc;
//	int     rcode = 0;

	if(argc == 2 && argv[1][0]>='0' && argv[1][0]<='1') {
		setenv("inspection", (argv[1][0]=='1')?"1":NULL);
		if (argv[1][0]=='1')
			setenv("buf_crc", NULL);
		saveenv();
	}else{
		return 1;
	}

	return 0;
}
U_BOOT_CMD(
	set_inspection, 2, 1,	do_set_inspection,
	"set_inspection - set/unset inspection mode\n",
	" flag\n"
	"    - set flag(0/1)\n"
);



#endif /*CONFIG_BUFFALO*/


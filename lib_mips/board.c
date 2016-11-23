/*
 * (C) Copyright 2003
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

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <devices.h>
#include <version.h>
#include <net.h>
#include <environment.h>

#ifdef	CONFIG_BUFFALO
#if		defined(RT2880_ASIC_BOARD) || defined(RT2883_ASIC_BOARD) || defined(RT3052_ASIC_BOARD)
#include <asm/mipsregs.h>
#include <rt_mmap.h>
#endif	//RALINK
#ifdef	CONFIG_AR9100
#include <asm/addrspace.h>
#include "ar7100_soc.h"
#endif	//CONFIG_AR9100
#ifdef	CONFIG_AR7240
#include <asm/addrspace.h>
#include "ar7240_soc.h"
#endif	//CONFIG_AR9100

#ifdef	CONFIG_STATUS_LED
#include <status_led.h>
#endif	//CONFIG_STATUS_LED

static	int		buffalo_gpio_init(void);
static	int		buffalo_usb_init(void);
#ifdef	BUFFALO_MEMTEST_EARLY
static	int		buffalo_dram_test(void);
static	int		buffalo_mtest_early(gd_t *gd);
#endif	//BUFFALO_MEMTEST_EARLY
#ifdef	BUFFALO_MEMTEST_ADVANCED
static	int		buffalo_mtest_short(gd_t *gd);
static int		buffalo_mtest_mode(unsigned long test_mode, unsigned long test_wait, unsigned long start_offset, unsigned long test_size, unsigned long test_pattern);
#endif	//BUFFALO_MEMTEST_ADVANCED
static	int		buffalo_mtest(gd_t *gd);
static	void	buffalo_env_check(void);
static	void	buffalo_eth_init(bd_t *bd);
static	void	buffalo_bootcmd(void);
#endif	//CONFIG_BUFFALO

DECLARE_GLOBAL_DATA_PTR;

#if ( ((CFG_ENV_ADDR+CFG_ENV_SIZE) < CFG_MONITOR_BASE) || \
      (CFG_ENV_ADDR >= (CFG_MONITOR_BASE + CFG_MONITOR_LEN)) ) || \
    defined(CFG_ENV_IS_IN_NVRAM)
#define	TOTAL_MALLOC_LEN	(CFG_MALLOC_LEN + CFG_ENV_SIZE)
#else
#define	TOTAL_MALLOC_LEN	CFG_MALLOC_LEN
#endif

#undef DEBUG

extern int timer_init(void);

extern int incaip_set_cpuclk(void);

extern ulong uboot_end_data;
extern ulong uboot_end;

ulong monitor_flash_len;

const char version_string[] =
	U_BOOT_VERSION" (" __DATE__ " - " __TIME__ ")";

static char *failed = "*** failed ***\n";

/*
 * Begin and End of memory area for malloc(), and current "brk"
 */
static ulong mem_malloc_start;
static ulong mem_malloc_end;
static ulong mem_malloc_brk;


/*
 * The Malloc area is immediately below the monitor copy in DRAM
 */
static void mem_malloc_init (void)
{
	ulong dest_addr = CFG_MONITOR_BASE + gd->reloc_off;

	mem_malloc_end = dest_addr;
	mem_malloc_start = dest_addr - TOTAL_MALLOC_LEN;
	mem_malloc_brk = mem_malloc_start;

	memset ((void *) mem_malloc_start,
		0,
		mem_malloc_end - mem_malloc_start);
}

void *sbrk (ptrdiff_t increment)
{
	ulong old = mem_malloc_brk;
	ulong new = old + increment;

	if ((new < mem_malloc_start) || (new > mem_malloc_end)) {
		return (NULL);
	}
	mem_malloc_brk = new;
	return ((void *) old);
}


static int init_func_ram (void)
{
#ifdef	CONFIG_BOARD_TYPES
	int board_type = gd->board_type;
#else
	int board_type = 0;	/* use dummy arg */
#endif

	if ((gd->ram_size = initdram (board_type)) > 0) {
		print_size (gd->ram_size, "\n");
		return (0);
	}
	puts (failed);
	return (1);
}

static int display_banner(void)
{
#ifdef	CONFIG_BUFFALO
	puts ("\r\n");
	puts ("\r\n");
	puts ("BUFFALO U-BOOT Ver " MK_STR(BUFFALO_BIN_VER) "\n");
#endif	//CONFIG_BUFFALO

	return (0);
}

static void display_flash_config(ulong size)
{
	puts ("Flash: ");
	print_size (size, "\n");
}


static int init_baudrate (void)
{
	char tmp[64];	/* long enough for environment variables */
	int i = getenv_r ("baudrate", tmp, sizeof (tmp));

	gd->baudrate = (i > 0)
			? (int) simple_strtoul (tmp, NULL, 10)
			: CONFIG_BAUDRATE;

	return (0);
}


/*
 * Breath some life into the board...
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependend #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

init_fnc_t *init_sequence[] = {
	timer_init,
	env_init,		/* initialize environment */
#ifdef CONFIG_INCA_IP
	incaip_set_cpuclk,	/* set cpu clock according to environment variable */
#endif
#ifdef	BUFFALO_MEMTEST_EARLY
	buffalo_gpio_init,
	buffalo_usb_init,
#endif	//BUFFALO_MEMTEST_EARLY
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,
	display_banner,		/* say that we are here */
#ifdef	CONFIG_BUFFALO
#endif	//CONFIG_BUFFALO
	checkboard,
	init_func_ram,
#ifdef	BUFFALO_MEMTEST_EARLY
	buffalo_dram_test,
#endif	//BUFFALO_MEMTEST_EARLY
	NULL,
};


void board_init_f(ulong bootflag)
{
	gd_t gd_data, *id;
	bd_t *bd;
	init_fnc_t **init_fnc_ptr;
	ulong addr, addr_sp, len = (ulong)&uboot_end - CFG_MONITOR_BASE;
	ulong *s;
#ifdef CONFIG_PURPLE
	void copy_code (ulong);
#endif

	/* Pointer is writable since we allocated a register for it.
	 */
	gd = &gd_data;
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

	memset ((void *)gd, 0, sizeof (gd_t));

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}

	/*
	 * Now that we have DRAM mapped and working, we can
	 * relocate the code and continue running from DRAM.
	 */
	addr = CFG_SDRAM_BASE + gd->ram_size;

	/* We can reserve some RAM "on top" here.
	 */

	/* round down to next 4 kB limit.
	 */
	addr &= ~(4096 - 1);
	debug ("Top of RAM usable for U-Boot at: %08lx\n", addr);

	/* Reserve memory for U-Boot code, data & bss
	 * round down to next 16 kB limit
	 */
	addr -= len;
	addr &= ~(16 * 1024 - 1);

	debug ("Reserving %ldk for U-Boot at: %08lx\n", len >> 10, addr);

	 /* Reserve memory for malloc() arena.
	 */
	addr_sp = addr - TOTAL_MALLOC_LEN;
	debug ("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, addr_sp);

	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof(bd_t);
	bd = (bd_t *)addr_sp;
	gd->bd = bd;
	debug ("Reserving %d Bytes for Board Info at: %08lx\n",
			sizeof(bd_t), addr_sp);

	addr_sp -= sizeof(gd_t);
	id = (gd_t *)addr_sp;
	debug ("Reserving %d Bytes for Global Data at: %08lx\n",
			sizeof (gd_t), addr_sp);

 	/* Reserve memory for boot params.
	 */
	addr_sp -= CFG_BOOTPARAMS_LEN;
	bd->bi_boot_params = addr_sp;
	debug ("Reserving %dk for boot params() at: %08lx\n",
			CFG_BOOTPARAMS_LEN >> 10, addr_sp);

	/*
	 * Finally, we set up a new (bigger) stack.
	 *
	 * Leave some safety gap for SP, force alignment on 16 byte boundary
	 * Clear initial stack frame
	 */
	addr_sp -= 16;
	addr_sp &= ~0xF;
	s = (ulong *)addr_sp;
	*s-- = 0;
	*s-- = 0;
	addr_sp = (ulong)s;
	debug ("Stack Pointer at: %08lx\n", addr_sp);

	/*
	 * Save local variables to board info struct
	 */
	bd->bi_memstart	= CFG_SDRAM_BASE;	/* start of  DRAM memory */
	bd->bi_memsize	= gd->ram_size;		/* size  of  DRAM memory in bytes */
	bd->bi_baudrate	= gd->baudrate;		/* Console Baudrate */

	memcpy (id, (void *)gd, sizeof (gd_t));

	/* On the purple board we copy the code in a special way
	 * in order to solve flash problems
	 */
#ifdef CONFIG_PURPLE
	copy_code(addr);
#endif

	relocate_code (addr_sp, id, addr);

	/* NOTREACHED - relocate_code() does not return */
}
/************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */

void board_init_r (gd_t *id, ulong dest_addr)
{
	cmd_tbl_t *cmdtp;
	ulong size;
	extern void malloc_bin_reloc (void);
#ifndef CFG_ENV_IS_NOWHERE
	extern char * env_name_spec;
#endif
	char *s, *e;
	bd_t *bd;
	int i;

	gd = id;
	gd->flags |= GD_FLG_RELOC;	/* tell others: relocation done */

	debug ("Now running in RAM - U-Boot at: %08lx\n", dest_addr);

	gd->reloc_off = dest_addr - CFG_MONITOR_BASE;

	monitor_flash_len = (ulong)&uboot_end_data - dest_addr;

	/*
	 * We have to relocate the command table manually
	 */
 	for (cmdtp = &__u_boot_cmd_start; cmdtp !=  &__u_boot_cmd_end; cmdtp++) {
		ulong addr;

		addr = (ulong) (cmdtp->cmd) + gd->reloc_off;
#if 0
		printf ("Command \"%s\": 0x%08lx => 0x%08lx\n",
				cmdtp->name, (ulong) (cmdtp->cmd), addr);
#endif
		cmdtp->cmd =
			(int (*)(struct cmd_tbl_s *, int, int, char *[]))addr;

		addr = (ulong)(cmdtp->name) + gd->reloc_off;
		cmdtp->name = (char *)addr;

		if (cmdtp->usage) {
			addr = (ulong)(cmdtp->usage) + gd->reloc_off;
			cmdtp->usage = (char *)addr;
		}
#ifdef	CFG_LONGHELP
		if (cmdtp->help) {
			addr = (ulong)(cmdtp->help) + gd->reloc_off;
			cmdtp->help = (char *)addr;
		}
#endif
	}
	/* there are some other pointer constants we must deal with */
#ifndef CFG_ENV_IS_NOWHERE
	env_name_spec += gd->reloc_off;
#endif

	/* configure available FLASH banks */
	size = flash_init();
	display_flash_config (size);

	bd = gd->bd;
	bd->bi_flashstart = CFG_FLASH_BASE;
	bd->bi_flashsize = size;
#if CFG_MONITOR_BASE == CFG_FLASH_BASE
	bd->bi_flashoffset = monitor_flash_len;	/* reserved area for U-Boot */
#else
	bd->bi_flashoffset = 0;
#endif

	/* initialize malloc() area */
	mem_malloc_init();
	malloc_bin_reloc();

	/* relocate environment function pointers etc. */
	env_relocate();

	/* board MAC address */
	s = getenv ("ethaddr");
	for (i = 0; i < 6; ++i) {
		bd->bi_enetaddr[i] = s ? simple_strtoul (s, &e, 16) : 0;
		if (s)
			s = (*e) ? e + 1 : e;
	}

	/* IP Address */
	bd->bi_ip_addr = getenv_IPaddr("ipaddr");

#if defined(CONFIG_PCI)
	/*
	 * Do pci configuration
	 */
	pci_init();
#endif

/** leave this here (after malloc(), environment and PCI are working) **/
	/* Initialize devices */
	devices_init ();

	jumptable_init ();

	/* Initialize the console (after the relocation and devices init) */
	console_init_r ();
/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

#ifdef	CONFIG_BUFFALO
	do	{
		disable_ctrlc(1);

#ifndef	BUFFALO_MEMTEST_EARLY
		buffalo_gpio_init();
#endif	//BUFFALO_MEMTEST_EARLY

		do	{
#ifdef	BUFFALO_MEMTEST_ADVANCED
#ifndef	BUFFALO_MEMTEST_EARLY
			if ((s = getenv ("inspection")) && simple_strtoul(s,NULL,0)>0)
			{
				puts ("Memory Test (address line)\n");
				if (buffalo_mtest_short(gd)) {
					status_led_blink_num_set(STATUS_LED_DIAG, 1);
					break;
				}
			}
#endif	//BUFFALO_MEMTEST_EARLY
#endif	//BUFFALO_MEMTEST_ADVANCED
			puts ("Memory Test\n");
			if(buffalo_mtest(gd)){
				status_led_blink_num_set(STATUS_LED_DIAG, 1);
				break;
			}
			puts ("Memory Test OK\n");
		} while(0);


		buffalo_env_check();

	} while(0);
#endif	//CONFIG_BUFFALO

	/* Initialize from environment */
	if ((s = getenv ("loadaddr")) != NULL) {
		load_addr = simple_strtoul (s, NULL, 16);
	}
#if (CONFIG_COMMANDS & CFG_CMD_NET)
	if ((s = getenv ("bootfile")) != NULL) {
		copy_filename (BootFile, s, sizeof (BootFile));
	}
#endif	/* CFG_CMD_NET */

#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r ();
#endif

#ifndef	CONFIG_BUFFALO
#if (CONFIG_COMMANDS & CFG_CMD_NET)
#if defined(CONFIG_NET_MULTI)
	puts ("Net:   ");
#endif
	eth_initialize(gd->bd);
#endif
#endif	//CONFIG_BUFFALO


#ifdef	CONFIG_BUFFALO
	do	{
		buffalo_eth_init(gd->bd);

		disable_ctrlc(0);
		ctrlc();

		if (had_ctrlc())	break;

		buffalo_bootcmd();

	} while(0);
#endif	//CONFIG_BUFFALO

	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		main_loop ();
	}

	/* NOTREACHED - no way out of command loop except booting */
}

void hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	for (;;);
}

#ifdef CONFIG_BUFFALO

#if		defined(CONFIG_AR9100) || defined(CONFIG_AR7100) || defined(CONFIG_AR7240)
static	void	buffalo_gpio_init_ATHEROS(void)
{
	u32		mask	= 0;

	mask	= 0
#if		STATUS_LED_BITS > 0
			|	STATUS_LED_BIT
#endif	//STATUS_LED_BITS > 0
#if		STATUS_LED_BITS > 1
			|	STATUS_LED_BIT1
#endif	//STATUS_LED_BITS > 1
#if		STATUS_LED_BITS > 2
			|	STATUS_LED_BIT2
#endif	//STATUS_LED_BITS > 2
#if		STATUS_LED_BITS > 3
			|	STATUS_LED_BIT3
#endif	//STATUS_LED_BITS > 3
#if		STATUS_LED_BITS > 4
			|	STATUS_LED_BIT4
#endif	//STATUS_LED_BITS > 4
#if		STATUS_LED_BITS > 5
			|	STATUS_LED_BIT5
#endif	//STATUS_LED_BITS > 5
			;

#if	defined(CONFIG_AR9100) || defined(CONFIG_AR7100)
	//	ap93 init gpio
	ar7100_reg_rmw_set(AR7100_GPIO_OE, mask);
#endif	//AR7100
#if	defined(CONFIG_AR7240)
	ar7240_gpio_config();
	ar7240_reg_wr(AR7240_GPIO_OE, CFG_BUFFALO_GPIO_OUT_MASK);
	udelay(10);
	ar7240_reg_wr(AR7240_GPIO_CLEAR, CFG_BUFFALO_GPIO_OUT_MASK);
	udelay(10);
	ar7240_reg_wr(AR7240_GPIO_INT_ENABLE, 0);

	//
	//	GPIO for AR9283
	//
	{
		extern	void	ar7240_pci_ar9283_gpio_init();
		ar7240_pci_ar9283_gpio_init();
	}
#endif	//AR7240

#ifdef	CONFIG_WZR_HP_G300NH
	status_led_set(STATUS_LED_ROUTER, STATUS_LED_ON);
	status_led_set(STATUS_LED_SECURITY, STATUS_LED_ON);
	status_led_set(STATUS_LED_DIAG, STATUS_LED_ON);
	status_led_set(STATUS_LED_WIRELESS, STATUS_LED_ON);
	status_led_set(STATUS_LED_USB, STATUS_LED_ON);

	//	off
	udelay (500*1000);

	status_led_set(STATUS_LED_ROUTER, STATUS_LED_OFF);
	status_led_set(STATUS_LED_SECURITY, STATUS_LED_OFF);
//	status_led_set(STATUS_LED_DIAG, STATUS_LED_OFF);
	status_led_set(STATUS_LED_WIRELESS, STATUS_LED_OFF);
	status_led_set(STATUS_LED_USB, STATUS_LED_OFF);
#endif	//CONFIG_WZR_HP_G300NH
#if	defined(CONFIG_WHR_HP_G300N) || defined(CONFIG_WHR_HP_GN) || defined(CONFIG_WHR_G301N) || defined(CONFIG_W200) || defined(CONFIG_FS_HP_G300N)
	status_led_set(STATUS_LED_ROUTER, STATUS_LED_ON);
	status_led_set(STATUS_LED_SECURITY, STATUS_LED_ON);
	status_led_set(STATUS_LED_DIAG, STATUS_LED_ON);
//	status_led_set(STATUS_LED_WIRELESS, STATUS_LED_ON);
//	status_led_set(STATUS_LED_USB, STATUS_LED_ON);

	udelay (500*1000);

	{
		extern	void	ar7240_pci_ar9283_led(int on);
		ar7240_pci_ar9283_led(0);
	}

	status_led_set(STATUS_LED_ROUTER, STATUS_LED_OFF);
	status_led_set(STATUS_LED_SECURITY, STATUS_LED_OFF);
//	status_led_set(STATUS_LED_DIAG, STATUS_LED_OFF);
//	status_led_set(STATUS_LED_WIRELESS, STATUS_LED_OFF);
//	status_led_set(STATUS_LED_USB, STATUS_LED_OFF);
#endif	//CONFIG_WZR_HP_G300NH

}
#endif	//CONFIG_AR9100 || CONFIG_AR7100 || CONFIG_AR7240

#if		defined(RT2880_ASIC_BOARD) || defined(RT2883_ASIC_BOARD) || defined(RT3052_ASIC_BOARD)
static	void	buffalo_gpio_init_RALINK(void)
{
#if		defined(RT2880_ASIC_BOARD) || defined(RT2883_ASIC_BOARD) || defined(RT3052_ASIC_BOARD)
	{
		// printf("before SYSCFG_REG=%X\n", reg);
		RT2882_REG(RT2880_SYSCFG_REG) = RT2882_REG(RT2880_SYSCFG_REG) & 0xFFFFFFBF;
		// printf("after SYSCFG_REG=%X\n", RT2882_REG(RT2880_SYSCFG_REG));

#if		defined(PRODUCT_BOARD_WRTR_222N)
		RT2882_REG(RT2880_GPIOMODE_REG) = 0x7; /*UARTF_GPIO_MODE(bit1)=GPIOMODE(1),SPI_GPIO_MODE(bit1)=GPIOMODE(1),L2C_GPIO_MODE(bit0)=GPIOMODE(1)*/
#elif	defined(PRODUCT_BOARD_WRTR_236G)
		RT2882_REG(RT2880_GPIOMODE_REG) = 0x0dc; /*MDIO_GPIO_MODE(bit7)=GPIOMODE(1),JTAG_GPIO_MODE(bit6)=GPIOMODE(1),UARTF_GPIO_MODE(bit2-4)=GPIOMODE(7)*/
#elif	defined(PRODUCT_BOARD_WRTR_237G)
		RT2882_REG(RT2880_GPIOMODE_REG) = 0x21e; /*RGMII_GPIO_MODE(bit9)=GPIOMODE(1),UARTF_GPIO_MODE(bit2-4)=GPIOMODE(7),SPI_GPIO_MODE(bit1)=GPIOMODE(1)*/
#else
		RT2882_REG(RT2880_GPIOMODE_REG) = 0x6; /* UARTF_GPIO_MODE(bit1)=GPIO MODE(1), SPI_GPIO_MODE(bit1)=GPIO MODE(1) */
#endif

		RT2882_REG(RT2880_PIODATA_REG)
			= (1<<GPIO_CONTROL_RESET);

#ifdef	PRODUCT_BOARD_WACR_130N
		/* GPIOの方向(In, Out)。 LEDとPOWER/RESET CtlはOut(1)。それ以外はIn(0) */
		RT2882_REG(RT2880_PIODIR_REG)
			= (1<<GPIO_CONTROL_POWER)|(1<<GPIO_LED_POWER_RED)|(1<<GPIO_LED_POWER_BLUE)|(1<<GPIO_LED_SECURITY)|(1<<GPIO_CONTROL_RESET);

		status_led_set(STATUS_LED_BOOT, STATUS_LED_ON);
		status_led_set(STATUS_LED_DIAG, STATUS_LED_OFF);
		status_led_set(STATUS_LED_SECURITY, STATUS_LED_OFF);

#endif	//PRODUCT_BOARD_WACR_130N
#ifdef	PRODUCT_BOARD_WRTR_222N
		/* GPIOの方向(In, Out)。 LEDとPOWER/RESET CtlはOut(1)。それ以外はIn(0) */
		RT2882_REG(RT2880_PIODIR_REG)
			= (1<<GPIO_LED_ROUTER)|(1<<GPIO_LED_SECURITY_G)|(1<<GPIO_LED_DIAG)|(1<<GPIO_LED_SECURITY_N)|(1<<GPIO_CONTROL_RESET);
		status_led_set(STATUS_LED_ROUTER, STATUS_LED_OFF);
		status_led_set(STATUS_LED_SECURITY_G, STATUS_LED_OFF);
		status_led_set(STATUS_LED_DIAG, STATUS_LED_ON);
		status_led_set(STATUS_LED_SECURITY_N, STATUS_LED_ON /*FF*/);
// { add BUFFALO 080218
		do_start_vlan(NULL, 0, 0, NULL);
// } add BUFFALO 080218
#endif	//PRODUCT_BOARD_WRTR_222N
#ifdef	PRODUCT_BOARD_WRTR_236G
		/* GPIOの方向(In, Out)。 LEDとPOWER/RESET CtlはOut(1)。それ以外はIn(0) */
		RT2882_REG(RT2880_PIODIR_REG)
			= (1<<GPIO_LED_ROUTER)|(1<<GPIO_LED_DIAG)|(1<<GPIO_LED_SECURITY)|(1<<GPIO_LED_USB)|(1<<GPIO_CONTROL_RESET);
		status_led_set(STATUS_LED_ROUTER, STATUS_LED_ON);
		status_led_set(STATUS_LED_SECURITY, STATUS_LED_ON);
		status_led_set(STATUS_LED_USB, STATUS_LED_ON);
		status_led_set(STATUS_LED_DIAG, STATUS_LED_ON);

		//	Wireless LED
		RT2882_REG(RT2880_REG_11N_LEDCFG)	= (RT2882_REG(RT2880_REG_11N_LEDCFG) & ~RT2880_MASK_11N_LED) | RT2880_VAL_11N_LED_ON;
		//	Wired LED
//		RT2882_REG(RT2880_REG_ETH_P0_LED)	= RT2880_VAL_ETH_LED_ON;
//		RT2882_REG(RT2880_REG_ETH_P1_LED)	= RT2880_VAL_ETH_LED_ON;
//		RT2882_REG(RT2880_REG_ETH_P2_LED)	= RT2880_VAL_ETH_LED_ON;
//		RT2882_REG(RT2880_REG_ETH_P3_LED)	= RT2880_VAL_ETH_LED_ON;
//		RT2882_REG(RT2880_REG_ETH_P4_LED)	= RT2880_VAL_ETH_LED_ON;

		//	off
		udelay (500*1000);

		status_led_set(STATUS_LED_ROUTER, STATUS_LED_OFF);
		status_led_set(STATUS_LED_SECURITY, STATUS_LED_OFF);
		status_led_set(STATUS_LED_USB, STATUS_LED_OFF);
		status_led_set(STATUS_LED_DIAG, STATUS_LED_OFF);

		//	Wireless LED
		RT2882_REG(RT2880_REG_11N_LEDCFG)	= (RT2882_REG(RT2880_REG_11N_LEDCFG) & ~RT2880_MASK_11N_LED) | RT2880_VAL_11N_LED_OFF;
		//	Wired LED
//		RT2882_REG(RT2880_REG_ETH_P0_LED)	= RT2880_VAL_ETH_LED_L_A;
//		RT2882_REG(RT2880_REG_ETH_P1_LED)	= RT2880_VAL_ETH_LED_L_A;
//		RT2882_REG(RT2880_REG_ETH_P2_LED)	= RT2880_VAL_ETH_LED_L_A;
//		RT2882_REG(RT2880_REG_ETH_P3_LED)	= RT2880_VAL_ETH_LED_L_A;
//		RT2882_REG(RT2880_REG_ETH_P4_LED)	= RT2880_VAL_ETH_LED_L_A;
#endif	//PRODUCT_BOARD_WRTR_236G
#ifdef	PRODUCT_BOARD_WRTR_237G
		RT2882_REG(RT2880_PIODIR_REG)
			= (1<<GPIO_LED_ROUTER)|(1<<GPIO_LED_DIAG)|(1<<GPIO_LED_SECURITY)|(1<<GPIO_CONTROL_RESET);
		status_led_set(STATUS_LED_ROUTER, STATUS_LED_ON);
		status_led_set(STATUS_LED_SECURITY, STATUS_LED_ON);
		status_led_set(STATUS_LED_DIAG, STATUS_LED_ON);

		//	Wireless LED
		RT2882_REG(RT2880_REG_11N_LEDCFG)	= (RT2882_REG(RT2880_REG_11N_LEDCFG) & ~RT2880_MASK_11N_LED) | RT2880_VAL_11N_LED_ON;
		//	Wired LED
		RT2882_REG(RT2880_REG_ETH_P0_LED)	= RT2880_VAL_ETH_LED_ON;
		RT2882_REG(RT2880_REG_ETH_P1_LED)	= RT2880_VAL_ETH_LED_ON;
		RT2882_REG(RT2880_REG_ETH_P2_LED)	= RT2880_VAL_ETH_LED_ON;
		RT2882_REG(RT2880_REG_ETH_P3_LED)	= RT2880_VAL_ETH_LED_ON;
		RT2882_REG(RT2880_REG_ETH_P4_LED)	= RT2880_VAL_ETH_LED_ON;

		//	off
		udelay (500*1000);

		status_led_set(STATUS_LED_ROUTER, STATUS_LED_OFF);
		status_led_set(STATUS_LED_SECURITY, STATUS_LED_OFF);
		status_led_set(STATUS_LED_DIAG, STATUS_LED_OFF);

		//	Wireless LED
		RT2882_REG(RT2880_REG_11N_LEDCFG)	= (RT2882_REG(RT2880_REG_11N_LEDCFG) & ~RT2880_MASK_11N_LED) | RT2880_VAL_11N_LED_OFF;
		//	Wired LED
		RT2882_REG(RT2880_REG_ETH_P0_LED)	= RT2880_VAL_ETH_LED_L_A;
		RT2882_REG(RT2880_REG_ETH_P1_LED)	= RT2880_VAL_ETH_LED_L_A;
		RT2882_REG(RT2880_REG_ETH_P2_LED)	= RT2880_VAL_ETH_LED_L_A;
		RT2882_REG(RT2880_REG_ETH_P3_LED)	= RT2880_VAL_ETH_LED_L_A;
		RT2882_REG(RT2880_REG_ETH_P4_LED)	= RT2880_VAL_ETH_LED_L_A;
#endif	//PRODUCT_BOARD_WRTR_237G

	}
#endif	//defined(RT2880_ASIC_BOARD) || defined(RT2883_ASIC_BOARD) || defined(RT3052_ASIC_BOARD)
}
#endif	//RALINK

static	int		buffalo_gpio_init(void)
{
#if		defined(CONFIG_AR9100) || defined(CONFIG_AR7100) || defined(CONFIG_AR7240)
	buffalo_gpio_init_ATHEROS();
#endif	//ATHEROS
#if		defined(RT2880_ASIC_BOARD) || defined(RT2883_ASIC_BOARD) || defined(RT3052_ASIC_BOARD)
	buffalo_gpio_init_RALINK();
#endif	//RALINK
	return	0;
}

static	int		buffalo_usb_init(void)
{
	int	i;

#if		defined(CONFIG_AR7240)
	ar7240_usb_initial_config();
#endif	//ATHEROS

	return	0;
}

#ifdef	BUFFALO_MEMTEST_ADVANCED
#ifdef	BUFFALO_MEMTEST_EARLY
static	int	buffalo_dram_test(void)
{
	int     	rcode = 1;
	char		*s;

	//	BUFFALO mtest mode
	do	{
		unsigned int slide = ar7240_reg_rd(AR7240_GPIO_IN) & BUFFALO_MTEST_SLIDE_MASK;
		if (0
		||	slide==BUFFALO_MTEST_SLIDE_TEST
#ifdef	BUFFALO_MTEST_SLIDE_TAPfix
		||	slide==BUFFALO_MTEST_SLIDE_TAPfix
#endif	//BUFFALO_MTEST_SLIDE_TAPfix
		) {
			volatile BUFFALO_MTEST_PARAM *pprm	= (volatile BUFFALO_MTEST_PARAM *)BUFFALO_MTEST_PARAM_ADDR;
			if (pprm->magic == BUFFALO_MTEST_PARAM_MAGIC && pprm->version == BUFFALO_MTEST_PARAM_VER) {
				//	test
				buffalo_mtest_mode(pprm->test_mode, pprm->test_wait, pprm->test_offset, pprm->test_size, pprm->test_pattern);
			}
		}
	} while(0);

	do	{
		puts ("Memory Test (address line)\n");
		if (buffalo_mtest_short(gd)) {
			status_led_blink_num_set(STATUS_LED_DIAG, 1);
			break;
		}

		if ((s = getenv ("inspection")) && simple_strtoul(s,NULL,0)==1) {
			puts ("Memory Test (bottom 1MB)\n");
			if(buffalo_mtest_early(gd)){
				status_led_blink_num_set(STATUS_LED_DIAG, 1);
				break;
			}
			puts ("Early Memory Test OK\n");
		}
		rcode	= 0;
	} while(0);

	return	rcode;
}
#endif	//BUFFALO_MEMTEST_EARLY

static int buffalo_mtest_mode(unsigned long test_mode, unsigned long test_wait, unsigned long start_offset, unsigned long test_size, unsigned long test_pattern)
{
	uint32_t			slide = ar7240_reg_rd(AR7240_GPIO_IN) & BUFFALO_MTEST_SLIDE_MASK;
	uint32_t			*start, *end;
	volatile uint32_t	*addr;
	int					pat;
	unsigned long		loop	= 1;


	static uint32_t pattern[]={ 0x5A5AA5A5, 0xA5A55A5A, 0x55555555, 0xAAAAAAAA , 0xFF00FF00, 0x00FF00FF };

	if (test_size==0)	test_size	= gd->ram_size;
	if (gd->ram_size < (start_offset + test_size))	test_size	= gd->ram_size - start_offset;


	if (!test_mode)	return	0;


	printf("Memory Test start(%08lX) end(%08lX) size(%08lX)\n",start_offset, start_offset + test_size, test_size);
	printf("  write %s / read %s\n", (test_mode & BUFFALO_MTEST_MODE_WC) ? "cache" : "uncache", (test_mode & BUFFALO_MTEST_MODE_RC) ? "cache" : "uncache");
	printf("------------------------------------------------------------------\n");

	//	         1         2         3         4         5         6         7         8
	//	12345678901234567890123456789012345678901234567890123456789012345678901234567890
	//	    Pattern xxxxxxxx  Writing...
	//	                                  Reading...
	//		    read fail offset xxxxxxxx, pattern:xxxxxxxx, readback:xxxxxxxx
	//		loop xxxxxxxx finished.
	for ( ; slide==(ar7240_reg_rd(AR7240_GPIO_IN) & BUFFALO_MTEST_SLIDE_MASK) ; loop++) {
		register	uint32_t			val;
		register	uint32_t			readback;

		for (pat=0 ; pat<sizeof(pattern)/sizeof(pattern[0]) && slide==(ar7240_reg_rd(AR7240_GPIO_IN) & BUFFALO_MTEST_SLIDE_MASK) ; pat++) {

			if (test_pattern) {
				val	= test_pattern;
				pat	= sizeof(pattern)/sizeof(pattern[0]);
			} else {
				val	= pattern[pat];
			}

			//
			//	write phase
			//
			printf ("\r    Pattern %08lX  Writing...", val);
			puts ("            ");
			puts ("\b\b\b\b\b\b\b\b\b\b");

			if (test_mode & BUFFALO_MTEST_MODE_WC)			start	= (uint32_t *)KSEG0ADDR(start_offset);
			else											start	= (uint32_t *)KSEG1ADDR(start_offset);
															end		= (uint32_t *)(((uint32_t)start) + test_size);

			for (addr=start ; addr<end ; addr++)
				*addr = val;


			//
			//	read phase
			//
			puts ("Reading...");

			if (test_mode & BUFFALO_MTEST_MODE_RC)			start	= (uint32_t *)KSEG0ADDR(start_offset);
			else											start	= (uint32_t *)KSEG1ADDR(start_offset);
															end		= (uint32_t *)(((uint32_t)start) + test_size);

			for (addr=start ; addr<end ; addr++) {
				readback = *addr;
				if (readback != val) {
					printf("\r    read fail offset %08lX, pattern:%08lX, readback:%08lX\n", (uint32_t)addr, val, readback);
				}
			}
		}
		printf("\rloop %08lx %-8.8s.                                           \n", loop, (pat<sizeof(pattern)/sizeof(pattern[0]))?"aborted":"finished");

		if (test_wait) {
			for (val=test_wait ; val>0 ; val--) {
				udelay(1000*1000);
			}
		}
	}

	puts ("\n");
	memset((void *)KSEG0ADDR(start_offset), 0, (size_t)(test_size));

	return 0;

}

static int buffalo_mtest_short(gd_t *gd)
{
	uint32_t	*addr, *start, *end;
	uint32_t	val;
	uint32_t	readback;
	int     	rcode = 0;
	char 		*eptr;
	int		i,j,pat_num = 2;
	int		skip_size = 0x4000/sizeof(uint32_t);	/*16KB*/
	uint32_t	*dummy = NULL;
	uint32_t	len;
	uint32_t	pattern;
	uint32_t	anti_pattern;
	uint32_t	test_offset;
	uint32_t	offset;
	uint32_t	temp;

	int pat=0;
//msato debug
//	uint32_t fill_pattern[]={0x5555AAAA, 0xAAAA5555, 0x0000FFFF, 0xFFFF0000, 0x00000000};
	uint32_t fill_pattern[]={0x5555AAAA, 0xAAAA5555, 0x0000FFFF, 0xFFFF0000, 0xAAAAAAAA};

#ifndef	BUFFALO_MEMTEST_EARLY
	//	uboot running on SDRAM
	start = (void *)KSEG0ADDR(0x00000000);
//	end   = (void *)KSEG0ADDR((uint32_t)start + gd->ram_size);
	end   = (void *)KSEG0ADDR((char *)start + gd->ram_size);
	if (1) {
		printf("uboot use  %08lX - %08lX\n", gd->bd->bi_boot_params, gd->bd->bi_memstart + gd->bd->bi_memsize);
		start = (uint32_t *)KSEG0ADDR(gd->bd->bi_memstart);
		end	= (uint32_t *)KSEG0ADDR(gd->bd->bi_boot_params & (~((uint32_t)0x100000-1)));
	}
#else	//BUFFALO_MEMTEST_EARLY
	//	uboot running on ROM
	start = (void *)KSEG1ADDR(0x00000000);
	end   = (void *)KSEG1ADDR(((char *)start + gd->ram_size));
#endif	//BUFFALO_MEMTEST_EARLY




	static const uint32_t bitpattern[] = {
		0x00000001,	/* single bit */
		0x00000003,	/* two adjacent bits */
		0x00000007,	/* three adjacent bits */
		0x0000000F,	/* four adjacent bits */
		0x00000005,	/* two non-adjacent bits */
		0x00000015,	/* three non-adjacent bits */
		0x00000055,	/* four non-adjacent bits */
		0xaaaaaaaa,	/* alternating 1/0 */
	};


	printf("Memory Test start(0x%08X) end(0x%08X) size(%u)\n", start, end, gd->ram_size);


	printf("Data line test start:0x%p pattern ", start);

	addr = start;
	dummy = start+2;
	for (j = 0; j < sizeof(bitpattern)/sizeof(bitpattern[0]); j++) {
	    val = bitpattern[j];

		printf("0x%08X ", val);
		for(; val != 0; val <<= 1) {
			*addr  = val;
			*dummy  = ~val; /* clear the test data off of the bus */
			readback = *addr;
			if(readback != val) {
				printf ("FAILURE (data line): expected %08lx, actual %08lx\n", val, readback);
			}

//			debug("dataline test addr:0x%p val:0x%08X write\n", addr, val);

			*addr  = ~val;
			*dummy  = val;
			readback = *addr;
			if(readback != ~val) {
				printf ("FAILURE (data line): Is %08lx, should be %08lx\n", val, readback);
			}

//			debug("dataline test addr:0x%p val:0x%08X read\n", addr, readback);
	    }
	}
	puts("\n");

	len = ((ulong)end - (ulong)start)/sizeof(vu_long);
	pattern = (vu_long) 0xaaaaaaaa;
	anti_pattern = (vu_long) 0x55555555;

	printf("Address line test start:0x%p len:0x%x pattern 0x%08X 0x%08X  ",
		addr, ((ulong)end - (ulong)start), pattern, anti_pattern);


	/*
	 * Write the default pattern at each of the
	 * power-of-two offsets.
	 */
	for (offset = 1; offset < len; offset <<= 1) {
		start[offset] = pattern;
	}

	/*
	 * Check for address bits stuck high.
	 */
	test_offset = 0;
	start[test_offset] = anti_pattern;

	for (offset = 1; offset < len; offset <<= 1) {
		temp = start[offset];
		if (temp != pattern) {
			printf ("\nFAILURE: Address bit stuck high @ 0x%.8lx: expected 0x%.8lx, actual 0x%.8lx\n",
					(ulong)&start[offset], pattern, temp);
			return 1;
	    }
//		debug("addrline test addr:0x%p val:0x%08X read\n", &start[offset], temp);
	}
	start[test_offset] = pattern;

	/*
	 * Check for addr bits stuck low or shorted.
	 */
	for (test_offset = 1; test_offset < len; test_offset <<= 1) {
		start[test_offset] = anti_pattern;
//	    debug("addrline test addr:0x%p val:0x%08X write\n", &start[test_offset], anti_pattern);

		for (offset = 1; offset < len; offset <<= 1) {
			temp = start[offset];
			if ((temp != pattern) && (offset != test_offset)) {
			    printf ("\nFAILURE: Address bit stuck low or shorted @ 0x%.8lx: expected 0x%.8lx, actual 0x%.8lx\n",
					(ulong)&start[offset], pattern, temp);
				return 1;
			}
//			debug("addrline test addr:0x%p val:0x%08X read\n", &start[offset], temp);
		}
		start[test_offset] = pattern;
//		debug("addrline test addr:0x%p val:0x%08X write\n", &start[test_offset], pattern);
	}

	puts("\n");



	eptr = getenv("short_memcheck");
	if (!eptr || !strcmp("off", eptr)) {
		pat_num = 5;
	}
//	if(eptr)
//		printf("short_memcheck %s %d\n", eptr, pat_num);
	printf("Fill test patnum:%d \n", pat_num);


	for (pat=0;pat<pat_num;pat++) {
//		uint mask=0xfffff;
		if (ctrlc()) {
			putc ('\n');
			return 1;
		}

		printf ("fill Pattern %08lX  Writing..."
			"%12s "
			"\b\b\b\b\b\b\b\b\b\b",
			fill_pattern[pat], "");

#ifdef CONFIG_STATUS_LED
//		status_led_set(STATUS_LED_DIAG, STATUS_LED_TOGGLE);
#endif
		for (addr=start, val=fill_pattern[pat], i=0; addr<end; ) {

			if(i>=skip_size)	i=0;
//			if(((uint)addr & mask) == 0)
//				printf ("\rfill Pattern %08lX Addr(0x%qx) Writing... %d * %d i:0x%x",
//					val, (addr+i), skip_size, sizeof(addr),i);

			*(addr+i) =  val;
			addr += skip_size;
			i+=sizeof(addr);

			if (ctrlc()) {
				putc ('\n');
				return 1;
			}
		}

		puts ("Reading...");
#ifdef CONFIG_STATUS_LED
//		status_led_set(STATUS_LED_DIAG, STATUS_LED_TOGGLE);
#endif

		for (addr=start, val=fill_pattern[pat], i=0; addr<end; ) {
			if(i>=skip_size)	i=0;
//			if(((uint)addr & mask) == 0)
//				printf ("\rfill Pattern %08lX Addr(0x%qx) Read... %d * %d i:0x%x",
//					val, (addr+i), skip_size, sizeof(addr), i);

			readback = *(addr+i);
			if (readback != val) {
				printf ("\nMem error @ 0x%qx: found %08lX, data %08lX\n", addr, readback, val);
				rcode = 1;
			}

			addr += skip_size;
			i+=sizeof(addr);

			if (ctrlc()) {
				putc ('\n');
				return 1;
			}
		}
		if(rcode){
			puts ("\n");
			break;
		}
		puts ("\n");
	}
	return rcode;

}

static int buffalo_mtest_early (gd_t *gd)
{
	uint32_t	*addr, *start, *end;
	uint32_t	val;

	int     	rcode = 0;

	int pat=0;
	uint32_t pattern[]={/*0xFFFFFFFF, */0x55555555, 0xAAAAAAAA /* , 0xFF00FF00, 0x00FF00FF */
//		,0x00000000
	};

	int			first_test	= 1;

	//	uboot running on ROM
	start = (void *)KSEG1ADDR(gd->ram_size - 0x100000);
	end   = (void *)KSEG1ADDR(gd->ram_size);

	printf("Memory Test start(%08lX) end(%08lX) size(%08lX)\n",start,end, (u32)end - (u32)start);

	first_test	= 0;

	if (first_test == 0) {

		for (pat=0 ; pat<sizeof(pattern)/sizeof(pattern[0]) ; pat++) {
			register uint32_t	readback;

			printf ("\rPattern %08lX  Writing...", pattern[pat]);
			puts ("            ");
			puts ("\b\b\b\b\b\b\b\b\b\b");


			//	writing
			for (addr=start,val=pattern[pat]; addr<end ; addr++)
				*addr = val;

			puts ("Reading...");

			//	reading
			for (addr=start,val=pattern[pat]; addr<end ; addr++) {

				readback = *addr;
				if (readback != val) {
					rcode = 1;
					break;
				}
			}

			if(rcode){
				printf ("\nMem error @ 0x%08X: "
							"found %08lX, expected %08lX\n",
							(uint)addr, readback, val);
				break;
			}
		}
	}

	puts ("\n");
	memset(start, 0, (size_t)(end-start));

	return rcode;
}

#endif	//BUFFALO_MEMTEST_ADVANCED

static int buffalo_mtest (gd_t *gd)
{
	vu_long	*addr, *start, *end;
	ulong	val;
	ulong	readback;
//	unsigned int i ,dramTotalSize=0;


	//ulong	incr;
	//ulong	pattern;
	int     rcode = 0;

	int pat=0;
	ulong pattern[]={/*0xFFFFFFFF, */0x55555555, 0xAAAAAAAA, /* 0xFF00FF00, 0x00FF00FF, */
		0x00000000};	// end with 0x0.

		//	uboot running on SDRAM
		start = (ulong *)KSEG0ADDR(CFG_MEMTEST_START);
		end = (ulong *)KSEG0ADDR(CFG_MEMTEST_END);
		if (1) {
//			printf("gd : %08lX / bd : %08lX\n", gd, gd->bd);
			printf("uboot use  %08lX - %08lX\n", gd->bd->bi_boot_params, gd->bd->bi_memstart + gd->bd->bi_memsize);
			start = (ulong *)KSEG0ADDR(gd->bd->bi_memstart);
			end	= (ulong *)KSEG0ADDR(gd->bd->bi_boot_params & (~((ulong)0x100000-1)));
		}

//		for(i = 0; i< CONFIG_NR_DRAM_BANKS; i++)
//		{
//			dramTotalSize += gd->bd->bi_dram[i].size;
//		}
//		start = (ulong *)gd->bd->bi_dram[0].start;
//		end = (ulong *)((uint)start + dramTotalSize);


	printf("Memory Test start(%08lX) end(%08lX) size(%08lX)\n",start,end, (u32)end - (u32)start);
	//	end = (ulong *)(CFG_MEMTEST_END);


	for (;;) {
//		uint mask=0xfffff;
		if (ctrlc()) {
			putc ('\n');
			return 1;
		}

		printf ("\rPattern %08lX  Writing..."
			"%12s"
			"\b\b\b\b\b\b\b\b\b\b",
			pattern[pat], "");

#ifdef PRODUCT_BOARD_WACR_130N
		status_led_set(STATUS_LED_BOOT, STATUS_LED_OFF);
#endif
#if		defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G)
		status_led_set(STATUS_LED_DIAG, STATUS_LED_OFF);
#endif

		for (addr=start,val=pattern[pat]; addr<end; addr++) {
			if(((uint)addr & 0x00c000)){continue;}	//check area 0-16k /64k
			// if((uint)addr >= 0xf00000 && (uint)addr < 0x1000000 ){continue;}	//Skip 15-16M
			// if((uint)addr >= _armboot_start && (uint)addr < _bss_end ){continue;}	//Skip 15-16M
			*addr = val;
			/*
			if(((uint)addr & mask) == 0)
				printf ("\rPattern %08lX Addr(%08lX) Writing...",
					pattern[pat], (uint)addr);
			*/
		}

		puts ("Reading...");
#ifdef PRODUCT_BOARD_WACR_130N
		status_led_set(STATUS_LED_BOOT, STATUS_LED_ON);
#endif
#ifdef PRODUCT_BOARD_WRTR_222N
		status_led_set(STATUS_LED_DIAG, STATUS_LED_ON);
#endif
#if		defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G)
		status_led_set(STATUS_LED_DIAG, STATUS_LED_ON);
#endif

		for (addr=start,val=pattern[pat]; addr<end; addr++) {
			if(((uint)addr & 0x00c000)){continue;}	//check area 0-16k /64k
			// if((uint)addr >= 0xf00000 && (uint)addr < 0x1000000 ){continue;}	//Skip 15-16M
			// if((uint)addr >= _armboot_start && (uint)addr < _bss_end ){continue;}	//Skip 15-16M
			readback = *addr;
			/*
			if(((uint)addr & mask) == 0)
				printf ("\rPattern %08lX Addr(%08lX) Reading...",
					pattern[pat], (uint)addr);
			*/
			if (readback != val) {
				printf ("\nMem error @ 0x%08X: "
					"found %08lX, expected %08lX\n",
					(uint)addr, readback, val);
				rcode = 1;
			}
		}
		if(pattern[pat++]==0 || rcode){
			puts ("\n");
			break;
		}
   #if 0
		/*
		 * Flip the pattern each time to make lots of zeros and
		 * then, the next time, lots of ones.  We decrement
		 * the "negative" patterns and increment the "positive"
		 * patterns to preserve this feature.
		 */
		if(pattern & 0x80000000) {
			pattern = -pattern;	/* complement & increment */
		}
		else {
			pattern = ~pattern;
		}
		incr = -incr;
   #endif
	}
#ifdef	PRODUCT_BOARD_WACR_130N
	status_led_set(STATUS_LED_BOOT, STATUS_LED_ON);
#endif	//PRODUCT_BOARD_WACR_130N
	return rcode;
}

static	void	buffalo_env_check(void)
{
#if	defined(BUFFALO_BIN_VER)
	char	*s;
	{
		#define BUILD_DATE __DATE__" - "__TIME__
//		extern flash_info_t flash_info[];
		char env_ver[16] = {0};
		char src_ver[16] = {0};
		char env_date[32] = {0};
//		char macaddr_bak[32] = {0};
#if		defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G)
		char	*pp_name_table[]	= {
			 "hw_rev"
			,"custom_id"
			,"pincode"
			,"SUP_MBSSID"
//			,"DEF-p_wireless_ra0_11bg-authmode"
//			,"DEF-p_wireless_ra0_11bg-crypto"
//			,"DEF-p_wireless_ra0_11bg-authmode_ex"
			,"DEF-p_wireless_ra0_11bg-wpapsk"
			,"inspection"
		};
		char	pp_name_bak[sizeof(pp_name_table)/sizeof(pp_name_table[0])][32];
		int		pp_i;
#elif	defined(CONFIG_WZR_HP_G300NH) || defined(CONFIG_WHR_HP_G300N) || defined(CONFIG_WHR_HP_GN) || defined(CONFIG_WHR_G301N) || defined(CONFIG_W200)
		char	*pp_name_table[]	= {
			 "hw_rev"
			,"custom_id"
			,"pincode"
//			,"DEF-p_wireless_ath0_11bg-authmode"
//			,"DEF-p_wireless_ath0_11bg-crypto"
//			,"DEF-p_wireless_ath0_11bg-authmode_ex"
			,"DEF-p_wireless_ath0_11bg-wpapsk"
			,"region"
			,"accept_open_rt_fmt"
			,"inspection"
		};
		char	pp_name_bak[sizeof(pp_name_table)/sizeof(pp_name_table[0])][32];
		int		pp_i;
#elif	defined(CONFIG_FS_HP_G300N)
		char	*pp_name_table[]	= {
			 "hw_rev"
			,"custom_id"
			,"pincode"
//			,"DEF-p_wireless_ath0_11bg-authmode"
//			,"DEF-p_wireless_ath0_11bg-crypto"
//			,"DEF-p_wireless_ath0_11bg-authmode_ex"
			,"DEF-p_wireless_ath0_11bg-wpapsk"
			,"CUSTOMIZE_MODEL"
			,"region"
			,"accept_open_rt_fmt"
			,"inspection"
		};
		char	pp_name_bak[sizeof(pp_name_table)/sizeof(pp_name_table[0])][32];
		int		pp_i;
#else
		char pin_bak[32] = {0};
		char custom_id_bak[32] = {0};
#endif	//defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G)
		int init_flag = 0;
		if(getenv("buf_ver"))
			strcpy(env_ver, getenv("buf_ver"));
		if(getenv("build_date"))
			strcpy(env_date, getenv("build_date"));

//		sprintf(src_ver, "%d.%02d", (int)BUFFALO_BIN_VER, (int)(BUFFALO_BIN_VER*100) % 100);
		sprintf(src_ver, "%s", MK_STR(BUFFALO_BIN_VER));
		printf("### buf_ver=[%s] U-Boot Ver.=[%s]\n", env_ver,src_ver);
		if(env_ver[0]==0 || strstr(src_ver, env_ver)==0){
			init_flag = 1;
		}
		printf("### build_date(env)=[%s] build_date(bin)=[%s]\n", env_date,BUILD_DATE);
		if(env_date[0]==0 || strstr(env_date, BUILD_DATE)==0){
			init_flag = 1;
		}

		if(init_flag){
			printf("     Init Env param.\n", env_ver,src_ver);
			//
			//	backup env values
			//
#if	defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G) \
 || defined(CONFIG_WZR_HP_G300NH) || defined(CONFIG_WHR_HP_G300N) || defined(CONFIG_WHR_HP_GN) || defined(CONFIG_WHR_G301N) || defined(CONFIG_W200) || defined(CONFIG_FS_HP_G300N)
			memset(pp_name_bak, 0, sizeof(pp_name_bak));
			for (pp_i=0 ; pp_i<sizeof(pp_name_table)/sizeof(pp_name_table[0]) ; pp_i++) {
				if (getenv(pp_name_table[pp_i]))
					strcpy(pp_name_bak[pp_i], getenv(pp_name_table[pp_i]));
			}
#else	//defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G)
//			if(getenv("ethaddr"))
//				strcpy(macaddr_bak, getenv("ethaddr"));
/* 2008/2/26 Buffalo: use pincode */
#if 1
			if(getenv("pincode"))
				strcpy(pin_bak, getenv("pincode"));
#else
			if(getenv("PIN"))
				strcpy(pin_bak, getenv("PIN"));
#endif

			if(getenv("custom_id"))
				strcpy(custom_id_bak, getenv("custom_id"));
#endif	//defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G)

			//
			//	re-build env partition.
			//
			gd->env_valid = 0;
			env_relocate();

			//
			//	restore env values
			//
#if	defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G) \
 || defined(CONFIG_WZR_HP_G300NH) || defined(CONFIG_WHR_HP_G300N) || defined(CONFIG_WHR_HP_GN) || defined(CONFIG_WHR_G301N) || defined(CONFIG_W200) || defined(CONFIG_FS_HP_G300N)
			for (pp_i=0 ; pp_i<sizeof(pp_name_table)/sizeof(pp_name_table[0]) ; pp_i++) {
				if (pp_name_bak[pp_i][0] != 0) {
					setenv(pp_name_table[pp_i], pp_name_bak[pp_i]);
					printf("Recover %s -> %s\n", pp_name_table[pp_i], pp_name_bak[pp_i]);
				}
			}

#else	//defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G)
//			if(macaddr_bak[0]!=0){
//				setenv("ethaddr", macaddr_bak);
//				printf("Recover ethaddr -> %s\n", macaddr_bak);
//			}
			if(pin_bak[0]!=0){
/* 2008/2/26 Buffalo: use pincode */
#if 1
				setenv("pincode", pin_bak);
				printf("Recover pincode -> %s\n", pin_bak);
#else
				setenv("PIN", pin_bak);
				printf("Recover PIN -> %s\n", pin_bak);
#endif
			}
			if(custom_id_bak[0]!=0){
				setenv("custom_id", custom_id_bak);
				printf("Recover custom_id -> %s\n", custom_id_bak);
			}
#endif	//defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G)
			setenv("build_date", BUILD_DATE);

			saveenv();
		}

#if	defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G) \
 || defined(CONFIG_WZR_HP_G300NH) || defined(CONFIG_WHR_HP_G300N) || defined(CONFIG_WHR_HP_GN) || defined(CONFIG_WHR_G301N) || defined(CONFIG_W200) || defined(CONFIG_FS_HP_G300N)
		//	check CRC of RF-EEPROM
		if ((s = getenv ("inspection")) && simple_strtoul(s,NULL,0)>0) {
			if ((s=getenv("buf_crc"))!=NULL) {
				setenv("buf_crc",NULL);
				saveenv();
			}
		} else {
			if ((s=getenv("buf_crc"))==NULL) {
				u32		calc_crc	= crc32(0, CFG_EEPROM_ADDR, CFG_EEPROM_SIZE);
				char	crc_str[16];
				sprintf(crc_str, "%08X", calc_crc);
				setenv("buf_crc", crc_str);
				printf("Recalc crc of EEPROM\n");
				saveenv();
			}
		}
#endif	//PRODUCT_BOARD_WRTR_237G
		if(init_flag) {
			do_reset(NULL, 0, 0, NULL);
		}
	}
#endif	//BUFFALO_BIN_VER

}

static	void	buffalo_eth_init(bd_t *bd)
{
#if	defined (CONFIG_WZR_HP_G300NH)
	eth_initialize(bd);
#endif	//AP83
#if	defined (CONFIG_AR7240)
	eth_initialize(bd);
#endif	//AP93
#if defined (RT3052_ASIC_BOARD) || defined (RT3052_FPGA_BOARD) //initial cpu + 3052
	buffalo_eth_and_cpu_init_rt3052();
#endif	//RT3052
}

#if defined (RT3052_ASIC_BOARD) || defined (RT3052_FPGA_BOARD) //initial cpu + 3052
static	void	buffalo_eth_and_cpu_init_rt3052()
{
	int	my_tmp;
	int	i;
#if defined (RT3052_ASIC_BOARD) || defined (RT3052_FPGA_BOARD) //initial cpu + 3052
	RT2882_REG(0xb01100e4) = 0x00000000;
	RT2882_REG(0xb0110014) = 0xffff5555;
	RT2882_REG(0xb0110090) = 0x00007f7f;
	RT2882_REG(0xb0110098) = 0x00007fff; //disable VLAN
	RT2882_REG(0xb011009C) = 0x0008a100; // bit[3:0]=0001=300 sec, 0000=Disable aging
	RT2882_REG(0xb011008C) = 0x02404040; // bit[3:0]=0001=300 sec, 0000=Disable aging
	//RT2882_REG(0xb0000060) = 0x00000000; // pin share mode to normal
#endif	//RT3052

#if defined (RT3052_ASIC_BOARD)
	RT2882_REG(0xb01100C8) = 0x3f502b28; //Change polling Ext PHY Addr=0x0
	RT2882_REG(0xb0110084) = 0x00000000;
#endif
#if defined (RT3052_FPGA_BOARD)
	RT2882_REG(0xb01100C8) = 0x3ff02b28; //Change polling Ext PHY Addr=0x0
	RT2882_REG(0xb0110084) = 0xffdf1f00;
#endif
#if defined (CONFIG_P5_RGMII_TO_MAC_MODE)
	my_tmp = RT2882_REG(0xb01100C8);
	my_tmp |= 0x3fff;
	RT2882_REG(0xb01100C8) = my_tmp;
#endif	//
#if defined (RT3052_ASIC_BOARD) || defined (RT3052_FPGA_BOARD)
/* to lower down PHY 10Mbps mode power */
	mii_mgr_write(0, 31, 0x8000);	//---> select local register
#if defined (RT3052_MP1)
	mii_mgr_write(0, 29, 0x5088);   //---> set port 0 phy
	mii_mgr_write(1, 29, 0x5088);   //---> set port 1 phy
	mii_mgr_write(2, 29, 0x5088);   //---> set port 2 phy
	mii_mgr_write(3, 29, 0x5088);   //---> set port 3 phy
	mii_mgr_write(4, 29, 0x5088);   //---> set port 4 phy
#elif defined (RT3052_MP2)
	for(i=0;i<5;i++){
		mii_mgr_write(i, 26, 0x1601); 	//TX10 waveform coefficient
		mii_mgr_write(i, 29, 0x7058); 	//TX100/TX10 AD/DA current bias
		mii_mgr_write(i, 30, 0x0058); 	//TX100 slew rate control
	}
#endif	//RT3052
/* PHY IOT */
	mii_mgr_write(0, 31, 0x0);	//select global register
#if defined (RT3052_MP1)
	mii_mgr_write(0, 18, 0x40da);	//set squelch amplitude to higher threshold
#elif defined (RT3052_MP2)
	mii_mgr_write(0, 22, 0x052f);	//tune TP_IDL tail and head waveform
	mii_mgr_write(0, 17, 0x0fe0);	//set TX10 signal amplitude threshold to minimum
	mii_mgr_write(0, 18, 0x40ba);	//set squelch amplitude to higher threshold
	mii_mgr_write(0, 14, 0x65);	//longer TP_IDL tail length
#endif	//RT3052_MP2
	mii_mgr_write(0, 31, 0x8000);	//select local register
#endif	//RT3052
}
#endif	//RT3052

static	void	buffalo_bootcmd(void)
{
	char *s;
//	int	f_break	= 0;
	do	{
		extern	ulong	NetTimeoutBuffalo;
		extern int do_bootm(cmd_tbl_t *, int, int, char *[]);
		extern int do_tftps(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
		extern int do_tftpb (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
		cmd_tbl_t *cmdtp;
		char	*argv[4];
		int		argc;

		ctrlc();
		if (had_ctrlc())	break;

		//
		//	ram-boot for TFTP-client
		//
		if ((s = getenv ("inspection")) && simple_strtoul(s,NULL,0)==1) {
			//	inspection-mode
#if			defined(CONFIG_WHR_HP_G300N) || defined(CONFIG_WHR_HP_GN) || defined(CONFIG_WHR_G301N) || defined(CONFIG_W200) || defined(CONFIG_FS_HP_G300N)
			//	wait athers26 up
			do	{
				unsigned	_i;
//				for (_i=0 ; _i<2000 ; _i++)	udelay(1000);
			} while(0);
#endif		//defined(CONFIG_WHR_HP_G300N) || defined(CONFIG_WHR_HP_GN)
//				setenv("netretry", "no");
			setenv("autostart", "no");
			setenv("loadaddr", MK_STR(BUFFALO_TMP_RAM));
			argv[0]	= "tftpboot";
			argv[1]	= MK_STR(BUFFALO_TMP_RAM);
			argv[2]	= "firmware.ram";
			argc	= 3;
			NetTimeoutBuffalo	= get_timer(0);
			do_tftpb(cmdtp, 0, argc, argv);
			//	not return if success.
		}


		ctrlc();
		if (had_ctrlc())	break;


		//
		//	FW-WRITE / RAM-BOOT for TFTP-server
		//
		setenv("loadaddr", MK_STR(BUFFALO_TMP_RAM));
		argc = 2;
		argv[0]= "TFTPS";
		argv[1]= "buf";
		NetTimeoutBuffalo	= get_timer(0);
		if (do_tftps(cmdtp, 0, argc, argv)==2) {
			//	no loaded
		}
		NetTimeoutBuffalo	= 0;


		ctrlc();
		if (had_ctrlc())	break;


#if		defined(PRODUCT_BOARD_WRTR_236G) || defined(PRODUCT_BOARD_WRTR_237G) \
		|| defined(CONFIG_WZR_HP_G300NH) || defined(CONFIG_WHR_HP_G300N) || defined(CONFIG_WHR_HP_GN) || defined(CONFIG_WHR_G301N) || defined(CONFIG_W200) || defined(CONFIG_FS_HP_G300N)
		//	check CRC of RF-EEPROM
		if ((s = getenv ("inspection")) && simple_strtoul(s,NULL,0)>0) {
			//	check skip
		} else {
			u32		calc_crc	= crc32(0, CFG_EEPROM_ADDR, CFG_EEPROM_SIZE);
			u32		read_crc	= (s=getenv("buf_crc")) ? simple_strtoul(s, NULL, 16) : ~calc_crc;
			if (read_crc != calc_crc) {
				printf("Bad CRC of EEPROM!!\n");
				status_led_blink_num_set(STATUS_LED_DIAG, 2);
			}
		}
#endif	//PRODUCT_BOARD_WRTR_237G

		ctrlc();
		if (had_ctrlc())	break;


		//	inspection==3 ----> run bootcmd
		if ((s = getenv ("inspection")) && simple_strtoul(s,NULL,0)==3)
			run_command("run bootcmd", 0/*flag*/);


		setenv("loadaddr", MK_STR(BUFFALO_FW_SADDR));
//		printf("   \n3: System Boot system code via Flash.\n");
		do_bootm(cmdtp, 0, 1, argv);
	} while(0);
}

#endif	//CONFIG_BUFFALO


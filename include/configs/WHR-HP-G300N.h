/*
 * This file contains the configuration parameters for the dbau1x00 board.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/ar7240.h>


/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CFG_MAX_FLASH_BANKS     1	    /* max number of memory banks */
#if	defined(CONFIG_WHR_HP_G300N)
//#define CFG_MAX_FLASH_SECT      128    /* max number of sectors on one chip */
//#define CFG_FLASH_SECTOR_SIZE   (64*1024)
#define CFG_MAX_FLASH_SECT      2048    /* max number of sectors on one chip */
#define CFG_FLASH_SECTOR_SIZE   (4*1024)
#define CFG_FLASH_SIZE          0x00800000 /* Total flash size */
#elif	defined(CONFIG_WHR_HP_GN)
#define CFG_MAX_FLASH_SECT      1024    /* max number of sectors on one chip */
#define CFG_FLASH_SECTOR_SIZE   (4*1024)
#define CFG_FLASH_SIZE          0x00400000 /* Total flash size */
#elif	defined(CONFIG_WHR_G301N)
#define CFG_MAX_FLASH_SECT      1024    /* max number of sectors on one chip */
#define CFG_FLASH_SECTOR_SIZE   (4*1024)
#define CFG_FLASH_SIZE          0x00400000 /* Total flash size */
#elif	defined(CONFIG_W200)
#define CFG_MAX_FLASH_SECT      1024    /* max number of sectors on one chip */
#define CFG_FLASH_SECTOR_SIZE   (4*1024)
#define CFG_FLASH_SIZE          0x00400000 /* Total flash size */
#elif	defined(CONFIG_FS_HP_G300N)
#define CFG_MAX_FLASH_SECT      1024    /* max number of sectors on one chip */
#define CFG_FLASH_SECTOR_SIZE   (4*1024)
#define CFG_FLASH_SIZE          0x00400000 /* Total flash size */
#else	//AP93
#define CFG_MAX_FLASH_SECT      64    /* max number of sectors on one chip */
#define CFG_FLASH_SECTOR_SIZE   (64*1024)
#define CFG_FLASH_SIZE          0x00400000 /* Total flash size */
#endif	//CONFIG_WHR_HP_GN


#if (CFG_MAX_FLASH_SECT * CFG_FLASH_SECTOR_SIZE) != CFG_FLASH_SIZE
#	error "Invalid flash configuration"
#endif

#define CFG_FLASH_WORD_SIZE     unsigned short 

/* 
 * We boot from this flash
 */
#define CFG_FLASH_BASE		    0xbf000000

//	SUPPORT LZMA
#define	CONFIG_LZMA			1
#define	BUFFALO_MEMTEST_ADVANCED	1
#define	BUFFALO_MEMTEST_EARLY		1

//	MEMORY ADDR DEFINE
#ifdef CONFIG_BUFFALO
#	define	XMK_STR(x)	#x
#	define	MK_STR(x)	XMK_STR(x)
#	define	XMK_HEX(x)	0x##x
#	define	MK_HEX(x)	XMK_HEX(x)
#	ifdef	CONFIG_WHR_HP_G300N
#		define		BUFFALO_FW_SADDR		BF040000
#		define		BUFFALO_FW_EADDR		BF3EFFFF
#		define		BUFFALO_UBOOT_SADDR		BF000000
#		define		BUFFALO_UBOOT_EADDR		BF03DFFF
#		define		BUFFALO_TMP_RAM			80F00000
#		define		BUFFALO_TMP_BOTTOM		81F00000
//#		define		BUFFALO_MEMTEST_SADDR	00000000
//#		define		BUFFALO_MEMTEST_EADDR	03F00000
		//	-- acceptable encrypt firmware --
#		define		PLATFORM_ath
#		define		BUFFALO_ACCEPT_FW_MODEL	"WHR-HP-G300N"
#		define		BUFFALO_REGION_ENV		"region"
#		define		BUFFALO_HW_REV_ENV		"hw_rev"
#	endif	//CONFIG_WHR_HP_G300N
#endif	//CONFIG_BUFFALO

/* 
 * The following #defines are needed to get flash environment right 
 */
#define	CFG_MONITOR_BASE	TEXT_BASE
#define	CFG_MONITOR_LEN		(192 << 10)

#undef CONFIG_BOOTARGS
/* XXX - putting rootfs in last partition results in jffs errors */
#define	CONFIG_BOOTARGS     "console=ttyS0,115200 root=31:02 rootfstype=jffs2 init=/sbin/init mtdparts=ar7240-nor0:256k(u-boot),64k(u-boot-env),2752k(rootfs),896k(uImage),64k(NVRAM),64k(ART)"

/* default mtd partition table */
#undef MTDPARTS_DEFAULT
#define MTDPARTS_DEFAULT    "mtdparts=ar7240-nor0:256k(u-boot),64k(u-boot-env),5120k(rootfs),1024k(uImage)"

#undef CFG_PLL_FREQ
#define CFG_PLL_FREQ	CFG_PLL_400_400_200

#undef CFG_HZ
/*
 * MIPS32 24K Processor Core Family Software User's Manual
 *
 * 6.2.9 Count Register (CP0 Register 9, Select 0)
 * The Count register acts as a timer, incrementing at a constant
 * rate, whether or not an instruction is executed, retired, or
 * any forward progress is made through the pipeline.  The counter
 * increments every other clock, if the DC bit in the Cause register
 * is 0.
 */
/* Since the count is incremented every other tick, divide by 2 */
/* XXX derive this from CFG_PLL_FREQ */
#if (CFG_PLL_FREQ == CFG_PLL_200_200_100)
#	define CFG_HZ          (200000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_300_300_150)
#	define CFG_HZ          (300000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_350_350_175)
#	define CFG_HZ          (350000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_333_333_166)
#	define CFG_HZ          (333000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_266_266_133)
#	define CFG_HZ          (266000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_266_266_66)
#	define CFG_HZ          (266000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_400_400_200) || (CFG_PLL_FREQ == CFG_PLL_400_400_100)
#	define CFG_HZ          (400000000/2)
#endif


/* 
 * timeout values are in ticks 
 */
#define CFG_FLASH_ERASE_TOUT	(2 * CFG_HZ) /* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT	(2 * CFG_HZ) /* Timeout for Flash Write */

/*
 * Cache lock for stack
 */
#define CFG_INIT_SP_OFFSET	0x1000

#define	CFG_ENV_IS_IN_FLASH    1
#undef CFG_ENV_IS_NOWHERE  

/* Address and size of Primary Environment Sector	*/
#define CFG_ENV_ADDR		0xbf040000
#define CFG_ENV_SIZE		0x10000

#define CONFIG_BOOTCOMMAND "bootm 0x9f300000"
//#define CONFIG_FLASH_16BIT

/* DDR init values */

#define CONFIG_NR_DRAM_BANKS	2
//#define CFG_DDR_REFRESH_VAL     0x4f10
//#define CFG_DDR_REFRESH_VAL     0x4bb8
#define CFG_DDR_REFRESH_VAL     0x4AF0
#define CFG_DDR_CONFIG_VAL      0xc7bc8cd0
#define CFG_DDR_MODE_VAL_INIT   0x133
//#define CFG_DDR_EXT_MODE_VAL    0x0
#define CFG_DDR_EXT_MODE_VAL    0x2
#define CFG_DDR_MODE_VAL        0x33

#define CFG_DDR_TRTW_VAL        0x1f
#define CFG_DDR_TWTR_VAL        0x1e

#define CFG_DDR_CONFIG2_VAL	 0x9dd0e6a8
#define CFG_DDR_RD_DATA_THIS_CYCLE_VAL  0x00ff


#define CONFIG_NET_MULTI

#define CONFIG_MEMSIZE_IN_BYTES
#define CONFIG_PCI

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CONFIG_COMMANDS	(( CONFIG_CMD_DFL | CFG_CMD_DHCP | CFG_CMD_ELF | CFG_CMD_PCI |	\
	CFG_CMD_MII | CFG_CMD_PING | CFG_CMD_NET | CFG_CMD_JFFS2 | CFG_CMD_ENV |	\
	CFG_CMD_FLASH | CFG_CMD_LOADS | CFG_CMD_RUN | CFG_CMD_LOADB | CFG_CMD_ELF | CFG_CMD_ETHREG ))


#define CONFIG_IPADDR   192.168.11.1
#define CONFIG_SERVERIP 192.168.11.2
#define CONFIG_ETHADDR 0x02:0xaa:0xbb:0xcc:0xdd:0x1c
#define CFG_FAULT_ECHO_LINK_DOWN    1


#define CFG_PHY_ADDR 0 
#define CFG_AG7240_NMACS 2
#define CFG_GMII     0
#define CFG_MII0_RMII             1
#define CFG_AG7100_GE0_RMII             1

#define CFG_BOOTM_LEN	(16 << 20) /* 16 MB */
#define DEBUG
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2 "hush>"

/*
** Parameters defining the location of the calibration/initialization
** information for the two Merlin devices.
** NOTE: **This will change with different flash configurations**
*/

#define WLANCAL                        0xbfff1000
#define BOARDCAL                       0xbfff0000
#define ATHEROS_PRODUCT_ID             137
#define CAL_SECTOR                     (CFG_MAX_FLASH_SECT - 1)

/* For Merlin, both PCI, PCI-E interfaces are valid */
#define AR7240_ART_PCICFG_OFFSET        12



#include <cmd_confdefs.h>




/***************************************************************************/
#ifdef CONFIG_BUFFALO

/* 注意: u_fw/u_uboot変数名は他製品でも共通で使用する名称のため、変更しないこと */
#define	CONFIG_EXTRA_ENV_SETTINGS							\
	"tmp_ram=" MK_STR(BUFFALO_TMP_RAM) "\0"				\
	"tmp_bottom=" MK_STR(BUFFALO_TMP_BOTTOM) "\0"				\
	"fw_eaddr=" MK_STR(BUFFALO_FW_SADDR) " " MK_STR(BUFFALO_FW_EADDR) "\0"				\
	"uboot_eaddr=" MK_STR(BUFFALO_UBOOT_SADDR) " " MK_STR(BUFFALO_UBOOT_EADDR) "\0"				\
	"u_fw=erase $fw_eaddr; cp.b $fileaddr " MK_STR(BUFFALO_FW_SADDR) " $filesize; bootm " MK_STR(BUFFALO_FW_SADDR) ";\0"		\
	"ut_fw=tftp $tmp_ram firmware.bin; erase $fw_eaddr; cp.b $fileaddr " MK_STR(BUFFALO_FW_SADDR) " $filesize; bootm " MK_STR(BUFFALO_FW_SADDR) ";\0"	\
	"ut_uboot=tftp $tmp_ram u-boot.bin; protect off $uboot_eaddr; erase $uboot_eaddr; cp.b $fileaddr " MK_STR(BUFFALO_UBOOT_SADDR) " $filesize;\0"		\
	"melco_id=" BUFFALO_MELCO_ID "\0"	\
	"hw_rev=" BUFFALO_HW_REV "\0"		\
	"tftp_wait=4\0"						\
	CONFIG_EXTRA_ENV_SETTINGS_PRODUCT	\
	""


/*--------------------------------------------------*/
#ifdef	CONFIG_WHR_HP_G300N
#undef		CONFIG_BOOTCOMMAND
#undef		CONFIG_BOOTARGS
#undef		CONFIG_BOOTDELAY
#undef		CFG_ENV_ADDR
#undef		CFG_ENV_SIZE
#undef		CONFIG_ETHADDR
#define		CFG_ENV_ADDR			0xbf03e000
#define		CFG_ENV_SIZE			0x2000
#define		CFG_EEPROM_ADDR			0xBF3F0000
#define		CFG_EEPROM_SIZE			0x10000
#define		CONFIG_ETHADDR			02:AA:BB:CC:DD:1C
#define		CONFIG_BOOTCOMMAND		"bootm " MK_STR(BUFFALO_FW_SADDR)
//#define	CONFIG_BOOTARGS			"console=ttyS0,115200 root=31:02 rootfstype=jffs2 init=/sbin/init mtdparts=ar7240-nor0:256k(u-boot),64k(u-boot-env),2752k(rootfs),896k(uImage),64k(NVRAM),64k(ART)"
#define	CONFIG_BOOTARGS 			"console=ttyS0,115200 root=31:03 rootfstype=jffs2 init=/sbin/init mtdparts=ar7240-nor0:248k(u-boot),8k(u-boot-env),896k(uImage),2816k(rootfs),64k(NVRAM),64k(ART)"
#define	CONFIG_BOOTDELAY			-1
#define	CONFIG_EXTRA_ENV_SETTINGS_PRODUCT				\
	"uboot_ethaddr=02:AA:BB:CC:DD:1C\0"					\
	"DEF-p_wireless_ath0_11bg-authmode=psk\0"			\
	"DEF-p_wireless_ath0_11bg-crypto=tkip+aes\0"		\
	"DEF-p_wireless_ath0_11bg-authmode_ex=mixed-psk\0"	\
	"DEF-p_wireless_ath0_11bg-wpapsk=1234567890123\0"	\
	"pincode=12345670\0"								\
	"custom_id=0\0"										\
	""
#endif	//CONFIG_WHR_HP_G300N
/*--------------------------------------------------*/


#define	CONFIG_STATUS_LED		1 /* enable status led driver */


/*--------------------------------------------------*/
#if		defined(CONFIG_WHR_HP_G300N)
#if		1
#define	CFG_BUFFALO_GPIO_OUT_MASK	0x3E45F
#define	STATUS_LED_BITS			3

#define	STATUS_LED_BIT			(1 << 6)	/* LED[ROUTER] */
#define	STATUS_LED_STATE		STATUS_LED_OFF
//#define	STATUS_LED_PERIOD		(CFG_HZ / 10)	/* ca. 1 Hz */
#define	STATUS_LED_PERIOD		1000
#define STATUS_LED_ACTIVE_LOW	1
#define	STATUS_LED_ROUTER		0		/* ROUTER LED */

#if	(STATUS_LED_BITS > 1)
#define	STATUS_LED_BIT1			(1 << 0)	/* LED[SECURITY_G] */
#define	STATUS_LED_STATE1		STATUS_LED_OFF
//#define	STATUS_LED_PERIOD1		(CFG_HZ / 50)	/* ca. 5 Hz */
#define	STATUS_LED_PERIOD1		1000
#define STATUS_LED_ACTIVE_LOW1	1
#define	STATUS_LED_SECURITY		1		/* SECURITY G LED */
#endif

#if	(STATUS_LED_BITS > 2)
#define	STATUS_LED_BIT2			(1 << 1)	/* LED[DIAG] */
#define	STATUS_LED_STATE2		STATUS_LED_OFF
//#define	STATUS_LED_PERIOD2		(CFG_HZ / 50)	/* ca. 5 Hz */
#define	STATUS_LED_PERIOD2		1000
#define STATUS_LED_ACTIVE_LOW2	1
#define	STATUS_LED_DIAG			2		/* fail LED */
#endif

#if	(STATUS_LED_BITS > 3)
#define	STATUS_LED_BIT3			(1 << 1) /* on AR9283 */	/* LED[WIRELESS] */
#define	STATUS_LED_STATE3		STATUS_LED_OFF
//#define	STATUS_LED_PERIOD2		(CFG_HZ / 50)	/* ca. 5 Hz */
#define	STATUS_LED_PERIOD3		1000
#define STATUS_LED_ACTIVE_LOW3	1
#define	STATUS_LED_WIRELESS		3		/* fail LED */
#endif

#if	(STATUS_LED_BITS > 4)
#define	STATUS_LED_BIT4			(1 << 0)	/* LED[USB] */
#define	STATUS_LED_STATE4		STATUS_LED_OFF
//#define	STATUS_LED_PERIOD2		(CFG_HZ / 50)	/* ca. 5 Hz */
#define	STATUS_LED_PERIOD4		1000
#define STATUS_LED_ACTIVE_LOW4	1
#define	STATUS_LED_USB			4		/* fail LED */
#endif

#else	//	for REFERENCE BOARD
#define	STATUS_LED_BITS			3

#define	STATUS_LED_BIT			(1 << 1)	/* LED[ROUTER] */
#define	STATUS_LED_STATE		STATUS_LED_OFF
//#define	STATUS_LED_PERIOD		(CFG_HZ / 10)	/* ca. 1 Hz */
#define	STATUS_LED_PERIOD		1000
#define STATUS_LED_ACTIVE_LOW	1
#define	STATUS_LED_ROUTER		0		/* ROUTER LED */

#if	(STATUS_LED_BITS > 1)
#define	STATUS_LED_BIT1			(1 << 11)	/* LED[SECURITY_G] */
#define	STATUS_LED_STATE1		STATUS_LED_OFF
//#define	STATUS_LED_PERIOD1		(CFG_HZ / 50)	/* ca. 5 Hz */
#define	STATUS_LED_PERIOD1		1000
#define STATUS_LED_ACTIVE_LOW1	1
#define	STATUS_LED_SECURITY		1		/* SECURITY G LED */
#endif

#if	(STATUS_LED_BITS > 2)
#define	STATUS_LED_BIT2			(1 << 0)	/* LED[DIAG] */
#define	STATUS_LED_STATE2		STATUS_LED_OFF
//#define	STATUS_LED_PERIOD2		(CFG_HZ / 50)	/* ca. 5 Hz */
#define	STATUS_LED_PERIOD2		1000
#define STATUS_LED_ACTIVE_LOW2	1
#define	STATUS_LED_DIAG			2		/* fail LED */
#endif

#if	(STATUS_LED_BITS > 3)
#define	STATUS_LED_BIT3			(1 << 1) /* on AR9283 */	/* LED[WIRELESS] */
#define	STATUS_LED_STATE3		STATUS_LED_OFF
//#define	STATUS_LED_PERIOD2		(CFG_HZ / 50)	/* ca. 5 Hz */
#define	STATUS_LED_PERIOD3		1000
#define STATUS_LED_ACTIVE_LOW3	1
#define	STATUS_LED_WIRELESS		3		/* fail LED */
#endif

#if	(STATUS_LED_BITS > 4)
#define	STATUS_LED_BIT4			(1 << 11)	/* LED[USB] */
#define	STATUS_LED_STATE4		STATUS_LED_OFF
//#define	STATUS_LED_PERIOD2		(CFG_HZ / 50)	/* ca. 5 Hz */
#define	STATUS_LED_PERIOD4		1000
#define STATUS_LED_ACTIVE_LOW4	1
#define	STATUS_LED_USB			4		/* fail LED */
#endif

#endif
#endif	//CONFIG_WHR_HP_G300N

#define	STATUS_LED_PAR			1 /* makes status_led.h happy */
#define CONFIG_BOARD_SPECIFIC_LED

#if		defined(CONFIG_WHR_HP_G300N)
#ifndef	__ASSEMBLY__
//####################################################################################
//
//	for memory test
//
#define	BUFFALO_MTEST_PARAM_ADDR	(MK_HEX(BUFFALO_FW_EADDR) & (~(unsigned long)0xFFFF))

#define	BUFFALO_MTEST_PARAM_MAGIC	0x4D545354		//'MTST'
#define	BUFFALO_MTEST_PARAM_VER		3

#define	BUFFALO_MTEST_MODE_WD		0x0001
#define	BUFFALO_MTEST_MODE_WC		0x0002
#define	BUFFALO_MTEST_MODE_RD		0x0000
#define	BUFFALO_MTEST_MODE_RC		0x0004
#define	BUFFALO_MTEST_MODE_WDRD		0x0001			//Write to direct / Read from direct
#define	BUFFALO_MTEST_MODE_WCRD		0x0002			//Write to cache / Read form direct
#define	BUFFALO_MTEST_MODE_WDRC		0x0005			//Write to direct / Read from cache		(unsupport)
#define	BUFFALO_MTEST_MODE_WCRC		0x0006			//Write to cache / Read from cache		(unsupport)
#define	BUFFALO_MTEST_MODE_RAND		0x0010			//test pattern ramdom

#define	BUFFALO_MTEST_SLIDE_MASK	0x00000180		//
#define	BUFFALO_MTEST_SLIDE_NONE	0x00000180		//POS='AUTO'
#define	BUFFALO_MTEST_SLIDE_TEST	0x00000100		//POS='ON'
//#define	BUFFALO_MTEST_SLIDE_TAPfix	0x00000080		//POS='OFF'		//TAP値固定で起動する
#define	BUFFALO_MTEST_SLIDE_TAPloop	0x00000080		//POS='OFF'			//TAPの計算ルーチンをループさせる

#define	BUFFALO_TAP_TEST_NORMAL		0x0000
#define	BUFFALO_TAP_TEST_ROW		0x0001
#define	BUFFALO_TAP_TEST_BIT		0x0002

typedef struct	_BUFFALO_MTEST_PARAM	{
	unsigned long	magic;
	unsigned long	version;
	unsigned long	test_mode;
	unsigned long	reserved01;
	unsigned long	test_offset;
	unsigned long	test_size;
	unsigned long	test_pattern;
	unsigned long	test_wait;
	//	reg flags
	unsigned long	tap_test_mode;
	unsigned long	reserved12;
	unsigned long	reserved13;
	unsigned long	reserved14;
	//	reg vals
	unsigned long	vAR7240_DDR_CONFIG;
	unsigned long	vAR7240_DDR_CONFIG2;
	unsigned long	vAR7240_DDR_MODE;
	unsigned long	vAR7240_DDR_EXT_MODE;
	unsigned long	vAR7240_DDR_CONTROL;
	unsigned long	vAR7240_DDR_REFRESH;
	unsigned long	vAR7240_DDR_RD_DATA_THIS_CYCLE;
	unsigned long	vAR7240_DDR_TAP_CONTROL0;
	unsigned long	vAR7240_DDR_TAP_CONTROL1;
	unsigned long	vAR7240_DDR_TAP_CONTROL2;
	unsigned long	vAR7240_DDR_TAP_CONTROL3;
	unsigned long	reserved21;
} BUFFALO_MTEST_PARAM;

//	---- dumping ---
#define	UL_VAL(P,OFF)									(((unsigned long *)(P))[OFF/4])
#define	BT_MASK(POS,BITLEN)								(((BITLEN)==32) ? ((unsigned long)0xFFFFFFFF) : ((((unsigned long)1 << (BITLEN)) - 1) << (POS)))
#define	BT_UMASK(POS,BITLEN)							(~BT_MASK(POS,BITLEN))
#define	GET_BITFIELD(REGVAL,BITPOS,BITLEN)				(((REGVAL) & BT_MASK(BITPOS,BITLEN))>>(BITPOS))
#define	GET_BITFIELD_OFF(PPRM,OFF,BITPOS,BITLEN)		GET_BITFIELD(UL_VAL(PPRM,OFF),BITPOS,BITLEN)
#define	SET_BITFIELD_OFF(PPRM,OFF,BITPOS,BITLEN,VAL)	do { UL_VAL(PPRM,OFF) = ((BITLEN)==32) ? (VAL) : (UL_VAL(PPRM,OFF) & BT_UMASK(BITPOS,BITLEN)) | (((VAL)<<(BITPOS)) & BT_MASK(BITPOS,BITLEN)) ; } while(0)

#include	<ar7240_soc.h>

static void inline buffalo_mtest_ddr_conf_dump(BUFFALO_MTEST_PARAM *p)
{
	extern void printf(const char *fmt, ...);
	printf("OFFSET:%08lX ==> %08lX\n",		AR7240_DDR_CONFIG, p->vAR7240_DDR_CONFIG);
	printf(" %-16.16s: %04Xh\n", "tRAS",	GET_BITFIELD(p->vAR7240_DDR_CONFIG, 0,5));
	printf(" %-16.16s: %04Xh\n", "tRCD",	GET_BITFIELD(p->vAR7240_DDR_CONFIG, 5,4));
	printf(" %-16.16s: %04Xh\n", "tRP",		GET_BITFIELD(p->vAR7240_DDR_CONFIG, 9,4));
	printf(" %-16.16s: %04Xh\n", "tRRD",	GET_BITFIELD(p->vAR7240_DDR_CONFIG,13,4));
	printf(" %-16.16s: %04Xh\n", "tRFC",	GET_BITFIELD(p->vAR7240_DDR_CONFIG,17,6));
	printf(" %-16.16s: %04Xh\n", "tMRD",	GET_BITFIELD(p->vAR7240_DDR_CONFIG,23,4));
	printf(" %-16.16s: %04Xh\n", "OP",		GET_BITFIELD(p->vAR7240_DDR_CONFIG,30,1));
	printf(" %-16.16s: %04Xh\n", "CL",		GET_BITFIELD(p->vAR7240_DDR_CONFIG,27,3));
	printf(" %-16.16s: %04Xh\n", "CL MSB",	GET_BITFIELD(p->vAR7240_DDR_CONFIG,31,1));
	printf("OFFSET:%08lX ==> %08lX\n", AR7240_DDR_CONFIG2, p->vAR7240_DDR_CONFIG2);
	printf(" %-16.16s: %04Xh\n", "BrustLength",		GET_BITFIELD(p->vAR7240_DDR_CONFIG2, 0,4));
	printf(" %-16.16s: %04Xh\n", "BurstType",		GET_BITFIELD(p->vAR7240_DDR_CONFIG2, 4,1));
	printf(" %-16.16s: %04Xh\n", "CNTL_OE_EN",		GET_BITFIELD(p->vAR7240_DDR_CONFIG2, 5,1));
	printf(" %-16.16s: %04Xh\n", "PHASE_SELECT",	GET_BITFIELD(p->vAR7240_DDR_CONFIG2, 6,1));
	printf(" %-16.16s: %04Xh\n", "CKE",				GET_BITFIELD(p->vAR7240_DDR_CONFIG2, 7,1));
	printf(" %-16.16s: %04Xh\n", "TWR",				GET_BITFIELD(p->vAR7240_DDR_CONFIG2, 8,4));
	printf(" %-16.16s: %04Xh\n", "TRTW",			GET_BITFIELD(p->vAR7240_DDR_CONFIG2,12,5));
	printf(" %-16.16s: %04Xh\n", "TRTP",			GET_BITFIELD(p->vAR7240_DDR_CONFIG2,17,4));
	printf(" %-16.16s: %04Xh\n", "TWTR",			GET_BITFIELD(p->vAR7240_DDR_CONFIG2,21,5));
	printf(" %-16.16s: %04Xh\n", "G_O_LATENCY",		GET_BITFIELD(p->vAR7240_DDR_CONFIG2,26,4));
	printf(" %-16.16s: %04Xh\n", "HALF_WIDTH",		GET_BITFIELD(p->vAR7240_DDR_CONFIG2,31,1));
	printf("OFFSET:%08lX ==> %08lX\n", AR7240_DDR_MODE, p->vAR7240_DDR_MODE);
	printf(" %-16.16s: %04Xh\n", "DDR_MODE",		GET_BITFIELD(p->vAR7240_DDR_MODE, 0,13));
	printf("OFFSET:%08lX ==> %08lX\n", AR7240_DDR_EXT_MODE, p->vAR7240_DDR_EXT_MODE);
	printf(" %-16.16s: %04Xh\n", "EXT_MODE",		GET_BITFIELD(p->vAR7240_DDR_EXT_MODE, 0,13));
	printf("OFFSET:%08lX ==> %08lX\n", AR7240_DDR_REFRESH, p->vAR7240_DDR_REFRESH);
	printf(" %-16.16s: %04Xh\n", "PERIOD",		GET_BITFIELD(p->vAR7240_DDR_REFRESH, 0,14));
	printf(" %-16.16s: %04Xh\n", "ENABLE",		GET_BITFIELD(p->vAR7240_DDR_REFRESH,14,1));
	printf("OFFSET:%08lX ==> %08lX\n", AR7240_DDR_RD_DATA_THIS_CYCLE, p->vAR7240_DDR_RD_DATA_THIS_CYCLE);
	printf(" %-16.16s: %06Xh\n", "VEC",			GET_BITFIELD(p->vAR7240_DDR_RD_DATA_THIS_CYCLE, 0,24));
}

#endif	//BUFFALO_MTEST_MODULE
#endif	//CONFIG_WHR_HP_G300N

/*--------------------------------------------------*/
#endif	//CONFIG_BUFFALO

#endif	/* __CONFIG_H */

#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm-mips/regdef.h>
#include <config.h>
#include <version.h>
#include "ar7240_soc.h"

extern void ar7240_ddr_initial_config(uint32_t refresh);
#ifdef	CONFIG_BUFFALO
extern void ar7240_ddr_initial_config_for_mtest(uint32_t refresh);
#endif	//CONFIG_BUFFALO
extern int ar7240_ddr_find_size(void);

void
ar7240_usb_initial_config(void)
{
    ar7240_reg_wr_nf(AR7240_USB_PLL_CONFIG, 0x0a04081e);
    ar7240_reg_wr_nf(AR7240_USB_PLL_CONFIG, 0x0804081e);
}

void ar7240_gpio_config(void)
{
    /* Disable clock obs */
    ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e0ff));
#ifdef	CONFIG_BUFFALO
    //	disable JTAG
    ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) | 0x1));
#endif	//CONFIG_BUFFALO
    /* Enable eth Switch LEDs */
    ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) | 0xf8));

}

int
ar7240_mem_config(void)
{
#ifndef	CONFIG_BUFFALO
    unsigned int tap_val1, tap_val2;
    ar7240_ddr_initial_config(CFG_DDR_REFRESH_VAL);
/* Default tap values for starting the tap_init*/

    ar7240_reg_wr (AR7240_DDR_TAP_CONTROL0, 0x7);
    ar7240_reg_wr (AR7240_DDR_TAP_CONTROL1, 0x7);

    ar7240_ddr_tap_init();

    tap_val1 = ar7240_reg_rd(0xb800001c);
    tap_val2 = ar7240_reg_rd(0xb8000020);

    printf("#### TAP VALUE 1 = %x, 2 = %x\n",tap_val1, tap_val2);

#else	//CONFIG_BUFFALO
	unsigned int	tap_val0, tap_val1;
	unsigned int	tap_val;
	unsigned int	tap_high, tap_low;
	unsigned int	read_cnt	= 0x200;
	unsigned int	read_fail;
	unsigned int	pos;
	unsigned int	start_pos		= 7;
	unsigned int	skip_pat_init	= 0;
#ifdef	BUFFALO_MTEST_SLIDE_TAPfix
	int				skip_tap_cal	= 0;
#endif	//BUFFALO_MTEST_SLIDE_TAPfix
#ifdef	BUFFALO_MTEST_SLIDE_TAPloop
	int				loop_tap_cal	= 0;
	int				tap_test_mode	= 0;
#endif	//BUFFALO_MTEST_SLIDE_TAPloop

	do	{
		unsigned int slide = ar7240_reg_rd(AR7240_GPIO_IN) & BUFFALO_MTEST_SLIDE_MASK;
		volatile BUFFALO_MTEST_PARAM *pprm	= (volatile BUFFALO_MTEST_PARAM *)BUFFALO_MTEST_PARAM_ADDR;
#ifdef	BUFFALO_MTEST_SLIDE_TAPloop
		if (slide==BUFFALO_MTEST_SLIDE_TAPloop) {
			if (pprm->magic == BUFFALO_MTEST_PARAM_MAGIC && pprm->version == BUFFALO_MTEST_PARAM_VER) {
				tap_test_mode	= pprm->tap_test_mode;

				loop_tap_cal	= 1;
				ar7240_ddr_initial_config_for_mtest(0);

				printf("\n\nTAP TEST LOOP\n");
				if (tap_test_mode==BUFFALO_TAP_TEST_BIT) {
					printf("--------------------------------\n");
					printf("0   4   8   C   0   4   8   C   \n");
					printf("----====----====----====----====\n");
				}
				break;
			}
		}
#endif	//BUFFALO_MTEST_SLIDE_TAPloop
#ifdef	BUFFALO_MTEST_SLIDE_TAPfix
		if (slide==BUFFALO_MTEST_SLIDE_TAPfix) {
			if (pprm->magic == BUFFALO_MTEST_PARAM_MAGIC && pprm->version == BUFFALO_MTEST_PARAM_VER) {
				skip_tap_cal	= 1;
				ar7240_ddr_initial_config_for_mtest(1);
				break;
			}
		}
#endif	//BUFFALO_MTEST_SLIDE_TAPfix
		if (slide==BUFFALO_MTEST_SLIDE_TEST) {
			if (pprm->magic == BUFFALO_MTEST_PARAM_MAGIC && pprm->version == BUFFALO_MTEST_PARAM_VER) {
				ar7240_ddr_initial_config_for_mtest(0);
				break;
			}
		}

		//	default routine
		ar7240_ddr_initial_config(CFG_DDR_REFRESH_VAL);
	} while(0);


#ifdef	BUFFALO_MTEST_SLIDE_TAPfix
if (!skip_tap_cal)
#endif	//BUFFALO_MTEST_SLIDE_TAPfix
#ifdef	BUFFALO_MTEST_SLIDE_TAPloop
do
#endif	//BUFFALO_MTEST_SLIDE_TAPloop
{
/* Default tap values for starting the tap_init*/

    ar7240_reg_wr (AR7240_DDR_TAP_CONTROL0, 0x7);
    ar7240_reg_wr (AR7240_DDR_TAP_CONTROL1, 0x7);

	switch(tap_test_mode) {
	case	BUFFALO_TAP_TEST_ROW:
		{
			//
			//	COUNT UP VER
			//
			printf("=======================\n");
			printf("TAP CALC (read_cnt=%d)\n", read_cnt);
			printf("\n");
			printf("TAP  FAIL\n");
			printf("---------\n");

			for (tap_val=start_pos, tap_low=32, tap_high=0 ; tap_val<32 ; tap_val++) {
				read_fail	= ar7240_ddr_tap_init(read_cnt, tap_val, skip_pat_init);
				printf("%3d  %4d\n", tap_val, read_fail);
				if (read_fail==0) {
					if (tap_val < tap_low)	tap_low	= tap_val;
					if (tap_val > tap_high)	tap_high= tap_val;
				}
				skip_pat_init	= 1;
			}
			if (start_pos>0) {
				for (tap_val=start_pos-1 ; tap_val>=0 ; tap_val--) {
					read_fail	= ar7240_ddr_tap_init(read_cnt, tap_val, skip_pat_init);
					printf("%3d  %4d\n", tap_val, read_fail);
					if (read_fail==0) {
						if (tap_val < tap_low)	tap_low	= tap_val;
						if (tap_val > tap_high)	tap_high= tap_val;
					}
					if (tap_val==0)	break;
				}
			}
			printf("---------\n");
    	}	//case	BUFFALO_TAP_TEST_ROW:
    	break;

	case	BUFFALO_TAP_TEST_BIT:
    	{
    		unsigned long	tap_res	= 0;

			udelay(100*1000);

			for (tap_val=start_pos, tap_low=32, tap_high=0 ; tap_val<32 ; tap_val++) {
				read_fail	= ar7240_ddr_tap_init(read_cnt, tap_val, skip_pat_init);
				if (read_fail==0) {
					if (tap_val < tap_low)	tap_low	= tap_val;
					if (tap_val > tap_high)	tap_high= tap_val;
				} else {
					tap_res	|= ((unsigned long)1 << tap_val);
				}
				skip_pat_init	= 1;
			}
			if (start_pos>0) {
				for (tap_val=start_pos-1 ; tap_val>=0 ; tap_val--) {
					read_fail	= ar7240_ddr_tap_init(read_cnt, tap_val, skip_pat_init);
					if (read_fail==0) {
						if (tap_val < tap_low)	tap_low	= tap_val;
						if (tap_val > tap_high)	tap_high= tap_val;
					} else {
						tap_res	|= ((unsigned long)1 << tap_val);
					}
					if (tap_val==0)	break;
				}
			}

			//	xOOOOOOOOOOOOOOOxxxxxxxxxxxxxxxx  range[ 1-15] val[ 7, 7]
			//	xOOOOOOOOOOOOOOOxxxxxxxxxxxxxxxx  range[ 1-15] val[ 7, 7]
			//	xOOOOOOOOOOOOOOOxxxxxxxxxxxxxxxx  range[ 1-15] val[ 7, 7]
			//	xOOOOOOOOOOOOOOOxxxxxxxxxxxxxxxx  range[ 1-15] val[ 7, 7]
			//	xOOOOOOOOOOOOOOOxxxxxxxxxxxxxxxx  range[ 1-15] val[ 7, 7]
			//	xOOOOOOOOOOOOOOOxxxxxxxxxxxxxxxx  range[ 1-15] val[ 7, 7]
			for (pos=0 ; pos<32 ; pos++) {
				if (tap_res & (1<<pos))	puts("x");
				else					puts("O");
			}
			printf("  range[%2d-%2d] val[%2d,%2d]\n", tap_low, tap_high, (tap_low + tap_high)/2, (tap_low + tap_high + 1)/2);

    	}	//case	BUFFALO_TAP_TEST_BIT:
		break;

	case	BUFFALO_TAP_TEST_NORMAL:
	default:
		{
			//
			//	DISP HORIZONTAL MODE
			//
			puts("=====");
			for (pos=0 ; pos<32 ; pos++)	puts("====");
			puts("\n");

			printf("  TAP CALC (read_cnt=%d)\n", read_cnt);

			puts("----+");
			for (pos=0 ; pos<32 ; pos++)	puts("----");
			puts("\n");

			puts("TAP |");
			for (pos=0 ; pos<32 ; pos++)	printf("%4d", pos);
			puts("\n");

			puts("----+");
			for (pos=0 ; pos<32 ; pos++)	puts("----");
			puts("\n");

			puts("FAIL|");

			//	pos set [7]
			for (pos=0 ; pos<start_pos ; pos++)	puts("    ");

			for (tap_val=pos, tap_low=32, tap_high=0 ; tap_val<32 ; tap_val++) {
				read_fail	= ar7240_ddr_tap_init(read_cnt, tap_val, skip_pat_init);
				if (read_fail==0) {
					if (tap_val < tap_low)	tap_low	= tap_val;
					if (tap_val > tap_high)	tap_high= tap_val;
				}
				skip_pat_init	= 1;

				//	disp
				printf("%4d", read_fail);
			}

			//	pos set [6]
			if (start_pos > 0) {
				for (pos=tap_val ; pos>=start_pos ; pos--)	puts("\b\b\b\b");

				for (tap_val=pos /* 6 */ ; tap_val>=0 ; tap_val--) {
					read_fail	= ar7240_ddr_tap_init(read_cnt, tap_val, skip_pat_init);
					if (read_fail==0) {
						if (tap_val < tap_low)	tap_low	= tap_val;
						if (tap_val > tap_high)	tap_high= tap_val;
					}
					//	disp
					printf("%4d", read_fail);
					puts("\b\b\b\b\b\b\b\b");

					if (tap_val==0)	break;
				}
			}
			puts("\n");
			puts("----+");
			for (pos=0 ; pos<32 ; pos++)	puts("----");
			puts("\n");
    	}	//case	BUFFALO_TAP_TEST_NORMAL:
    	break;
    }

	if (tap_test_mode != BUFFALO_TAP_TEST_BIT) {
		if (tap_low > tap_high) {
			printf("########################################\n");
			printf("#                                      #\n");
			printf("#  FAIL all... use default value.      #\n");
			printf("#                                      #\n");
			printf("########################################\n");
			for (tap_low=0 ; tap_low<2000 ; tap_low++)
				udelay(1000);
			tap_low	= 7;
			tap_high= 7;
		}

		tap_val0	= (tap_low + tap_high) / 2;
		tap_val1	= (tap_low + tap_high + 1) / 2;
		printf("SELECT tap0=%d / tap1=%d\n", tap_val0, tap_val1);
	    ar7240_reg_wr (AR7240_DDR_TAP_CONTROL0, tap_val0);
	    ar7240_reg_wr (AR7240_DDR_TAP_CONTROL1, tap_val1);
		printf("\n");

	    printf("#### ddr registers\n");
	    for (tap_val=0xB8000000 ; tap_val<=0xB8000028 ; tap_val+=4)
			printf("  %08lX = %08lX\n", tap_val, ar7240_reg_rd(tap_val));

#ifdef	BUFFALO_MTEST_SLIDE_TAPloop
	    printf("\n\n");
#endif	//BUFFALO_MTEST_SLIDE_TAPloop
	}
}	//if (!skip_tap_cal)
#ifdef	BUFFALO_MTEST_SLIDE_TAPloop
while(loop_tap_cal && BUFFALO_MTEST_SLIDE_TAPloop==(ar7240_reg_rd(AR7240_GPIO_IN) & BUFFALO_MTEST_SLIDE_MASK));
#endif	//BUFFALO_MTEST_SLIDE_TAPloop
#endif	//CONFIG_BUFFALO


#ifndef	CONFIG_BUFFALO
    ar7240_usb_initial_config();
    ar7240_gpio_config();
#endif	//CONFIG_BUFFALO

    return (ar7240_ddr_find_size());
}

long int initdram(int board_type)
{
#ifndef	CONFIG_BUFFALO
    return (ar7240_mem_config());

#else	//CONFIG_BUFFALO
	long int ret	= ar7240_mem_config();

	do {
//		unsigned int slide = ar7240_reg_rd(AR7240_GPIO_IN) & BUFFALO_MTEST_SLIDE_MASK;


//		//	test
//		if ((slide==BUFFALO_MTEST_SLIDE_TEST) || (slide==BUFFALO_MTEST_SLIDE_TAPloop)) {
//			printf("watch dog reg  %08X:%08X,  %08X:%08X\n", AR7240_WATCHDOG_TMR_CONTROL, ar7240_reg_rd(AR7240_WATCHDOG_TMR_CONTROL), AR7240_WATCHDOG_TMR, ar7240_reg_rd(AR7240_WATCHDOG_TMR));
//		} else {
//			break;
//		}

		if (ar7240_reg_rd(AR7240_WATCHDOG_TMR_CONTROL) & 0x80000000) {
//			printf("skip\n");
		} else {
			printf("Force reset!!\n");
			ar7240_reg_wr_nf(AR7240_WATCHDOG_TMR_CONTROL, 0x3);
			ar7240_reg_wr_nf(AR7240_WATCHDOG_TMR, 1);
			for (;;);
		}

	} while(0);

	return	ret;
#endif	//CONFIG_BUFFALO

}

int checkboard (void)
{

    printf("AP93 (ar7240) U-boot\n");
    return 0;
}

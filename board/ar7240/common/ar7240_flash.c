#include <common.h>
#include <jffs2/jffs2.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include "ar7240_soc.h"
#include "ar7240_flash.h"

#ifdef	CONFIG_STATUS_LED
#include <status_led.h>
#endif	//CONFIG_STATUS_LED

#ifndef	CONFIG_BUFFALO		//for CAMEO Design
#error	"Need BUFFALO configuration"
#endif	//CONFIG_BUFFALO	//for CAMEO Design

/*
 * globals
 */
flash_info_t flash_info[CFG_MAX_FLASH_BANKS];

#undef display
#define display(x)  ;

/*
 * statics
 */
static void ar7240_spi_write_enable(void);
static void ar7240_spi_poll(void);
static void ar7240_spi_write_page(uint32_t addr, uint8_t *data, int len);
static void ar7240_spi_sector_erase(uint32_t addr);
#ifdef	CONFIG_BUFFALO
static void ar7240_spi_block_erase(uint32_t addr);
#else
#error
#endif	//CONFIG_BUFFALO


static void
read_id(void)
{
    u32 rd = 0x777777;

    ar7240_reg_wr_nf(AR7240_SPI_WRITE, AR7240_SPI_CS_DIS);
    ar7240_spi_bit_banger(0x9f);
    ar7240_spi_delay_8();
    ar7240_spi_delay_8();
    ar7240_spi_delay_8();
    ar7240_spi_done();
    /* rd = ar7240_reg_rd(AR7240_SPI_RD_STATUS); */
    rd = ar7240_reg_rd(AR7240_SPI_READ);
    printf("id read %#x\n", rd);
}

unsigned long
flash_init (void)
{
/*    int i;
    u32 rd = 0x666666; */

    ar7240_reg_wr_nf(AR7240_SPI_CLOCK, 0x43);
    read_id();
/*
    rd = ar7240_reg_rd(AR7240_SPI_RD_STATUS);
    printf ("rd = %x\n", rd);
    if (rd & 0x80) {
    }
*/

    /*
     * hook into board specific code to fill flash_info
     */
    return (flash_get_geom(&flash_info[0]));
}


void flash_print_info (flash_info_t *info)
{
    printf("The hell do you want flinfo for??\n");
}

int
flash_erase(flash_info_t *info, int s_first, int s_last)
{
    int i, sector_size = info->size/info->sector_count;
#ifdef	CONFIG_STATUS_LED
    int	led_cnt	= 0;
#endif	//CONFIG_STATUS_LED

    printf("\nFirst %#x last %#x sector size %#x\n",
           s_first, s_last, sector_size);

#ifdef	CONFIG_STATUS_LED
	status_led_set(STATUS_LED_DIAG, STATUS_LED_ON);
#endif	//CONFIG_STATUS_LED

#ifndef	CONFIG_BUFFALO
    for (i = s_first; i <= s_last; i++) {
        printf("\b\b\b\b%4d", i);
        ar7240_spi_sector_erase(i * sector_size);
    }
#else	//CONFIG_BUFFALO
#define	SECTORS_PER_BLOCK	(AR7240_SPI_BLOCK_SIZE/AR7240_SPI_SECTOR_SIZE)
    for (i = s_first; (i % SECTORS_PER_BLOCK) && i <= s_last; i++) {
        printf("\b\b\b\b%4d", i);
//		printf("erasing sector %d\n", i);
        ar7240_spi_sector_erase(i * sector_size);
#ifdef	CONFIG_STATUS_LED
		status_led_set(STATUS_LED_DIAG, (led_cnt++ & 0x01) ? STATUS_LED_ON : STATUS_LED_OFF);
#endif	//CONFIG_STATUS_LED
    }
    for ( ; (i + SECTORS_PER_BLOCK-1) <= s_last ; i+=SECTORS_PER_BLOCK) {
        printf("\b\b\b\b%4d", i);
//		printf("erasing block %d-%d\n", i, i+SECTORS_PER_BLOCK-1);
        ar7240_spi_block_erase(i * sector_size);
#ifdef	CONFIG_STATUS_LED
		status_led_set(STATUS_LED_DIAG, (led_cnt++ & 0x01) ? STATUS_LED_ON : STATUS_LED_OFF);
#endif	//CONFIG_STATUS_LED
    }
    for ( ; i <= s_last; i++) {
        printf("\b\b\b\b%4d", i);
//		printf("erasing sector %d\n", i);
        ar7240_spi_sector_erase(i * sector_size);
#ifdef	CONFIG_STATUS_LED
		status_led_set(STATUS_LED_DIAG, (led_cnt++ & 0x01) ? STATUS_LED_ON : STATUS_LED_OFF);
#endif	//CONFIG_STATUS_LED
    }
#endif	//CONFIG_BUFFALO
    ar7240_spi_done();
    printf("\n");

#ifdef	CONFIG_STATUS_LED
	status_led_set(STATUS_LED_DIAG, STATUS_LED_OFF);
#endif	//CONFIG_STATUS_LED

    return 0;
}

/*
 * Write a buffer from memory to flash:
 * 0. Assumption: Caller has already erased the appropriate sectors.
 * 1. call page programming for every 256 bytes
 */
int
write_buff(flash_info_t *info, uchar *source, ulong addr, ulong len)
{
    int total = 0, len_this_lp, bytes_this_page;
    ulong dst;
    uchar *src;
#ifdef	CONFIG_STATUS_LED
    int	led_cnt	= 0;
#endif	//CONFIG_STATUS_LED

    printf ("write addr: %x\n", addr);
    addr = addr - CFG_FLASH_BASE;

#ifdef	CONFIG_STATUS_LED
	status_led_set(STATUS_LED_DIAG, STATUS_LED_ON);
#endif	//CONFIG_STATUS_LED

    while(total < len) {
        src              = source + total;
        dst              = addr   + total;
        bytes_this_page  = AR7240_SPI_PAGE_SIZE - (addr % AR7240_SPI_PAGE_SIZE);
        len_this_lp      = ((len - total) > bytes_this_page) ? bytes_this_page
                                                             : (len - total);
        ar7240_spi_write_page(dst, src, len_this_lp);
        total += len_this_lp;
#ifdef	CONFIG_STATUS_LED
        if (!(total & 0xFFFF))
			status_led_set(STATUS_LED_DIAG, (led_cnt++ & 0x01) ? STATUS_LED_ON : STATUS_LED_OFF);
#endif	//CONFIG_STATUS_LED
    }

    ar7240_spi_done();
#ifdef	CONFIG_STATUS_LED
	status_led_set(STATUS_LED_DIAG, STATUS_LED_OFF);
#endif	//CONFIG_STATUS_LED

    return 0;
}

static void
ar7240_spi_write_enable()
{
    ar7240_reg_wr_nf(AR7240_SPI_FS, 1);
    ar7240_reg_wr_nf(AR7240_SPI_WRITE, AR7240_SPI_CS_DIS);
    ar7240_spi_bit_banger(AR7240_SPI_CMD_WREN);
    ar7240_spi_go();
}

static void
ar7240_spi_poll()
{
    int rd;

    do {
        ar7240_reg_wr_nf(AR7240_SPI_WRITE, AR7240_SPI_CS_DIS);
        ar7240_spi_bit_banger(AR7240_SPI_CMD_RD_STATUS);
        ar7240_spi_delay_8();
        rd = (ar7240_reg_rd(AR7240_SPI_RD_STATUS) & 1);
    }while(rd);
}

static void
ar7240_spi_write_page(uint32_t addr, uint8_t *data, int len)
{
    int i;
    uint8_t ch;

    display(0x77);
    ar7240_spi_write_enable();
    ar7240_spi_bit_banger(AR7240_SPI_CMD_PAGE_PROG);
    ar7240_spi_send_addr(addr);

    for(i = 0; i < len; i++) {
        ch = *(data + i);
        ar7240_spi_bit_banger(ch);
    }

    ar7240_spi_go();
    display(0x66);
    ar7240_spi_poll();
    display(0x6d);
}

static void
ar7240_spi_sector_erase(uint32_t addr)
{
//	printf("sector erase %08lX\n", (unsigned long)addr);
    ar7240_spi_write_enable();
    ar7240_spi_bit_banger(AR7240_SPI_CMD_SECTOR_ERASE);
    ar7240_spi_send_addr(addr);
    ar7240_spi_go();
    display(0x7d);
    ar7240_spi_poll();
}

#ifdef	CONFIG_BUFFALO
static void
ar7240_spi_block_erase(uint32_t addr)
{
//	printf("block erase %08lX\n", (unsigned long)addr);
    ar7240_spi_write_enable();
    ar7240_spi_bit_banger(AR7240_SPI_CMD_BLOCK_ERASE);
    ar7240_spi_send_addr(addr);
    ar7240_spi_go();
    display(0x7d);
    ar7240_spi_poll();
}
#endif	//CONFIG_BUFFALO



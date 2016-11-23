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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <status_led.h>

#ifdef	CONFIG_BUFFALO
#ifdef	CONFIG_AR7100
#include <asm/addrspace.h>
#include "ar7100_soc.h"
#endif
#ifdef	CONFIG_AR7240
#include <asm/addrspace.h>
#include "ar7240_soc.h"
#endif
#endif	//CONFIG_BUFFALO

/*
 * The purpose of this code is to signal the operational status of a
 * target which usually boots over the network; while running in
 * U-Boot, a status LED is blinking. As soon as a valid BOOTP reply
 * message has been received, the LED is turned off. The Linux
 * kernel, once it is running, will start blinking the LED again,
 * with another frequency.
 */

#ifdef CONFIG_BUFFALO
unsigned int last_er_val;

#endif	//CONFIG_BUFFALO

/* ------------------------------------------------------------------------- */

#ifdef CONFIG_STATUS_LED

typedef struct {
	led_id_t mask;
	int state;
	int period;
#ifdef	CONFIG_BUFFALO
	int active_low;
	int cur_state;
#endif	//CONFIG_BUFFALO
	int cnt;
} led_dev_t;

led_dev_t led_dev[] = {
    {	STATUS_LED_BIT,
	STATUS_LED_STATE,
	STATUS_LED_PERIOD,
#ifdef	CONFIG_BUFFALO
	STATUS_LED_ACTIVE_LOW,
	0,
#endif	//CONFIG_BUFFALO
	0,
    },
#if defined(STATUS_LED_BIT1)
    {	STATUS_LED_BIT1,
	STATUS_LED_STATE1,
	STATUS_LED_PERIOD1,
#ifdef	CONFIG_BUFFALO
	STATUS_LED_ACTIVE_LOW1,
	0,
#endif	//CONFIG_BUFFALO
	0,
    },
#endif
#if defined(STATUS_LED_BIT2)
    {	STATUS_LED_BIT2,
	STATUS_LED_STATE2,
	STATUS_LED_PERIOD2,
#ifdef	CONFIG_BUFFALO
	STATUS_LED_ACTIVE_LOW2,
	0,
#endif	//CONFIG_BUFFALO
	0,
    },
#endif
#if defined(STATUS_LED_BIT3)
    {	STATUS_LED_BIT3,
	STATUS_LED_STATE3,
	STATUS_LED_PERIOD3,
#ifdef	CONFIG_BUFFALO
	STATUS_LED_ACTIVE_LOW3,
	0,
#endif	//CONFIG_BUFFALO
	0,
    },
#endif
#if defined(STATUS_LED_BIT4)
    {	STATUS_LED_BIT4,
	STATUS_LED_STATE4,
	STATUS_LED_PERIOD4,
#ifdef	CONFIG_BUFFALO
	STATUS_LED_ACTIVE_LOW4,
	0,
#endif	//CONFIG_BUFFALO
	0,
    },
#endif
#if defined(STATUS_LED_BIT5)
    {	STATUS_LED_BIT5,
	STATUS_LED_STATE5,
	STATUS_LED_PERIOD5,
#ifdef	CONFIG_BUFFALO
	STATUS_LED_ACTIVE_LOW5,
	0,
#endif	//CONFIG_BUFFALO
	0,
    },
#endif
};

#define MAX_LED_DEV	(sizeof(led_dev)/sizeof(led_dev_t))

static int status_led_init_done = 0;

#ifdef	CONFIG_BUFFALO

static inline void __led_init()
{

}

static inline void __led_toggle(led_id_t mask)
{

}

static inline void __led_set(led_id_t mask, int state)
{

	unsigned int new_er_val;

	new_er_val = mask;

#ifdef	CONFIG_AR7100
	switch(state){
		case STATUS_LED_ON:
			ar7100_reg_wr(AR7100_GPIO_SET, mask);
			break;
		case STATUS_LED_BLINKING:
			ar7100_reg_wr(AR7100_GPIO_SET, mask & (0xFFFFFF ^ ar7100_reg_rd(AR7100_GPIO_IN)));
			ar7100_reg_wr(AR7100_GPIO_CLEAR, mask & ar7100_reg_rd(AR7100_GPIO_IN));
			break;
		case STATUS_LED_OFF:
		default:
			ar7100_reg_wr(AR7100_GPIO_CLEAR, mask);
			break;
	}
#endif	//CONFIG_AR7100

#ifdef	CONFIG_AR7240
	switch(state){
		case STATUS_LED_ON:
			ar7240_reg_wr(AR7240_GPIO_SET, mask);
			break;
		case STATUS_LED_BLINKING:
			ar7240_reg_wr(AR7240_GPIO_SET, mask & (0xFFFFFF ^ ar7240_reg_rd(AR7240_GPIO_IN)));
			ar7240_reg_wr(AR7240_GPIO_CLEAR, mask & ar7240_reg_rd(AR7240_GPIO_IN));
			break;
		case STATUS_LED_OFF:
		default:
			ar7240_reg_wr(AR7240_GPIO_CLEAR, mask);
			break;
	}
#endif	//CONFIG_AR7240

#if	defined(CONFIG_RT2880) || defined(CONFIG_RT3052)

	switch(state){
		case STATUS_LED_ON:
			RT2882_REG(RT2880_PIODATA_REG) = RT2882_REG(RT2880_PIODATA_REG) | mask;
			break;
		case STATUS_LED_OFF:
		case STATUS_LED_BLINKING:
		default:
			RT2882_REG(RT2880_PIODATA_REG) = RT2882_REG(RT2880_PIODATA_REG)&(0xFFFF^mask);
			break;
	}
#endif	//defined(CONFIG_RT2880) || defined(CONFIG_RT3052)


	last_er_val = new_er_val;
}

static inline void __led_blink_set(led_id_t mask, int state)
{

}
#endif	//CONFIG_BUFFALO


static void status_led_init (void)
{
#ifdef	CONFIG_BUFFALO
	__led_init();

	led_dev[0].period = (CFG_HZ / 10);
	led_dev[1].period = (CFG_HZ / 50);
	led_dev[2].period = (CFG_HZ / 10);

#else
	led_dev_t *ld;
	int i;

	for (i = 0, ld = led_dev; i < MAX_LED_DEV; i++, ld++)
		__led_init (ld->mask, ld->state);

#endif	//CONFIG_BUFFALO
	status_led_init_done = 1;
}

void status_led_tick (ulong timestamp)
{
	led_dev_t *ld;
	int i;

	if (!status_led_init_done)
		status_led_init ();

	for (i = 0, ld = led_dev; i < MAX_LED_DEV; i++, ld++) {

		if (ld->state != STATUS_LED_BLINKING)
			continue;

		if (++ld->cnt >= ld->period) {
#ifndef	CONFIG_BUFFALO
			__led_toggle (ld->mask);
#else	//CONFIG_BUFFALO
			__led_set (ld->mask, (ld->cur_state==STATUS_LED_ON?STATUS_LED_OFF:STATUS_LED_ON));
#endif	//CONFIG_BUFFALO
			ld->cnt -= ld->period;
		}

	}
}

void status_led_set (int led, int state)
{
	led_dev_t *ld;

	if (led < 0 || led >= MAX_LED_DEV)
		return;

	if (!status_led_init_done)
		status_led_init ();

	ld = &led_dev[led];

	ld->state = state;
	if (state == STATUS_LED_BLINKING) {
		ld->cnt = 0;		/* always start with full period    */
		state = STATUS_LED_ON;	/* always start with LED _ON_       */
	}
#ifdef	CONFIG_BUFFALO
	ld->cur_state = state;
	if(ld->active_low){
		state = (state)? STATUS_LED_OFF : STATUS_LED_ON;
	}
#endif	//CONFIG_BUFFALO
	//printf("__led_set(mask[%X], state[%d])\n", ld->mask, state);
	__led_set (ld->mask, state);
}

#ifdef	CONFIG_BUFFALO
void status_led_blink_set (int led, int state)
{
	led_dev_t *ld;

	if (led < 0 || led >= MAX_LED_DEV)
		return;

	if (!status_led_init_done)
		status_led_init ();

	ld = &led_dev[led];

	ld->state = state;
	if (state == STATUS_LED_BLINKING) {
		ld->cnt = 0;		/* always start with full period    */
		state = STATUS_LED_ON;	/* always start with LED _ON_       */
	}
	printf(" # LED Blink(0x%x)\n",ld->mask);
	__led_blink_set (ld->mask, state);
}

void status_led_blink_num_set (int led, int num)
{
	led_dev_t *ld;
	int state_on=STATUS_LED_ON, state_off=STATUS_LED_OFF;

	if (led < 0 || led >= MAX_LED_DEV)
		return;

	if (!status_led_init_done)
		status_led_init ();

	ld = &led_dev[led];
	if(ld->active_low){
		state_on=STATUS_LED_OFF;
		state_off=STATUS_LED_ON;
	}

	printf(" # LED(0x%x) Blink[%d] (Please press 'Ctrl+c' to stop)\n",ld->mask,num);
	for (;;) {
		int i,wait;
		if (ctrlc()) {
			putc ('\n');
			return;
		}
		for(i=0; i<num; i++){
			ld->cur_state = STATUS_LED_ON;
			__led_set (ld->mask, state_on);
			for(wait=0; wait<250; wait++){udelay(1000);}
			ld->cur_state = STATUS_LED_OFF;
			__led_set (ld->mask, state_off);
			for(wait=0; wait<250; wait++){udelay(1000);}
		}
		for(wait=0; wait<1750; wait++){udelay(1000);}
	}
}
#endif	//CONFIG_BUFFALO

#endif	/* CONFIG_STATUS_LED */

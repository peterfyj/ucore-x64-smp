#include <clock.h>
#include <board.h>
#include <stdio.h>
#include <or32.h>
#include <or32/spr_defs.h>

volatile size_t ticks = 0;

void
clock_init (void)
{
	cprintf ("++ setup timer interrupts\n");
	
	mtspr (SPR_TTMR, TIMER_FREQ | SPR_TTMR_IE | SPR_TTMR_RT);
	mtspr (SPR_TTCR, 0);
}

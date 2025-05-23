#include <platform.h>
#include <stdint.h>
#include "delay.h"
//#include "core_cm4.h" // Or core_cmInstr.h depending on setup

void delay_ms(unsigned int ms) {
	unsigned int max_step = 1000 * (UINT32_MAX / CLK_FREQ);
	unsigned int max_sleep_cycles = max_step * (CLK_FREQ / 1000);
	while (ms > max_step) {
		ms -= max_step;
		delay_cycles(max_sleep_cycles);
	}
	delay_cycles(ms * (CLK_FREQ / 1000));
}

void delay_us(unsigned int us) {
	unsigned int max_step = 1000000 * (UINT32_MAX / CLK_FREQ);
	unsigned int max_sleep_cycles = max_step * (CLK_FREQ / 1000000);
	while (us > max_step) {
		us -= max_step;
		delay_cycles(max_sleep_cycles);
	}
	delay_cycles(us * (CLK_FREQ / 1000000));
}

__asm void delay_cycles(unsigned int cycles) {
	LSRS r0, #2
	BEQ done
loop
	SUBS r0, #1
#if __CORTEX_M == 3 || __CORTEX_M == 4
	NOP
#endif
	BNE loop
done
	BX lr
}

volatile uint32_t tick_count = 0;

void SysTick_Handler(void) {
    tick_count++;
}

void delay_ms_low_power(uint32_t ms) {
    tick_count = 0;
    SysTick_Config(SystemCoreClock / 1000); // 1ms tick
    while (tick_count < ms) {
        __WFI(); 
    }
    SysTick->CTRL = 0; // Disable SysTick after use
}

void delay_100us_low_power(uint32_t us100) {
    tick_count = 0;
    SysTick_Config(SystemCoreClock / 10000); // 100us tick
    while (tick_count < us100) {
        __WFI(); 
    }
    SysTick->CTRL = 0; // Disable SysTick after use
}
// *******************************ARM University Program Copyright © ARM Ltd 2014*************************************   

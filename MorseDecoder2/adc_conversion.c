#include "platform.h"       // Provides CLK_FREQ, ADC_MASK, and pin definitions (e.g., P_ADC)
#include "adc.h"            // ADC driver functions: adc_init() and adc_read()
#include "gpio.h"           // GPIO functions: gpio_set_mode() and gpio_set()
#include "delay.h"          // Delay functions: delay_ms(), delay_us(), etc.
#include "lcd.h"            // LCD driver functions: lcd_init(), lcd_clear(), lcd_print(), lcd_set_cursor()
#include "adc_conversion.h"
#include <stdio.h>          // For sprintf()

#define VREF 3.3            // ADC reference voltage in volts

// Use the red LED for feedback. If P_LED_R is not defined in platform.h, we define it here.
// Adjust this pin if your board uses a different one.
#ifndef P_LED_R
  #define P_LED_R P1_11
#endif

// For an active-low LED, setting the pin low turns it on.
#define LED_ON  0
#define LED_OFF 1

/**
 * @brief A simple busy-loop delay function (in milliseconds).
 *
 * This function uses nested loops. You might need to adjust the inner loop count
 * based on your system clock.
 */
static void simple_delay_ms(unsigned int ms) {
    volatile unsigned int i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 10000; j++) {
            __asm("NOP");
        }
    }
}

/**
 * @brief Blinks the feedback LED for approximately 10 ms.
 */
static void blink_feedback_led(void) {
    gpio_set(P_LED_R, LED_ON);
    simple_delay_ms(10);
    gpio_set(P_LED_R, LED_OFF);
}

/**
 * @brief Runs the ADC conversion routine.
 *
 * Initializes the ADC, LCD, and feedback LED.
 * Displays "System Running" on the first line of the LCD.
 * In the main loop, it reads the ADC value (from pin 15 / P_ADC),
 * blinks the LED to indicate a conversion event,
 * scales the ADC value to a voltage (using a 3.3V reference),
 * and displays both the raw ADC value and the voltage on the second line of the LCD.
 */
void run_adc_conversion(void) {
    char msg[32];

    // Initialize ADC: this sets up the ADC peripheral and configures the analog input pin.
    adc_init();

    // Initialize the LCD and display a header message.
    lcd_init();
    lcd_clear();
    lcd_print("System Running");

    // Configure the feedback LED pin as output and set it off initially.
    gpio_set_mode(P_LED_R, Output);
    gpio_set(P_LED_R, LED_OFF);

    // Main loop: continuously perform ADC conversion.
    while (1) {
        // Read the ADC value (12-bit result, range 0 to ADC_MASK, typically 4095).
        int adc_val = adc_read();

        // Blink the feedback LED to indicate an ADC conversion event.
        blink_feedback_led();

        // Scale the ADC value to a voltage.
        float voltage = adc_val * (VREF / ADC_MASK);

        // Format the ADC value and voltage into a string.
        sprintf(msg, "ADC:%d V:%.2f", adc_val, voltage);

        // Set the LCD cursor to the second line (row 1) and display the message.
        lcd_set_cursor(0, 1);
        lcd_print(msg);

        // Delay 100 ms between conversions.
        simple_delay_ms(100);
    }
}

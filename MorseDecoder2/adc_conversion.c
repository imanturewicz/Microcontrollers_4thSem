#include "platform.h"       // Provides CLK_FREQ, ADC_MASK, and pin definitions (e.g., P_ADC)
#include "adc.h"            // ADC driver functions: adc_init() and adc_read()
#include "gpio.h"           // GPIO functions: gpio_set_mode() and gpio_set()
#include "delay.h"          // Delay functions: delay_ms(), delay_us(), etc.
#include "lcd.h"            // LCD driver functions: lcd_init(), lcd_clear(), lcd_print(), lcd_set_cursor()
#include "adc_conversion.h"
#include <stdio.h>          // For sprintf()
#include <string.h>         // For string operations

#define VREF 3.3            // ADC reference voltage in volts

// Use the red LED for feedback. If P_LED_R is not defined in platform.h, we define it here.
// Adjust this pin if your board uses a different one.
#ifndef P_LED_R
  #define P_LED_R P1_11
#endif

// For an active-low LED, setting the pin low turns it on.
#define LED_ON  0
#define LED_OFF 1

// Smoothing factor for the IIR filter
#define ALPHA 0.1f

// Global variable for filtered ADC value.
static float filtered_adc = 0.0f;

/**
 * @brief Updates the IIR filter with the latest ADC sample.
 *
 * Uses the formula:
 *     filtered_adc = ALPHA * raw_adc + (1 - ALPHA) * filtered_adc
 *
 * @param raw_adc The latest raw ADC sample.
 * @return The updated filtered ADC value.
 */
static float update_iir_filter(int raw_adc) {
    filtered_adc = ALPHA * raw_adc + (1.0f - ALPHA) * filtered_adc;
    return filtered_adc;
}
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

    // Initialize ADC.
    adc_init();

    // Initialize LCD and display a startup message on the first line.
    lcd_init();
    lcd_clear();
    lcd_print("System Running");

    // Configure the feedback LED as output and turn it off.
    gpio_set_mode(P_LED_R, Output);
    gpio_set(P_LED_R, LED_OFF);

    // Initialize the filter with the first ADC reading.
    filtered_adc = (float)adc_read();

    // Demodulation configuration
    #define THRESHOLD       50
    #define BASE						2000
    #define DOT_DURATION     2
    #define DASH_DURATION    5
    #define DOT_GAP						1
    #define SYMBOL_GAP       6
    #define WORD_GAP         14

    int tone_duration = 0;
    int silence_duration = 0;
    int is_tone = 0;

    char demod_buffer[128];
    int demod_index = 0;

    char current_symbol[16];        // holds current group of . and - forming a Morse character/word
    int current_symbol_index = 0;   // current position in that buffer

    while (1) {
        
        // Read the raw ADC value (expected range: 0 to ADC_MASK, typically 0 to 4095)
        int raw_adc = adc_read();

        // Update the filtered ADC value using our IIR filter.
        float filt = update_iir_filter(raw_adc);

        // Blink the feedback LED to indicate a conversion event.
        blink_feedback_led();

        // Demodulation logic
        int signal_active = (raw_adc> (BASE+THRESHOLD)/* || raw_adc < (BASE-THRESHOLD)*/);

        // Measuring how long the tone lasts
        if (signal_active) {
            tone_duration++;            //counts how long the tone is playing
            silence_duration = 0;   //Reset silence time since active signal.

            if (!is_tone) {
                is_tone = 1;						//if we weren't in tone before, now we are
            }

        } else {
            silence_duration++; 

            if (is_tone) {
                if (tone_duration >= 2 && tone_duration < DASH_DURATION) {  
                    if (current_symbol_index < sizeof(current_symbol) - 1) {
                        current_symbol[current_symbol_index++] = '.';  //  add to symbol group
                    }
                } else if (tone_duration >= DASH_DURATION) { // dash detected
                    if (current_symbol_index < sizeof(current_symbol) - 1) {
                        current_symbol[current_symbol_index++] = '-';  // add to symbol group
                    }
                }
                tone_duration = 0;   // clean reset after tone ends(dot or dash)
                is_tone = 0;       // mark that we're in silence now
            }

            if (silence_duration == SYMBOL_GAP || silence_duration == WORD_GAP) {
                // Print current symbol group
                current_symbol[current_symbol_index] = '\0';  // Null-terminate string
						    
						
                lcd_set_cursor(0, 0);
                lcd_print("                "); // Clear LCD line

                lcd_set_cursor(0, 0);
                sprintf(msg, "Word: %s", current_symbol);
                lcd_print(msg);
							
							{
							// transltion logic  ?
							}

                // Append current_symbol to demod_buffer
                int len = strlen(current_symbol);
                if (demod_index + len < sizeof(demod_buffer) - 2) {   // makek sure there is room in demod_buffer[]
                    for (int i = 0; i < len; i++) {
                        demod_buffer[demod_index++] = current_symbol[i];
                    }
                    // Add space or slash for gap type
                    demod_buffer[demod_index++] = (silence_duration == WORD_GAP) ? '/' : ' ';
                    demod_buffer[demod_index] = '\0'; // Null terminate
                }

                current_symbol_index = 0;  //  Reset symbol buffer
            }
        }

        // Null-terminate the buffer for safety
        if (demod_index < sizeof(demod_buffer)) {
            demod_buffer[demod_index] = '\0';
        }

        // Clear the LCD's second line by printing 16 spaces.
        lcd_set_cursor(0, 1);
        lcd_print("                "); // 16 spaces

        // Set the cursor again and format the message.
        lcd_set_cursor(0, 1);
        sprintf(msg, "R:%d F:%d", raw_adc, (int)(filt + 0.5f));
        lcd_print(msg);

        // Delay for  ms before the next reading.
        delay_ms(100);
    }
}

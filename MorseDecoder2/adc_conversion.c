#include "platform.h"       // Provides CLK_FREQ, ADC_MASK, and pin definitions (e.g., P_ADC)
#include "adc.h"            // ADC driver functions: adc_init() and adc_read()
#include "gpio.h"           // GPIO functions: gpio_set_mode() and gpio_set()
#include "delay.h"          // Delay functions: delay_ms(), delay_us(), etc.
#include "lcd.h"            // LCD driver functions: lcd_init(), lcd_clear(), lcd_print(), lcd_set_cursor()
#include "adc_conversion.h"
#include <stdio.h>          // For sprintf()
#include <string.h>         // For string operations
#include "switches.h"

#define VREF 3.3            // ADC reference voltage in volts

#ifndef P_LED_R
  #define P_LED_R P1_11
#endif

#define LED_ON  0
#define LED_OFF 1
#define ALPHA 0.1f
#define CLK 30
#define THRESHOLD 50
    #define BASE 2000
    #define DOT_DURATION 1
    #define DASH_DURATION 3
    #define SYMBOL_GAP 3
    #define WORD_GAP 7




int calc_movingAverage() {
    long long sum = 0;
    for(int i = 0; i < CLK; i++) {
        sum += adc_read();
				delay_100us_low_power(1);
    }
    return sum / CLK;
}

char morse_to_char(const char* symbol) {
    struct {
        const char* code;
        char letter;
    } morse_table[] = {
        {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {"." , 'E'},
        {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'},
        {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'},
        {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
        {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'}, {"--..", 'Z'},
        {".----", '1'}, {"..---", '2'}, {"...--", '3'}, {"....-", '4'}, {".....", '5'},
        {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'}, {"-----", '0'},
        {".-.-.-", '.'}, {"--..--", ','}, {"---...", ':'}, {"..--..", '?'},
        {".----.", '\''}, {"-....-", '-'}, {"-..-.", '/'}, {"-.--.", '('}, {"-.--.-", ')'},
        {".-..-.", '"'}, {"-...-", '='}, {".-.-.", '+'}, {"...-.-", '!'},
        {"-.-", '^'}, {".-...", '~'}, {"........", '#'}, {"...-.", '_'},
        {".--.-.", '@'}, {"-..-", '*'}
    };

    for (int i = 0; i < sizeof(morse_table)/sizeof(morse_table[0]); i++) {
        if (strcmp(symbol, morse_table[i].code) == 0) {
            return morse_table[i].letter;
        }
    }
    return '#';
}

int wait_for_start_signal(void) {

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Waiting for the signal...");

    while (1) {
        int averagedSample = calc_movingAverage();
        int signal_active = (averagedSample > (BASE + THRESHOLD));

        if (signal_active) {
            return 0; // Signal detected, exit waiting loop
        }
		
        delay_ms_low_power(1); // Wait for a short time before checking again
    }

    return 0;
}


void run_adc_conversion(void) {
		
    char sentence[128] = "";
    int sentence_index = 0;
    char msg[32];

    adc_init();
    lcd_init();
	switches_init();

    wait_for_start_signal();

    lcd_clear();
    gpio_set_mode(P_LED_R, Output);
    gpio_set(P_LED_R, LED_OFF);
	


    int tone_duration = 0;
    int silence_duration = 0;
    int is_tone = 0;

    char demod_buffer[128];
    int demod_index = 0;
    char current_symbol[16];
    int current_symbol_index = 0;

    while (1) {
				if (switch_get(P_SW_CR)) {
					lcd_clear();
					sentence_index = 0;
					sentence[0] = '\0';
					delay_ms_low_power(2);
				}

        int averagedSample = calc_movingAverage();
       
        int signal_active = (averagedSample > (BASE + THRESHOLD));

        if (signal_active) {
            tone_duration++;
            silence_duration = 0;
            if (!is_tone) is_tone = 1;
        } else {
            silence_duration++;

            if (is_tone) {
                if (tone_duration >= DOT_DURATION && tone_duration < DASH_DURATION) {
                    if (current_symbol_index < sizeof(current_symbol) - 1)
                        current_symbol[current_symbol_index++] = '.';
                } else if (tone_duration >= DASH_DURATION) {
                    if (current_symbol_index < sizeof(current_symbol) - 1)
                        current_symbol[current_symbol_index++] = '-';
                }
                tone_duration = 0;
                is_tone = 0;
            }

            if (silence_duration == SYMBOL_GAP || silence_duration == WORD_GAP) {
                current_symbol[current_symbol_index] = '\0';

                lcd_set_cursor(0, 0);
                lcd_print("                ");
                lcd_set_cursor(0, 0);
                sprintf(msg, "Word: %s", current_symbol);
                lcd_print(msg);

                if (current_symbol[0] != '\0') {
                    char translated = morse_to_char(current_symbol);

                    if (translated == '#') {
                        gpio_set(P_LED_R, LED_ON);
											#
										}
						
										if (sentence_index < sizeof(sentence) - 1) {
                            sentence[sentence_index++] = translated;
                            sentence[sentence_index] = '\0';
                    }

                    current_symbol_index = 0;
                    current_symbol[0] = '\0';

                } else if (silence_duration == WORD_GAP) {
                    if (sentence_index < sizeof(sentence) - 1) {
                        sentence[sentence_index++] = ' ';
                        sentence[sentence_index] = '\0';
                    }
                }

                int len = strlen(sentence);
                const char* display_ptr = sentence;
                if (len > 16) {
                    display_ptr = sentence + (len - 16);
                }
                lcd_set_cursor(0, 1);
                lcd_print(display_ptr);

                int len_sym = strlen(current_symbol);
                if (demod_index + len_sym < sizeof(demod_buffer) - 2) {
                    for (int i = 0; i < len_sym; i++) {
                        demod_buffer[demod_index++] = current_symbol[i];
                    }
                    demod_buffer[demod_index++] = (silence_duration == WORD_GAP) ? '/' : ' ';
                    demod_buffer[demod_index] = '\0';
                }

                current_symbol_index = 0;
            }
						else if (silence_duration >= 2*WORD_GAP) {
									break;
						}
        } 

        if (demod_index < sizeof(demod_buffer)) {
            demod_buffer[demod_index] = '\0';
        }
				delay_ms_low_power(14);
    }
}
#ifndef ADC_CONVERSION_H
#define ADC_CONVERSION_H

/**
 * @brief Runs the ADC conversion routine.
 *
 * Initializes the ADC, LCD, and feedback LED.
 * Then continuously reads the analog signal from pin 15 (P_ADC),
 * blinks the LED to signal a conversion event, scales the ADC reading to a voltage,
 * and displays the ADC value and voltage on the LCD.
 */
void run_adc_conversion(void);

#endif // ADC_CONVERSION_H

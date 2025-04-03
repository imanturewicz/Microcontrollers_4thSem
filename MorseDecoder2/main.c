#include "adc_conversion.h"

int main(void) {
    // Start the ADC conversion routine. This function never returns.
    run_adc_conversion();
    
    // Embedded applications should not exit main.
    return 0;
}

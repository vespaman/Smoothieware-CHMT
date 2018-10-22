/* mbed Library - ADC
 * Copyright (c) 2010, sblandford
 * released under MIT license http://mbed.org/licence/mit
 */

#ifndef MBED_ADC_H
#define MBED_ADC_H

#include "PinNames.h" // mbed.h lib

#define CLKS_PER_SAMPLE 64

#define ADC_CHANNEL_COUNT   16

namespace mbed {
class ADC {
public:

    //Initialize ADC with ADC maximum sample rate of
    //sample_rate and system clock divider of cclk_div
    //Maximum recommened sample rate is 184000
    ADC(int sample_rate, int cclk_div);

    //Enable/disable ADC on pin according to state
    //and also select/de-select for next conversion
    uint8_t setup(PinName pin, int state);

    //Enable/disable burst mode according to state
    void burst(int state);

    //Return burst mode enabled/disabled
    int burst(void);

    //Set interrupt enable/disable for pin to state
    void interrupt_state(PinName pin, int state);

    //Append custom global interrupt handler
    void append(void(*fptr)(int chan, uint32_t value));

    uint8_t _pin_to_channel(PinName pin);

private:
    uint8_t scan_chan_lut[ADC_CHANNEL_COUNT];
    uint8_t scan_count;
    uint8_t scan_index;

    uint32_t interrupt_mask;

    uint32_t _data_of_pin(PinName pin);

    void adcisr(void);
    static void _adcisr(void);
    static ADC *instance;

    void(*_adc_g_isr)(int chan, uint32_t value);
};
}

#endif

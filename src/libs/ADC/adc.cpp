/* mbed Library - ADC
 * Copyright (c) 2010, sblandford
 * released under MIT license http://mbed.org/licence/mit
 */

#include "stm32f407xx.h"
#undef ADC

#include "mbed.h"
#include "adc.h"
#include "mri.h"

#include "pinmap.h"
#include "PeripheralPins.h"

#define STM_ADC ADC1

using namespace mbed;

ADC *ADC::instance;

ADC::ADC(int sample_rate, int cclk_div)
{
    scan_count = 0;
    scan_index = 0;

    memset(scan_chan_lut, 0xFF, sizeof(scan_chan_lut));
/*
    int i, adc_clk_freq, pclk, clock_div, max_div=1;

    //Work out CCLK
    adc_clk_freq=CLKS_PER_SAMPLE*sample_rate;
    int m = (LPC_SC->PLL0CFG & 0xFFFF) + 1;
    int n = (LPC_SC->PLL0CFG >> 16) + 1;
    int cclkdiv = LPC_SC->CCLKCFG + 1;
    int Fcco = (2 * m * XTAL_FREQ) / n;
    int cclk = Fcco / cclkdiv;

    //Power up the ADC
    LPC_SC->PCONP |= (1 << 12);
    //Set clock at cclk / 1.
    LPC_SC->PCLKSEL0 &= ~(0x3 << 24);
    switch (cclk_div) {
        case 1:
            LPC_SC->PCLKSEL0 |= 0x1 << 24;
            break;
        case 2:
            LPC_SC->PCLKSEL0 |= 0x2 << 24;
            break;
        case 4:
            LPC_SC->PCLKSEL0 |= 0x0 << 24;
            break;
        case 8:
            LPC_SC->PCLKSEL0 |= 0x3 << 24;
            break;
        default:
            printf("ADC Warning: ADC CCLK clock divider must be 1, 2, 4 or 8. %u supplied.\n",
                cclk_div);
            printf("Defaulting to 1.\n");
            LPC_SC->PCLKSEL0 |= 0x1 << 24;
            break;
    }
    pclk = cclk / cclk_div;
    clock_div=pclk / adc_clk_freq;

    if (clock_div > 0xFF) {
        printf("ADC Warning: Clock division is %u which is above 255 limit. Re-Setting at limit.\n", clock_div);
        clock_div=0xFF;
    }
    if (clock_div == 0) {
        printf("ADC Warning: Clock division is 0. Re-Setting to 1.\n");
        clock_div=1;
    }

    _adc_clk_freq=pclk / clock_div;
    if (_adc_clk_freq > MAX_ADC_CLOCK) {
        printf("ADC Warning: Actual ADC sample rate of %u which is above %u limit\n",
            _adc_clk_freq / CLKS_PER_SAMPLE, MAX_ADC_CLOCK / CLKS_PER_SAMPLE);
        while ((pclk / max_div) > MAX_ADC_CLOCK) max_div++;
        printf("ADC Warning: Maximum recommended sample rate is %u\n", (pclk / max_div) / CLKS_PER_SAMPLE);
    }

    LPC_ADC->ADCR =
        ((clock_div - 1 ) << 8 ) |    //Clkdiv
        ( 1 << 21 );                  //A/D operational

    //Default no channels enabled
    LPC_ADC->ADCR &= ~0xFF;
    //Default NULL global custom isr
    //Initialize arrays
    for (i=7; i>=0; i--) {
        _adc_data[i] = 0;
        _adc_isr[i] = NULL;
    }


    // Attach IRQ
    NVIC_SetVector(ADC_IRQn, (uint32_t)&_adcisr);

    //Disable global interrupt
    LPC_ADC->ADINTEN &= ~0x100;
*/
    _adc_g_isr = NULL;
    instance = this;

};

void ADC::_adcisr(void)
{
    instance->adcisr();
}


void ADC::adcisr(void)
{
    uint16_t data;
    
    // must read data before checking overflow bit
    data = STM_ADC->DR; // to be sure we are valid

    if (STM_ADC->SR & ADC_SR_OVR) {
        // conversion was clobbered by overflow, clear its flag too
        STM_ADC->SR &= ~(ADC_SR_OVR | ADC_SR_EOC);

        // overrun will abort scan sequence, next start will resume from beginning
        scan_index = 0;
        __debugbreak();

    } else if (STM_ADC->SR & ADC_SR_EOC) {
        STM_ADC->SR &= ~ADC_SR_EOC;

        if (_adc_g_isr != NULL && (interrupt_mask & (1 << scan_index)))
            _adc_g_isr(scan_index, data);
        
        if (++scan_index >= scan_count)
            scan_index = 0;
    }
}

uint8_t ADC::_pin_to_channel(PinName pin) {
    uint32_t function = pinmap_find_function(pin, PinMap_ADC);
    uint8_t chan = 0xFF;

    if (function != (uint32_t)NC)
        chan = scan_chan_lut[STM_PIN_CHANNEL(function)];

    return chan;
}

//Enable or disable an ADC pin
void ADC::setup(PinName pin, int state) {
    uint32_t function = pinmap_find_function(pin, PinMap_ADC);
    uint8_t stm_chan = 0xFF;
    uint8_t chan = 0xFF;

    // we don't support dealloc for now, exit early if all channels full or pin doesn't support adc
    if (!state || scan_count >= ADC_CHANNEL_COUNT || function == (uint32_t)NC) 
        return;
    
    stm_chan = STM_PIN_CHANNEL(function);
    chan = scan_count++;

    scan_chan_lut[stm_chan] = chan;

    // configure adc scan channel
    if (chan <= 5) {
        STM_ADC->SQR3 |= (stm_chan << chan);
    } else if (chan <= 11) {
        STM_ADC->SQR2 |= (stm_chan << (chan - 6));
    } else if (chan <= 15) {
        STM_ADC->SQR1 |= (stm_chan << (chan - 12));
    }

    // increase scan count
    STM_ADC->SQR1 = (STM_ADC->SQR1 & (~ADC_SQR1_L)) | (chan << ADC_SQR1_L_Pos);
}

// enable or disable burst mode
void ADC::burst(int state) {
    // this is the only mode we support, do nothing as we were configured in the constructor
}

// set interrupt enable/disable for pin to state
void ADC::interrupt_state(PinName pin, int state) {
    int chan = _pin_to_channel(pin);

    if (chan < ADC_CHANNEL_COUNT) {
        if (state)
            interrupt_mask |= (1 << chan);
        else
            interrupt_mask &= ~(1 << chan);

        // should we set/clear ie bits here too?
        if (interrupt_mask)
            NVIC_EnableIRQ(ADC_IRQn);
        else
            NVIC_DisableIRQ(ADC_IRQn);
    }
}

// append global interrupt handler to function isr
void ADC::append(void(*fptr)(int chan, uint32_t value)) {
    _adc_g_isr = fptr;
}


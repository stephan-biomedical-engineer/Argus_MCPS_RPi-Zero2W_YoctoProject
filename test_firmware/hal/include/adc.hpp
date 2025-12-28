#ifndef ADC_HPP
#define ADC_HPP

#include "hal_adc.hpp"

class Adc {
public:
    enum class Channel {
        AIN0,
        AIN1,
        AIN2,
        AIN3
    };

    Adc();               // construtor simples
    float read(Channel); // leitura direta em volts

private:
    HalI2C  _i2c;
    HalGpio _alert;
    HalAdc  _hal;
};

#endif

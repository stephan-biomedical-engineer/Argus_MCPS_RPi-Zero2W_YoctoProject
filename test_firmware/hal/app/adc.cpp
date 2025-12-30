#include "adc.hpp"

/* Hardware real da sua placa */
static constexpr const char* I2C_DEV = "/dev/i2c-1";
static constexpr uint8_t ADC_ADDR = ADS1115_ADDR_GND;
static constexpr int ALERT_GPIO = 17;

Adc::Adc()
    : _i2c(I2C_DEV, ADC_ADDR),
      _alert(ALERT_GPIO,
             HalGpio::Direction::Input,
             HalGpio::Edge::Falling,
             true),
      _hal(_i2c, &_alert)
{
    uint16_t cfg =
        ADS1115_PGA_4_096V |
        ADS1115_MODE_CONTINUOUS |
        ADS1115_DR_860SPS |
        ADS1115_COMP_QUE_1CONV;

    _hal.configure(cfg);
}

float Adc::read(Channel ch) {
    return _hal.read(static_cast<HalAdc::AdcChannel>(ch));
}

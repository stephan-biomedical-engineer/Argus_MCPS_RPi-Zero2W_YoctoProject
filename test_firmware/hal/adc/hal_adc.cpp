#include "hal_adc.hpp"
#include <unistd.h>
#include <iostream>

/* ================= Helpers ================= */

static inline void split_word(uint16_t w, uint8_t& msb, uint8_t& lsb) {
    msb = (w >> 8) & 0xFF;
    lsb = w & 0xFF;
}

static inline int16_t build_result(uint8_t msb, uint8_t lsb) {
    return static_cast<int16_t>((msb << 8) | lsb);
}

/* ================= Implementation ================= */

HalAdc::HalAdc(HalI2C& i2c, HalGpio* alert_gpio)
    : _i2c(i2c), _alert(alert_gpio) {}

bool HalAdc::configure(uint16_t config) {
    _config = config;

    uint8_t msb, lsb;
    split_word(config, msb, lsb);
    uint8_t buf[2] = { msb, lsb };
    if(!setup_ready_pin()){
       std::cerr << "ALERT not configured correctly" << std::endl;
    }
    return _i2c.write_bytes(ADS1115_REG_CONFIG, buf, 2);
}

bool HalAdc::start_conversion() {
    uint16_t cfg = _config | ADS1115_OS_START;

    uint8_t msb, lsb;
    split_word(cfg, msb, lsb);
    uint8_t buf[2] = { msb, lsb };

    return _i2c.write_bytes(ADS1115_REG_CONFIG, buf, 2);
}

bool HalAdc::read_raw(int16_t& value) {
    if (_config & ADS1115_MODE_SINGLE_SHOT) {
        if (!wait_conversion_ready(50))
            return false;
    }

    uint8_t buf[2];
    if (!_i2c.read_bytes(ADS1115_REG_CONVERSION, buf, 2))
        return false;

    value = build_result(buf[0], buf[1]);
    return true;
}

float HalAdc::read_voltage() {
    int16_t raw;
    if (!read_raw(raw))
        return 0.0f;

    return raw * pga_to_lsb();
}

float HalAdc::read(AdcChannel ch) {
    _config &= ~(0x7 << 12);             // clear MUX
    _config |= channel_to_mux(ch);       // set channel
    usleep(5000);
    if (!configure(_config))
        return 0.0f;

    if (!start_conversion())
        return 0.0f;

    return read_voltage();
}

bool HalAdc::wait_conversion_ready(int timeout_ms) {
    if (_alert) {
        return _alert->wait_for_edge(timeout_ms) > 0;
    }

    uint8_t buf[2];
    int elapsed = 0;

    while (elapsed < timeout_ms) {
        if (!_i2c.read_bytes(ADS1115_REG_CONFIG, buf, 2))
            return false;

        uint16_t cfg = (buf[0] << 8) | buf[1];
        if (cfg & ADS1115_OS_START)
            return true;

        usleep(1000);
        elapsed++;
    }
    return false;
}

float HalAdc::pga_to_lsb() const {
    switch (_config & (0x7 << 9)) {
        case ADS1115_PGA_6_144V: return 6.144f / 32768.0f;
        case ADS1115_PGA_4_096V: return 4.096f / 32768.0f;
        case ADS1115_PGA_2_048V: return 2.048f / 32768.0f;
        case ADS1115_PGA_1_024V: return 1.024f / 32768.0f;
        case ADS1115_PGA_0_512V: return 0.512f / 32768.0f;
        case ADS1115_PGA_0_256V: return 0.256f / 32768.0f;
        default:                return 2.048f / 32768.0f;
    }
}

uint16_t HalAdc::channel_to_mux(AdcChannel ch) const {
    switch (ch) {
        case AdcChannel::AIN0: return ADS1115_MUX_SINGLE_0;
        case AdcChannel::AIN1: return ADS1115_MUX_SINGLE_1;
        case AdcChannel::AIN2: return ADS1115_MUX_SINGLE_2;
        case AdcChannel::AIN3: return ADS1115_MUX_SINGLE_3;
        default:               return ADS1115_MUX_SINGLE_0;
    }
}

bool HalAdc::setup_ready_pin() {
    // Para o pino ALERT funcionar como READY: 
    // Lo_thresh MSB = 0, Hi_thresh MSB = 1
    uint8_t lo[2] = { 0x00, 0x00 };
    uint8_t hi[2] = { 0x80, 0x00 }; 
    
    if (!_i2c.write_bytes(ADS1115_REG_LO_THRESH, lo, 2)) return false;
    return _i2c.write_bytes(ADS1115_REG_HI_THRESH, hi, 2);
}

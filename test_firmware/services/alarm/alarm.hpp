#ifndef ALARM_HPP
#define ALARM_HPP

#include "hal_gpio.hpp"
#include <cstdint>

class BuzzerAlarm{
    public:
        BuzzerAlarm(unsigned int buzzer_pin);
        void set_alarm(bool enable);
    private:
        HalGpio _buzzer;
};

class LedAlarm{
    public:
        LedAlarm(unsigned int led_pin);
        void blink_short(uint32_t times);
        void blink_long(uint32_t times);
        void blink_fast(uint32_t times, uint32_t frequency_hz);
        void continuous_on(void);

    private:
        HalGpio _led;
        void blink(uint32_t on_time_ms, uint32_t off_time_ms, uint32_t duration_ms);

};

class RedLedAlarm{
    public:
        RedLedAlarm(unsigned int led_pin);
        void red_led_init(void);

    private:
        HalGpio _red_led;
};

class GreenLedAlarm{
    public:
        GreenLedAlarm(unsigned int led_pin);
        void green_led_init(void);
        
    private:
        HalGpio _green_led;
};

#endif
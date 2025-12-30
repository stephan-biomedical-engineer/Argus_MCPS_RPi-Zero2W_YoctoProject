#ifndef IR_LED_PWM_HPP
#define IR_LED_PWM_HPP

#include <cstdint>

void ir_led_pwm_init(uint32_t frequency_hz);
void ir_led_pwm_deinit(void);

#endif
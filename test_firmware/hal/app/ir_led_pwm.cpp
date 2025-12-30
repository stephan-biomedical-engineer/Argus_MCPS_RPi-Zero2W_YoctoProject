#include <cstdint>
#include "ir_led_pwm.hpp"
#include "hal_pwm.hpp"

static constexpr int IR_PWM_CHIP    = 0;
static constexpr int IR_PWM_CHANNEL = 0;

static HalPWM ir_led_pwm(IR_PWM_CHIP, IR_PWM_CHANNEL);

void ir_led_pwm_init(uint32_t frequency_hz)
{
    ir_led_pwm.set_frequency(frequency_hz);
    ir_led_pwm.set_duty_cycle(50);
}

void ir_led_pwm_deinit(void)
{
    ir_led_pwm.set_duty_cycle(0);
}

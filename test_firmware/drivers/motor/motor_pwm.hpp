#ifndef MOTOR_PWM_HPP
#define MOTOR_PWM_HPP

#include "hal_pwm.hpp"
#include "hal_gpio.hpp"

class HalStepper {
public:
    HalStepper(int step_pwm_chip, int step_pwm_channel, int dir_pin, int ena_pin);

    void move_steps(int steps, int speed_hz, bool clockwise);
    void start_continuous(int speed_hz, bool clockwise);
    void stop();

private:
    void set_enabled(bool enable);

    HalPWM _step_pwm;
    HalGpio _dir;
    HalGpio _ena;
};

#endif

#include "motor_pwm.hpp"
#include <thread>
#include <chrono>

HalStepper::HalStepper(int step_pwm_chip,
                       int step_pwm_channel,
                       int dir_pin,
                       int ena_pin)
                        : _step_pwm(step_pwm_chip, step_pwm_channel),
                        _dir(dir_pin, HalGpio::Direction::Output),
                        _ena(ena_pin, HalGpio::Direction::Output)
{
    set_enabled(true);
}

void HalStepper::set_enabled(bool enable) {
    _ena.set(enable); 
}

void HalStepper::start_continuous(int speed_hz, bool clockwise) {
    _dir.set(clockwise);
    _step_pwm.set_frequency(speed_hz);
    _step_pwm.set_duty_cycle(50);
}

void HalStepper::stop() {
    _step_pwm.apply_state(1000000, 0, false);
}

void HalStepper::move_steps(int steps, int speed_hz, bool clockwise) {
    start_continuous(speed_hz, clockwise);

    // Cálculo do tempo de espera: (1 / frequência) * número de passos
    // Ex: 200 passos a 10Hz = 20 segundos
    double duration_sec = static_cast<double>(steps) / speed_hz;
    
    std::this_thread::sleep_for(std::chrono::duration<double>(duration_sec));

    stop();
}
#ifndef ENCODER_POSITION_HPP
#define ENCODER_POSITION_HPP

#include "hal_gpio.hpp"
#include <atomic>
#include <thread>

class EncoderPosition {
public:
    EncoderPosition(
        unsigned int clk_pin,
        unsigned int dt_pin,
        unsigned int sw_pin
    );

    ~EncoderPosition();

    void start();
    void stop();

    int get_position() const;
    void reset_position();

    bool is_button_pressed();

private:
    void monitor_loop();

    HalGpio _clk;
    HalGpio _dt;
    HalGpio _sw;

    std::atomic<int> _counter{0};
    std::atomic<bool> _running{false};
    std::thread _monitor_thread;
};

#endif

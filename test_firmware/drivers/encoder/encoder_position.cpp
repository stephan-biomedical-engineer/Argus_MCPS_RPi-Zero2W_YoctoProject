#include "encoder_position.hpp"

EncoderPosition::EncoderPosition(unsigned int clk_pin, unsigned int dt_pin, unsigned int sw_pin)
    : _clk(clk_pin, HalGpio::Direction::Input, HalGpio::Edge::Rising),
      _dt(dt_pin, HalGpio::Direction::Input),
      _sw(sw_pin, HalGpio::Direction::Input, HalGpio::Edge::None, true)
{}

EncoderPosition::~EncoderPosition() {
    stop();
}

void EncoderPosition::start() {
    if (!_running) {
        _running = true;
        _monitor_thread = std::thread(&EncoderPosition::monitor_loop, this);
    }
}

void EncoderPosition::stop() {
    _running = false;
    if (_monitor_thread.joinable())
        _monitor_thread.join();
}

void EncoderPosition::monitor_loop() {
    while (_running) {
        auto edge = _clk.wait_for_edge(100000000); // 100ms

        if (edge == HalGpio::Edge::None)
            continue;

        if (_dt.get() == false)
            ++_counter;
        else
            --_counter;
    }
}

int EncoderPosition::get_position() const {
    return _counter.load();
}

void EncoderPosition::reset_position() {
    _counter.store(0);
}

bool EncoderPosition::is_button_pressed() {
    return _sw.get(); // active_low já tratado
}


#include "hal_gpio.hpp"
#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string>

HalGpio::HalGpio(unsigned int pin, Direction dir, Edge edge, bool active_low, const char* chip_name) : _pin(pin)
{
    _chip = gpiod_chip_open(chip_name);
    
    if(!_chip){
        std::cerr << "HAL GPIO: falha ao abrir gpiochip\n";
        return;
    };

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, dir == Direction::Input ? GPIOD_LINE_DIRECTION_INPUT : GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_active_low(settings, active_low);
    if (edge != Edge::None) {
        gpiod_line_settings_set_edge_detection(settings, 
            edge == Edge::Rising ? GPIOD_LINE_EDGE_RISING :
            edge == Edge::Falling ? GPIOD_LINE_EDGE_FALLING :
            GPIOD_LINE_EDGE_BOTH);
        _buffer = gpiod_edge_event_buffer_new(1);
    }

    gpiod_line_config_add_line_settings(line_cfg, &_pin, 1, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "hal-gpio");
    _req = gpiod_chip_request_lines(_chip, req_cfg, line_cfg);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);
};

void HalGpio::set(bool high){
    if (!_req)
        return;
    gpiod_line_request_set_value(_req, _pin, high ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
}

bool HalGpio::get() const {
    if (!_req)
        return false;
    int val = gpiod_line_request_get_value(_req, _pin);
    return val == GPIOD_LINE_VALUE_ACTIVE;
}

HalGpio::Edge HalGpio::wait_for_edge(int timeout_ns)
{
    if (!_req || !_buffer)
        return Edge::None;

    int ret = gpiod_line_request_wait_edge_events(_req, timeout_ns);
    if (ret <= 0)
        return Edge::None;

    ret = gpiod_line_request_read_edge_events(_req, _buffer, 1);
    if (ret <= 0)
        return Edge::None;

    const gpiod_edge_event* event = gpiod_edge_event_buffer_get_event(_buffer, 0);
    auto* ev = const_cast<struct gpiod_edge_event*>(event);

    switch (gpiod_edge_event_get_event_type(ev)) {
        case GPIOD_EDGE_EVENT_RISING_EDGE:
            return Edge::Rising;
        case GPIOD_EDGE_EVENT_FALLING_EDGE:
            return Edge::Falling;
        default:
            return Edge::None;
    }
}



HalGpio::~HalGpio() {
    if (_req) {
        gpiod_line_request_release(_req);
    }
    if (_chip) {
        gpiod_chip_close(_chip);
    }
    if (_buffer) {
        gpiod_edge_event_buffer_free(_buffer);
    }
}

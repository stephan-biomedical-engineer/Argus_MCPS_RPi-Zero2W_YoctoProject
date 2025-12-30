#ifndef BUBBLE_DETECTOR_HPP
#define BUBBLE_DETECTOR_HPP

#include <cstdint>
#include <atomic>
#include <thread>
#include <functional>
#include "adc.hpp"

constexpr size_t SAMPLING_PACKAGE_SIZE 4;

class OpticalFrontEnd
{
    public:
        OpticalFrontEnd(Adc &adc, Adc::Channel channel);
        ~OpticalFrondEnd(void);
        uint16_t sample(void);
        void sample_block(uint16_t *buffer, size_t size);

    private:
        Adc &_adc;
        Adc::Channel _channel;
};

using BubbleCallback = std::function<void(void)>;

class BubbleDetector
{
    public:
        BubbleDetector(OpticalFrontEnd &frontend);
        void start();
        void stop();
        void set_threshold(uint16_t limit);
        void register_callback(BubbleCallback cbk);

    private:
        void run();
        uint32_t compute_baseline(const uint32_t *buffer, size_t size);

        OpticalFrontEnd &_frontend;

        std::atomic<bool> _running{false};
        std::thread _thread;
        uint16_t _threshold_delta;
        BubbleCallback _callback{nullptr};
};

#endif

#ifndef HAL_SPI_HPP
#define HAL_SPI_HPP

#include <cstdint>
#include <cstddef>

class HalSpi {
public:
    HalSpi(const char* device_path, uint32_t speed_hz);
    ~HalSpi();
    bool transfer(const uint8_t* tx_buf, uint8_t* rx_buf, size_t len);

private:
    int _fd{-1};
    uint32_t _speed;
};
#endif
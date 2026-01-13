#include "hal_spi.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstring>
#include <iostream>

HalSpi::HalSpi(const char* device_path, uint32_t speed_hz) : _speed(speed_hz) {
    _fd = open(device_path, O_RDWR);
    // Configuração básica (Mode 0, 8 bits) omitida para brevidade, mas deve estar aqui igual ao anterior
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    ioctl(_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(_fd, SPI_IOC_WR_MAX_SPEED_HZ, &_speed);
}

HalSpi::~HalSpi() { if (_fd >= 0) close(_fd); }

bool HalSpi::transfer(const uint8_t* tx_buf, uint8_t* rx_buf, size_t len) {
    if (_fd < 0) return false;
    struct spi_ioc_transfer tr;
    std::memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx_buf;
    tr.rx_buf = (unsigned long)rx_buf;
    tr.len = (uint32_t)len;
    tr.speed_hz = _speed;
    tr.bits_per_word = 8;

    return ioctl(_fd, SPI_IOC_MESSAGE(1), &tr) >= 1;
}
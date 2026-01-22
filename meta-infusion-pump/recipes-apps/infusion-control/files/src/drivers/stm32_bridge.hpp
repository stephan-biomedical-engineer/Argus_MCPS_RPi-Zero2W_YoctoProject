#ifndef STM32_BRIDGE_HPP
#define STM32_BRIDGE_HPP

#include "hal_spi.hpp"
#include "hal_gpio.hpp"
#include <cstdint>
#include <vector>

extern "C"
{
#include "cmd.h"
}

class Stm32Bridge
{
public:
    // Recebe referências para as HALs já instanciadas
    Stm32Bridge(HalSpi& spi, HalGpio& ready_pin);

    // Método principal síncrono
    bool send_command(cmd_ids_t req_id, cmd_cmds_t* req_data, cmd_cmds_t* res_data);

    void suspend_hardware()
    {
        _ready_pin.release();
        _spi.close_device();
    }

    void resume_hardware()
    {
        _spi.open_device();
        _ready_pin.acquire();
    }

private:
    HalSpi& _spi;
    HalGpio& _ready_pin;

    // Buffers internos
    uint8_t _tx_buf[300];
    uint8_t _rx_buf[300];

    // O método que replica o spi_transaction do loopback
    bool _safe_transfer(size_t len);
};

#endif

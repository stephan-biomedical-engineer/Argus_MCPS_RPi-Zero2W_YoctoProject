#include "stm32_bridge.hpp"
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

extern "C"
{
#include "cmd.h"
#include "utl_io.h"
}

Stm32Bridge::Stm32Bridge(HalSpi& spi, HalGpio& ready_pin) : _spi(spi), _ready_pin(ready_pin) {}

// Transação Segura: Espera Hardware -> Delay -> Transfere
bool Stm32Bridge::_safe_transfer(size_t len)
{
    int retries = 500; // Timeout de segurança (~5s)

    // 1. Bloqueia aqui até o STM32 dizer que está PRONTO
    // Se o STM32 estiver processando o comando, o pino fica LOW e nós ficamos parados aqui.
    // Isso elimina a necessidade do "while(tentativas--)" lá fora.
    while(!_ready_pin.get())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if(--retries <= 0)
        {
            std::cerr << "[BRIDGE] Timeout Hardware: STM32 nao levantou Ready Pin" << std::endl;
            return false;
        }
    }

    // 2. Delay de Estabilização DMA (Crítico)
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    // 3. Transferência SPI
    return _spi.transfer(_tx_buf, _rx_buf, len);
}

bool Stm32Bridge::send_command(cmd_ids_t req_id, cmd_cmds_t* req_data, cmd_cmds_t* res_data)
{
    size_t size = 0;
    uint8_t master = ADDR_MASTER;
    uint8_t slave = ADDR_SLAVE;

    std::memset(_tx_buf, 0, sizeof(_tx_buf));
    std::memset(_rx_buf, 0, sizeof(_rx_buf));

    // 1. Encode do Comando
    if(!cmd_encode(_tx_buf, &size, &master, &slave, &req_id, req_data))
    {
        std::cerr << "[BRIDGE] Erro de Encode" << std::endl;
        return false;
    }

    size_t xfer_len = (size < 64) ? 64 : size;

    // 2. Envia o Comando
    // O _safe_transfer garante que só enviamos se o STM32 estiver ouvindo
    if(!_safe_transfer(xfer_len))
    {
        return false;
    }

    // 3. Lê a Resposta (Imediatamente)
    // Pequeno delay para o STM32 trocar o contexto de RX para TX e calcular a resposta
    // O _safe_transfer abaixo vai BLOQUEAR no GPIO até a resposta estar pronta.

    // Prepara Dummys
    std::memset(_tx_buf, 0, 64);

    // Assim que o _safe_transfer retornar, significa que o GPIO subiu e lemos os dados válidos.
    if(!_safe_transfer(64))
    {
        return false;
    }

    // 4. Decode
    uint8_t rx_id = _rx_buf[2];

    // Validação básica se não veio lixo (0x00 ou 0xFF)
    if(rx_id == 0x00 || rx_id == 0xFF)
    {
        // Se confiamos no pino Ready, isso aqui seria um erro grave de firmware do lado de lá
        // std::cerr << "[BRIDGE] Erro: STM32 indicou pronto mas enviou lixo." << std::endl;
        return false;
    }

    uint8_t src, dst;
    cmd_ids_t res_id_decoded;
    uint16_t payload_len = (_rx_buf[4] << 8) | _rx_buf[3];
    size_t total_len = 5 + payload_len + 2;

    if(cmd_decode(_rx_buf, total_len, &src, &dst, &res_id_decoded, res_data))
    {
        return true;
    }

    std::cerr << "[BRIDGE] Erro de Checksum na resposta" << std::endl;
    return false;
}

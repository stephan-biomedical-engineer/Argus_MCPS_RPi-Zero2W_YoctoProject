#include "stm32_bridge.hpp"
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio> // Para printf

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
    // Limpa buffers
    std::memset(_tx_buf, 0, sizeof(_tx_buf));
    std::memset(_rx_buf, 0, sizeof(_rx_buf));

    size_t encoded_size = 0;
    uint8_t master = ADDR_MASTER;
    uint8_t slave = ADDR_SLAVE;

    // 1. Encode do Comando
    // IMPORTANTE: O cmd_encode (versão nova) já insere o SOF (AA 55) automaticamente.
    if(!cmd_encode(_tx_buf, &encoded_size, &master, &slave, &req_id, req_data))
    {
        std::cerr << "[BRIDGE] Erro de Encode" << std::endl;
        return false;
    }

    // Garante tamanho mínimo de transferência (64 bytes para manter o clock)
    size_t xfer_len = (encoded_size < 64) ? 64 : encoded_size;

    // 2. Envia o Comando
    if(!_safe_transfer(xfer_len))
    {
        return false;
    }

    // 3. Lê a Resposta (Imediatamente)
    // Prepara Dummys
    std::memset(_tx_buf, 0, 64);

    // Lê 64 bytes de resposta (pode conter lixo + resposta)
    if(!_safe_transfer(64))
    {
        return false;
    }

    // printf("[SPI RAW RX]: ");
    // for(int rx_byte = 0; rx_byte < 16; rx_byte++) 
    // { 
    //     printf("%02X ", _rx_buf[rx_byte]); 
    // }
    // printf("\n");

    // 4. SCANNER DE SOF (A Mágica da Sincronia) 
    // Em vez de assumir que a resposta está no byte 0 ou 2, procuramos a assinatura.
    
    int sof_index = -1;
    // Varre o buffer procurando AA 55
    for(int scan_sof_idx = 0; scan_sof_idx < (64 - CMD_HDR_SIZE); scan_sof_idx++) 
    {
        if(_rx_buf[scan_sof_idx] == CMD_SOF_1_BYTE && _rx_buf[scan_sof_idx+1] == CMD_SOF_2_BYTE) 
        {
            sof_index = scan_sof_idx;
            break;
        }
    }

    if (sof_index < 0) 
    {
        // Se não achou SOF, é erro de comunicação ou o STM32 não respondeu.
        std::cerr << "[BRIDGE] Erro: SOF nao encontrado. Dump RX (16 bytes): ";
        for(int rx_byte=0; rx_byte<16; rx_byte++) 
        {
            printf("%02X ", _rx_buf[rx_byte]);
        }
        printf("\n");
        return false;
    }

    // Aponta para o início real do pacote encontrado
    uint8_t* p_packet = &_rx_buf[sof_index];
    
    // Calcula tamanho total esperado
    // Header V2 tem 7 bytes. O Payload Size está nos bytes 5 e 6 (indices relativos ao SOF).
    // p_packet[0]=AA, [1]=55, [2]=DST, [3]=SRC, [4]=ID, [5]=SizeL, [6]=SizeH
    
    uint16_t payload_len = utl_io_get16_fl(&p_packet[5]); 
    size_t total_valid_len = CMD_HDR_SIZE + payload_len + CMD_TRAILER_SIZE;

    // Decodifica a partir do SOF encontrado
    // O cmd_decode novo já sabe pular o SOF interno.
    uint8_t src, dst;
    cmd_ids_t res_id_decoded;

    if(cmd_decode(p_packet, total_valid_len, &src, &dst, &res_id_decoded, res_data))
    {
        return true;
    }

    std::cerr << "[BRIDGE] Erro de Checksum na resposta (SOF achado em " << sof_index << ")" << std::endl;
    return false;
}
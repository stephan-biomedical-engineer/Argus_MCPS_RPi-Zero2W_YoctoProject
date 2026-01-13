#include "infusion_manager.hpp"
#include <iostream>
#include <chrono>
#include <cstring>

InfusionManager::InfusionManager(Stm32Bridge& bridge) 
    : _bridge(bridge), _running(false) {}

InfusionManager::~InfusionManager() {
    stop();
}

void InfusionManager::start() {
    if (!_running) {
        _running = true;
        _monitor_thread = std::thread(&InfusionManager::monitor_loop, this);
        std::cout << "[Service] Monitoramento iniciado." << std::endl;
    }
}

void InfusionManager::stop() {
    _running = false;
    if (_monitor_thread.joinable()) {
        _monitor_thread.join();
    }
}

void InfusionManager::set_status_callback(StatusCallback cb) {
    std::lock_guard<std::mutex> lock(_spi_mutex);
    _status_cb = cb;
}

// --- Loop de Monitoramento (Roda em Thread Separada) ---
void InfusionManager::monitor_loop() {
    while (_running) {
        cmd_cmds_t req;
        cmd_cmds_t res;
        std::memset(&req, 0, sizeof(req));
        std::memset(&res, 0, sizeof(res));

        bool success = false;

        {
            // Bloqueia o Mutex: Ninguém mais usa o SPI agora
            std::lock_guard<std::mutex> lock(_spi_mutex);
            success = _bridge.send_command(CMD_GET_STATUS_REQ_ID, &req, &res);
        } // Mutex destravado aqui

        if (success) {
            // Se temos callback registrado, enviamos os dados pra cima
            if (_status_cb) {
                _status_cb(res.status_res.status_data);
            }
        } else {
            // Opcional: Logar erro de comunicação ou notificar desconexão
            // std::cerr << "[Service] Falha ao ler status do STM32" << std::endl;
        }

        // Aguarda 1 segundo antes da próxima leitura
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// --- Comandos de Controle ---

bool InfusionManager::start_infusion() {
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req)); // Payload vazio para RUN
    
    std::lock_guard<std::mutex> lock(_spi_mutex);
    return _bridge.send_command(CMD_ACTION_RUN_REQ_ID, &req, &res);
}

bool InfusionManager::pause_infusion() {
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));
    
    std::lock_guard<std::mutex> lock(_spi_mutex);
    return _bridge.send_command(CMD_ACTION_PAUSE_REQ_ID, &req, &res);
}

bool InfusionManager::stop_infusion() {
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));
    
    std::lock_guard<std::mutex> lock(_spi_mutex);
    return _bridge.send_command(CMD_ACTION_ABORT_REQ_ID, &req, &res);
}

bool InfusionManager::set_config(uint32_t volume_ml, uint32_t rate_ml_h, uint8_t mode) {
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));

    req.config_req.config.volume = volume_ml;
    req.config_req.config.flow_rate = rate_ml_h;
    req.config_req.config.mode = mode;
    req.config_req.config.diameter = 1; // Default ou parametrizar se quiser

    std::lock_guard<std::mutex> lock(_spi_mutex);
    bool ok = _bridge.send_command(CMD_SET_CONFIG_REQ_ID, &req, &res);
    
    if (ok) {
        // Verifica se o STM32 retornou CMD_OK logicamente
        return (res.config_res.status == CMD_OK);
    }
    return false;
}

// --- NOVOS COMANDOS CORRIGIDOS (SEM CHAMAR SET_CONFIG) ---

bool InfusionManager::start_bolus(uint32_t volume_ml, uint32_t rate_ml_h) {
    /* NOTA IMPORTANTE:
       Nós NÃO chamamos set_config() aqui. 
       Se chamássemos, sobrescreveríamos a 'flow_rate' padrão (ex: 100ml/h) com a taxa de bolus (600ml/h).
       Apenas enviamos o comando de AÇÃO. O Firmware usará a taxa padrão de Bolus.
    */
    
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));
    
    // (Opcional) Se o protocolo SPI Action suportasse payload, passaríamos volume/rate aqui.
    // Como o firmware logic_engine.c usa RATE_BOLUS_DEFAULT, apenas o ID da ação basta.

    std::lock_guard<std::mutex> lock(_spi_mutex);
    return _bridge.send_command(CMD_ACTION_BOLUS_REQ_ID, &req, &res);
}

bool InfusionManager::start_purge() {
    /* Mesma lógica do Bolus: Apenas envia a Ação */
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));
    
    std::lock_guard<std::mutex> lock(_spi_mutex);
    return _bridge.send_command(CMD_ACTION_PURGE_REQ_ID, &req, &res);
}
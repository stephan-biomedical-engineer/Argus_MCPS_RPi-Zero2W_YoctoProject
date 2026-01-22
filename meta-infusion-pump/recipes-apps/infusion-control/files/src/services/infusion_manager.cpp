#include "infusion_manager.hpp"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdlib>        // Para std::system
#include <sys/wait.h>     // Para WEXITSTATUS
#include <boost/json.hpp> // <--- JSON SERIALIZER

static std::string state_to_string(uint8_t state)
{
    switch(state)
    {
    case 0:
        return "POWER_ON"; // Início
    case 1:
        return "IDLE"; // Parado pronto
    case 2:
        return "RUNNING"; // Infundindo
    case 3:
        return "BOLUS"; // Dose rápida
    case 4:
        return "PURGE"; // Cebar
    case 5:
        return "PAUSED"; // Pausado
    case 6:
        return "KVO"; // Keep Vein Open (vazão mínima)
    case 7:
        return "END"; // Fim da infusão

    // Mapeamos todos os alarmes para a string "ALARM"
    // para o Python reconhecer e pintar de vermelho.
    case 8:
        return "ALARM"; // (Bolha)
    case 9:
        return "ALARM"; // (Oclusão)
    case 10:
        return "ALARM"; // (Porta)

    case 11:
        return "OFF";

    default:
        return "UNKNOWN (" + std::to_string(state) + ")";
    }
}

InfusionManager::InfusionManager(Stm32Bridge& bridge, HalGpio& reset_pin)
    : _bridge(bridge), _reset_pin(reset_pin), _running(false), _maintenance_mode(false)
{}

InfusionManager::~InfusionManager()
{
    stop();
}

void InfusionManager::start()
{
    if(!_running)
    {
        _running = true;
        _monitor_thread = std::thread(&InfusionManager::monitor_loop, this);
        std::cout << "[Service] Monitoramento iniciado." << std::endl;
    }
}

void InfusionManager::stop()
{
    _running = false;
    if(_monitor_thread.joinable())
    {
        _monitor_thread.join();
    }
}

void InfusionManager::set_status_callback(StatusCallback cb)
{
    std::lock_guard<std::mutex> lock(_spi_mutex);
    _status_cb = cb;
}

// --- Loop de Monitoramento ---
void InfusionManager::monitor_loop()
{
    while(_running)
    {

        // 1. Trava de Manutenção (OTA)
        if(_maintenance_mode)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        cmd_cmds_t req, res;
        std::memset(&req, 0, sizeof(req));
        std::memset(&res, 0, sizeof(res));

        bool success = false;
        {
            std::lock_guard<std::mutex> lock(_spi_mutex);
            success = _bridge.send_command(CMD_GET_STATUS_REQ_ID, &req, &res);
        }

        if(success)
        {
            if(_status_cb)
            {
                // --- SERIALIZAÇÃO JSON COM NOMES CORRETOS ---
                auto& data = res.status_res.status_data;

                boost::json::object json;
                // Nomes vindos do seu cmd.c:
                json["state"] = state_to_string(data.current_state);
                json["infused_volume_ml"] = data.volume;
                json["real_rate_ml_h"] = data.flow_rate_set;
                // json["pressure"] = data.pressure; // Opcional
                // json["alarm"] = (bool)data.alarm_active; // Opcional

                std::string payload = boost::json::serialize(json);
                _status_cb(payload);
                // --------------------------------------------
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// --- Comandos de Controle ---
bool InfusionManager::start_infusion()
{
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));
    std::lock_guard<std::mutex> lock(_spi_mutex);
    return _bridge.send_command(CMD_ACTION_RUN_REQ_ID, &req, &res);
}

bool InfusionManager::pause_infusion()
{
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));
    std::lock_guard<std::mutex> lock(_spi_mutex);
    return _bridge.send_command(CMD_ACTION_PAUSE_REQ_ID, &req, &res);
}

bool InfusionManager::stop_infusion()
{
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));
    std::lock_guard<std::mutex> lock(_spi_mutex);
    return _bridge.send_command(CMD_ACTION_ABORT_REQ_ID, &req, &res);
}

bool InfusionManager::set_config(uint32_t volume_ml, uint32_t rate_ml_h, uint8_t mode)
{
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));
    req.config_req.config.volume = volume_ml;
    req.config_req.config.flow_rate = rate_ml_h;
    req.config_req.config.mode = mode;
    req.config_req.config.diameter = 1;

    std::lock_guard<std::mutex> lock(_spi_mutex);
    bool ok = _bridge.send_command(CMD_SET_CONFIG_REQ_ID, &req, &res);
    return (ok && res.config_res.status == CMD_OK);
}

bool InfusionManager::start_bolus(uint32_t volume_ml, uint32_t rate_ml_h)
{
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));
    std::lock_guard<std::mutex> lock(_spi_mutex);
    return _bridge.send_command(CMD_ACTION_BOLUS_REQ_ID, &req, &res);
}

bool InfusionManager::start_purge()
{
    cmd_cmds_t req, res;
    std::memset(&req, 0, sizeof(req));
    std::lock_guard<std::mutex> lock(_spi_mutex);
    return _bridge.send_command(CMD_ACTION_PURGE_REQ_ID, &req, &res);
}

void InfusionManager::start_ota_process(const std::string& filepath)
{
    _maintenance_mode = true;

    // Roda em thread detachada
    std::thread([this, filepath]() {
        std::cout << "[MANAGER] Pausando Hardware para OTA Externo..." << std::endl;

        // 1. O PAI SOLTA O OSSO
        {
            std::lock_guard<std::mutex> lock(_spi_mutex);
            _bridge.suspend_hardware();
        }

        std::cout << "[MANAGER] Iniciando stm32-updater..." << std::endl;
        std::string cmd = "/usr/bin/stm32-updater " + filepath;

        // 2. O FILHO RODA
        int ret_code = std::system(cmd.c_str());

        // 3. VALIDAÇÃO E ESPERA (AQUI ESTÁ A MUDANÇA)
        if(WEXITSTATUS(ret_code) == 0)
        {
            std::cout << "[MANAGER] OTA SUCESSO." << std::endl;

            // --- O TRECHO NOVO ENTRA AQUI ---
            std::cout << "[MANAGER] Aguardando reboot do STM32..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
            // --------------------------------
        }
        else
        {
            std::cerr << "[MANAGER] OTA FALHOU. Code: " << ret_code << std::endl;
            // Se falhou, não precisamos esperar 5s, pois o STM32 provavelmente nem reiniciou
        }

        // 4. O PAI PEGA O OSSO DE VOLTA
        std::cout << "[MANAGER] OTA Finalizado. Retomando Hardware..." << std::endl;
        {
            std::lock_guard<std::mutex> lock(_spi_mutex);
            _bridge.resume_hardware();
        }

        _maintenance_mode = false;
    }).detach();
}

void InfusionManager::hard_reset_stm32()
{
    std::cout << "[MANAGER] EXECUTANDO HARD RESET NO STM32..." << std::endl;

    // 1. Pausa o Hardware (Solta SPI/GPIOs para evitar glitchs)
    {
        std::lock_guard<std::mutex> lock(_spi_mutex);
        _bridge.suspend_hardware();
    }

    // 2. O Pulso da Morte (GND por 100ms)
    _reset_pin.set(false); // LOW -> Reset
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    _reset_pin.set(true); // HIGH -> Solta o Reset

    // 3. Aguarda o Boot do STM32 (importante!)
    std::cout << "[MANAGER] Aguardando STM32 acordar..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 4. Retoma o Hardware
    {
        std::lock_guard<std::mutex> lock(_spi_mutex);
        _bridge.resume_hardware();
    }

    std::cout << "[MANAGER] STM32 Reiniciado." << std::endl;
}

#ifndef INFUSION_MANAGER_HPP
#define INFUSION_MANAGER_HPP

#include "stm32_bridge.hpp"
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <string> // <--- 1. Necessário para std::string

// --- 2. MUDANÇA CRÍTICA: O Callback agora transporta JSON (String), não struct bruta ---
using StatusCallback = std::function<void(std::string)>;

class InfusionManager
{
public:
    InfusionManager(Stm32Bridge& bridge, HalGpio& reset_pin);
    ~InfusionManager();

    // Inicia a thread de monitoramento
    void start();

    // Para a thread
    void stop();

    // Registra quem quer ouvir as atualizações de status
    void set_status_callback(StatusCallback cb);

    // --- Comandos de Controle (Chamados pelo MQTT) ---
    // Retornam true se o STM32 confirmou (ACK)

    bool start_infusion();
    bool pause_infusion();
    bool stop_infusion(); // Abort

    // Configuração completa
    bool set_config(uint32_t volume_ml, uint32_t rate_ml_h, uint8_t mode);

    // Métodos de Ação Direta
    bool start_bolus(uint32_t volume_ml, uint32_t rate_ml_h);
    bool start_purge();

    // --- 3. NOVO: Método para iniciar atualização de Firmware ---
    void start_ota_process(const std::string& filepath);

    void hard_reset_stm32();

private:
    Stm32Bridge& _bridge;

    HalGpio& _reset_pin;

    // Mutex para proteger o acesso concorrente ao SPI
    std::mutex _spi_mutex;

    // Controle da Thread de Monitoramento
    std::atomic<bool> _running;

    std::atomic<bool> _maintenance_mode{false};

    std::thread _monitor_thread;

    // Callback para notificar o andar de cima
    StatusCallback _status_cb;

    // Loop principal da thread
    void monitor_loop();
};

#endif

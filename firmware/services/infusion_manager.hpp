#ifndef INFUSION_MANAGER_HPP
#define INFUSION_MANAGER_HPP

#include "stm32_bridge.hpp"
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

// Define um tipo de callback para quando chegar status novo
// O Server Layer vai usar isso para publicar no MQTT
using StatusCallback = std::function<void(const cmd_status_payload_t&)>;

class InfusionManager {
public:
    InfusionManager(Stm32Bridge& bridge);
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

    // --- NOVOS MÉTODOS (Correção Lógica Bolus/Purge) ---
    // Estes métodos enviam apenas o comando de AÇÃO, sem reconfigurar a vazão padrão
    bool start_bolus(uint32_t volume_ml, uint32_t rate_ml_h);
    bool start_purge();

private:
    Stm32Bridge& _bridge;
    
    // Mutex para proteger o acesso concorrente ao SPI
    // (Monitor Thread vs MQTT Thread)
    std::mutex _spi_mutex;

    // Controle da Thread de Monitoramento
    std::atomic<bool> _running;
    std::thread _monitor_thread;
    
    // Callback para notificar o andar de cima
    StatusCallback _status_cb;

    // Loop principal da thread
    void monitor_loop();
};

#endif
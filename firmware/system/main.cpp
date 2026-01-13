#include <iostream>
#include <csignal>
#include <boost/asio.hpp>
#include <systemd/sd-daemon.h>

#include "hal_spi.hpp"
#include "hal_gpio.hpp"
#include "stm32_bridge.hpp"
#include "infusion_manager.hpp"
#include "mqtt_client.hpp" 

// Globais para Signal Handler
boost::asio::io_context* g_io = nullptr;
InfusionManager* g_manager = nullptr;

void signal_handler(int signal) {
    std::cout << "\n[SYSTEM] Sinal " << signal << " recebido. Parando..." << std::endl;
    // Para o polling do hardware primeiro
    if (g_manager) g_manager->stop();
    // Para o loop de eventos (desconecta MQTT)
    if (g_io) g_io->stop();
}

int main() {
    // Permite logs imediatos no journalctl
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::cout << "--- BOMBA DE INFUSAO IOT (FINAL) ---" << std::endl;

    try {
        // 1. Hardware Initialization
        HalSpi spi("/dev/spidev0.0", 1000000);
        HalGpio ready_pin(25, HalGpio::Direction::Input, HalGpio::Edge::Rising, false, "/dev/gpiochip0");

        // 2. Driver Layer
        Stm32Bridge bridge(spi, ready_pin);

        // 3. Service Layer (Manager)
        InfusionManager manager(bridge);
        g_manager = &manager;

        // 4. Server Layer (MQTT)
        boost::asio::io_context io;
        g_io = &io;
        MqttClient mqtt(io, manager);

        // Sinais do SO
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // Start
        manager.start(); // Inicia thread de polling (1Hz)
        mqtt.start();    // Conecta no Broker e inicia loop de msg

        // Notifica Systemd
        sd_notify(0, "READY=1");
        std::cout << "[SYSTEM] Online. Aguardando comandos MQTT..." << std::endl;

        // Bloqueia aqui
        io.run();

    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
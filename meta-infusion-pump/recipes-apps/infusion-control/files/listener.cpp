#include <iostream>
#include <string>
#include <mqtt/async_client.h>
#include <gpiod.h>
#include <systemd/sd-daemon.h>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <fstream>

const std::string ADDRESS { "tcp://localhost:1883" };
const std::string CLIENT_ID { "infusion_pump_cpp" };
const std::string TOPIC { "bomba/infusao" };
const unsigned int MOTOR_PIN = 17; // Digital On/Off
const char* CHIP_PATH = "/dev/gpiochip0";

// Função para aplicar velocidade (0 a 100)
void set_pump_speed(int percentage) {
    long period = 1000000; // 1kHz em nanosegundos
    long duty = (period * percentage) / 100;

    std::ofstream duty_file("/sys/class/pwm/pwmchip0/pwm0/duty_cycle");
    if(duty_file.is_open()){
        duty_file << std::to_string(duty);
        duty_file.close();
    }
}

int main() {
    // --- Configuração GPIO ---
    struct gpiod_chip *chip = gpiod_chip_open(CHIP_PATH);
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = MOTOR_PIN;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "infusion-pump");
    struct gpiod_line_request *request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    // --- Configuração MQTT ---
    mqtt::async_client client(ADDRESS, CLIENT_ID);
    auto connOpts = mqtt::connect_options_builder().clean_session(true).finalize();

    client.set_message_callback([&](mqtt::const_message_ptr msg) {
        std::string payload = msg->to_string();
        std::cout << "Comando: " << payload << std::endl;

        if (payload == "LIGAR") {
            gpiod_line_request_set_value(request, MOTOR_PIN, GPIOD_LINE_VALUE_ACTIVE);
            set_pump_speed(50); // Inicia com 50% de velocidade
        } else if (payload == "DESLIGAR") {
            gpiod_line_request_set_value(request, MOTOR_PIN, GPIOD_LINE_VALUE_INACTIVE);
            set_pump_speed(0);
        }
    });

    try {
        client.connect(connOpts)->wait();
        client.subscribe(TOPIC, 1)->wait();
        
        std::cout << "Sistema Online. Watchdog ativo." << std::endl;
        sd_notify(0, "READY=1");

        // Loop de Watchdog Principal
        while (true) {
            if (client.is_connected()) {
                sd_notify(0, "WATCHDOG=1");
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    } catch (const mqtt::exception& exc) {
        std::cerr << "Erro: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
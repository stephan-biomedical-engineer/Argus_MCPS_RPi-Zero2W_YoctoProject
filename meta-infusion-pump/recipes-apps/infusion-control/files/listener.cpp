#include <boost/asio.hpp>
#include <boost/mqtt5/mqtt_client.hpp>
#include <gpiod.h>
#include <systemd/sd-daemon.h>
#include <iostream>
#include <fstream>
#include <string>

// --- Configurações ---
const std::string BROKER_ADDR = "127.0.0.1";
const uint16_t BROKER_PORT = 1883;
const std::string TOPIC_NAME = "bomba/infusao";
const unsigned int MOTOR_PIN = 17;

class InfusionPump {
public:
    InfusionPump(boost::asio::io_context& io)
        : io_(io),
          watchdog_timer_(io, boost::asio::chrono::seconds(2)),
          mqtt_client_(io) 
    {
        setup_hardware();
        setup_mqtt();
        start_watchdog();
    }

private:
    boost::asio::io_context& io_;
    boost::asio::steady_timer watchdog_timer_;
    boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> mqtt_client_;
    struct gpiod_line_request* gpio_request_;

    void setup_hardware() {
        struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
        struct gpiod_line_settings *settings = gpiod_line_settings_new();
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
        
        struct gpiod_line_config *line_cfg = gpiod_line_config_new();
        unsigned int offset = MOTOR_PIN;
        gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
        
        struct gpiod_request_config *req_cfg = gpiod_request_config_new();
        gpiod_request_config_set_consumer(req_cfg, "infusion-pump");
        
        gpio_request_ = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
        std::cout << "Hardware GPIO configurado via libgpiod." << std::endl;
    }

    void set_pump_speed(int percentage) {
        long period = 1000000; // 1kHz
        long duty = (period * percentage) / 100;
        std::ofstream duty_file("/sys/class/pwm/pwmchip0/pwm0/duty_cycle");
        if(duty_file.is_open()){
            duty_file << std::to_string(duty);
            std::cout << "PWM: Velocidade ajustada para " << percentage << "%" << std::endl;
        }
    }

    void setup_mqtt() {
        mqtt_client_.brokers(BROKER_ADDR, BROKER_PORT);

        // Iniciamos o loop de recepção ANTES do async_run
        setup_receive_loop();

        // Inicia o cliente
        mqtt_client_.async_run(boost::asio::detached);

        // Inscreve no tópico
        boost::mqtt5::subscribe_topic sub_topic{TOPIC_NAME, {boost::mqtt5::qos_e::at_least_once}};
        mqtt_client_.async_subscribe(
            sub_topic,
            boost::mqtt5::subscribe_props{},
            [](boost::mqtt5::error_code ec, std::vector<boost::mqtt5::reason_code> rc, boost::mqtt5::suback_props props) {
                if (!ec) std::cout << "Inscrito e pronto para comandos." << std::endl;
            }
        );
    }

    // Loop recursivo para continuar recebendo mensagens
    void setup_receive_loop() {
        // Usamos o operador -> para garantir que estamos acessando o cliente corretamente
        mqtt_client_.async_receive([this](
            boost::mqtt5::error_code ec, 
            std::string topic, 
            std::string payload, 
            boost::mqtt5::publish_props props
        ) {
            if (!ec) {
                process_message(payload);
                setup_receive_loop(); // Realimenta o loop imediatamente
            } else {
                // Se houver erro (ex: desconexão), tentamos religar o loop após 1 segundo
                std::cerr << "Erro no receive: " << ec.message() << ". Tentando reconectar loop..." << std::endl;
                auto timer = std::make_shared<boost::asio::steady_timer>(io_, boost::asio::chrono::seconds(1));
                timer->async_wait([this, timer](const boost::system::error_code&) {
                    setup_receive_loop();
                });
            }
        });
    }

    void process_message(const std::string& payload) {
        std::cout << "Comando Recebido: " << payload << std::endl;
        if (payload == "LIGAR") {
            gpiod_line_request_set_value(gpio_request_, MOTOR_PIN, GPIOD_LINE_VALUE_ACTIVE);
            set_pump_speed(50);
        } else if (payload == "DESLIGAR") {
            gpiod_line_request_set_value(gpio_request_, MOTOR_PIN, GPIOD_LINE_VALUE_INACTIVE);
            set_pump_speed(0);
        }
    }

    void start_watchdog() {
        watchdog_timer_.async_wait([this](const boost::system::error_code& ec) {
            if (!ec) {
                sd_notify(0, "WATCHDOG=1");
                watchdog_timer_.expires_at(watchdog_timer_.expires_at() + boost::asio::chrono::seconds(2));
                start_watchdog();
            }
        });
    }
};

int main() {
    try {
        boost::asio::io_context io;
        InfusionPump pump(io);
        sd_notify(0, "READY=1");
        std::cout << "Sistema Online (Asio + MQTT5)" << std::endl;
        io.run(); 
    } catch (const std::exception& e) {
        std::cerr << "Erro Fatal: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
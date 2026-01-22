#ifndef MQTT_CLIENT_HPP
#define MQTT_CLIENT_HPP

#include <boost/asio.hpp>
#include <boost/mqtt5/mqtt_client.hpp>
#include <boost/mqtt5/types.hpp>
#include <boost/mqtt5/reason_codes.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include "infusion_manager.hpp"

// Configurações
const std::string BROKER_ADDR = "127.0.0.1";
const uint16_t BROKER_PORT = 1883;
const std::string CLIENT_ID = "infusion_pump_rpi";
const std::string TOPIC_CMD = "bomba/comando";
const std::string TOPIC_STATUS = "bomba/status";

// Constantes para a "Tradução" de Modos
const uint8_t MODE_CONTINUOUS = 0;
const uint8_t MODE_BOLUS = 2;
const uint8_t MODE_PURGE = 3;
const uint32_t MAX_PURGE_RATE = 1200; // ml/h

class MqttClient
{
public:
    using client_type = boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket>;

    MqttClient(boost::asio::io_context& io, InfusionManager& manager)
        : _io(io), _client(io), _manager(manager), _retry_timer(io)
    {
        setup_manager_callbacks();
    }

    void start()
    {
        _client.brokers(BROKER_ADDR, BROKER_PORT).credentials(CLIENT_ID).async_run(boost::asio::detached);

        subscribe_topics();
        receive_loop();
    }

private:
    boost::asio::io_context& _io;
    client_type _client;
    InfusionManager& _manager;
    boost::asio::steady_timer _retry_timer;

    void subscribe_topics()
    {
        _client.async_subscribe(
            boost::mqtt5::subscribe_topic{TOPIC_CMD, boost::mqtt5::qos_e::at_least_once},
            boost::mqtt5::subscribe_props{},
            [this](boost::mqtt5::error_code ec, std::vector<boost::mqtt5::reason_code> rc, auto props) {
                if(!ec)
                    std::cout << "[MQTT] Inscrito em: " << TOPIC_CMD << std::endl;
                else
                {
                    std::cerr << "[MQTT] Falha inscricao: " << ec.message() << ". Retry 5s..." << std::endl;
                    schedule_subscribe_retry();
                }
            });
    }

    void schedule_subscribe_retry()
    {
        _retry_timer.expires_after(std::chrono::seconds(5));
        _retry_timer.async_wait([this](boost::system::error_code ec) {
            if(!ec)
                subscribe_topics();
        });
    }

    void receive_loop()
    {
        _client.async_receive([this](boost::mqtt5::error_code ec, std::string topic, std::string payload, auto props) {
            if(!ec)
            {
                process_command(payload);
                receive_loop();
                return;
            }
            if(ec == boost::mqtt5::client::error::session_expired)
            {
                std::cout << "[MQTT] Sessao expirada. Re-inscrevendo..." << std::endl;
                subscribe_topics();
                receive_loop();
                return;
            }
            if(ec == boost::asio::error::operation_aborted)
                return;

            receive_loop();
        });
    }

    // --- CORAÇÃO DA TRADUÇÃO JSON -> C++ ---
    void process_command(const std::string& json_str)
    {
        try
        {
            boost::system::error_code ec;
            auto json_val = boost::json::parse(json_str, ec);
            if(ec)
            {
                std::cerr << "[MQTT] JSON Malformado." << std::endl;
                return;
            }

            auto json = json_val.as_object();
            if(!json.contains("action"))
                return;

            std::string action = boost::json::value_to<std::string>(json.at("action"));
            std::cout << "[MQTT] Acao: " << action << std::endl;

            bool success = false;

            // 1. Comandos de Controle (Iguais para qualquer modo)
            if(action == "start")
                success = _manager.start_infusion();
            else if(action == "pause")
                success = _manager.pause_infusion();
            else if(action == "stop" || action == "abort")
                success = _manager.stop_infusion();

            // 2. Configuração Padrão (Contínuo)
            else if(action == "config")
            {
                if(json.contains("volume") && json.contains("rate") && json.contains("mode"))
                {
                    uint32_t vol = (uint32_t) json.at("volume").as_int64();
                    uint32_t rate = (uint32_t) json.at("rate").as_int64();
                    uint8_t mode = (uint8_t) json.at("mode").as_int64();
                    success = _manager.set_config(vol, rate, mode);
                }
                else
                {
                    std::cerr << "[MQTT] Config faltando params." << std::endl;
                }
            }

            // 3. Atalho: PURGE (Traduz para Config Mode 3)
            else if(action == "purge")
            {
                std::cout << "   -> Traduzindo Purge para Config Mode 3" << std::endl;
                success = _manager.set_config(0, MAX_PURGE_RATE, MODE_PURGE);
            }

            // 4. Atalho: BOLUS (Traduz para Config Mode 2)
            else if(action == "bolus")
            {
                uint32_t vol = json.contains("volume") ? (uint32_t) json.at("volume").as_int64() : 5;
                uint32_t rate = json.contains("rate") ? (uint32_t) json.at("rate").as_int64() : 600;

                std::cout << "   -> Traduzindo Bolus para Config Mode 2" << std::endl;
                success = _manager.set_config(vol, rate, MODE_BOLUS);
            }

            // 5. UPDATE FIRMWARE (A PEÇA QUE FALTAVA)
            else if(action == "update_firmware")
            {
                if(json.contains("file_path"))
                {
                    std::string path = boost::json::value_to<std::string>(json.at("file_path"));
                    std::cout << "   -> Iniciando Processo de Update: " << path << std::endl;

                    // Chama o manager. Como roda em thread separada, consideramos comando aceito imediatamente.
                    _manager.start_ota_process(path);
                    success = true;
                }
                else
                {
                    std::cerr << "[MQTT] Erro: update_firmware sem 'file_path'" << std::endl;
                }
            }

            else if(action == "reset_mcu")
            {
                std::cout << "[MQTT] Solicitacao de Reset Fisico..." << std::endl;
                _manager.hard_reset_stm32();
                success = true;
            }

            if(success)
                std::cout << "   -> Comando Aceito (ACK)" << std::endl;
            else
                std::cerr << "   -> Comando Rejeitado (NACK)" << std::endl;
        }
        catch(const std::exception& e)
        {
            std::cerr << "[MQTT] Erro logico: " << e.what() << std::endl;
        }
    }

    void setup_manager_callbacks()
    {
        _manager.set_status_callback([this](std::string json_payload) {
            boost::asio::post(_io, [this, json_payload]() {
                _client.async_publish<boost::mqtt5::qos_e::at_most_once>(
                    TOPIC_STATUS, json_payload, boost::mqtt5::retain_e::no, boost::mqtt5::publish_props{},
                    [](boost::system::error_code ec) {
                        if(ec)
                            std::cerr << "Erro Pub: " << ec.message() << std::endl;
                    });
            });
        });
    }
};

#endif

#include "sim7000_mqtt_service.hpp"
#include "sim7000_helpers.hpp"
#include <esp_log.h>
#include <cstring>

namespace axomotor::lte_modem
{
    using namespace axomotor::lte_modem::internal;
    
    static const char *TAG = "sim7000:mqtt";

    SIM7000_MQTT::SIM7000_MQTT(
        std::weak_ptr<SIM7000_Modem> modem,
        int message_buffer_size,
        int response_buffer_size) : SIM7000_Service(modem)
    { 
        m_result_info = std::make_shared<internal::sim7000_cmd_result_info_t>();
        m_result_info->response.reserve(response_buffer_size);
    }

    esp_err_t SIM7000_MQTT::init()
    {
        if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
        auto modem = m_modem.lock();
        ESP_LOGI(TAG, "HERE");
        esp_err_t err = modem->activate_network();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize MQTT client");
        }

        return err;
    }

    esp_err_t SIM7000_MQTT::deinit()
    {
        m_result_info->reset();
        return ESP_OK;
    }

    esp_err_t SIM7000_MQTT::set_config(const mqtt_config_t &config)
    {
        if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
        auto modem = m_modem.lock();
        esp_err_t err;
        std::string cmd_params;

        // veifica la URL del broker
        if (!config.broker || strlen(config.broker) == 0) {
            ESP_LOGE(TAG, "Missing MQTT broker address");
            return ESP_ERR_INVALID_ARG;
        }

        // establece la url y el puerto
        cmd_params.append("=\"URL\",\"").append(config.broker).append("\"");
        if (config.port != 0) {
            cmd_params.append(",\"")
                .append(std::to_string(config.port))
                .append("\"");
        }

        err = modem->execute_cmd(at_cmd_t::SMCONF, cmd_params, m_result_info);
        // verifica si se recibió un identificador de cliente
        if (err == ESP_OK && config.client_id) {
            cmd_params.clear();
            cmd_params.append("=\"CLIENTID\",\"")
                .append(config.client_id)
                .append("\"");
            err = modem->execute_cmd(at_cmd_t::SMCONF, cmd_params, m_result_info);
        }

        // verifica si se recibió un nombre de usuario
        if (err == ESP_OK && config.username) {
            cmd_params.clear();
            cmd_params.append("=\"USERNAME\",\"")
                .append(config.username)
                .append("\"");
            err = modem->execute_cmd(at_cmd_t::SMCONF, cmd_params, m_result_info);
        }

        // verifica si se recibió una contraseña
        if (err == ESP_OK && config.password) {
            cmd_params.clear();
            cmd_params.append("=\"PASSWORD\",\"")
                .append(config.password)
                .append("\"");
            err = modem->execute_cmd(at_cmd_t::SMCONF, cmd_params, m_result_info);
        }

        // verifica si se recibió un keep time diferente de 60
        if (err == ESP_OK && config.keep_time != 60) {
            cmd_params.clear();
            cmd_params.append("=\"KEEPTIME\",")
                .append(std::to_string(config.keep_time));
            err = modem->execute_cmd(at_cmd_t::SMCONF, cmd_params, m_result_info);
        }

        // verifica si se recibió un session cleaning diferente de 60
        if (err == ESP_OK && config.session_cleaning != 0) {
            cmd_params.clear();
            cmd_params.append("=\"CLEANSS\",")
                .append(std::to_string(config.session_cleaning));

            err = modem->execute_cmd(at_cmd_t::SMCONF, cmd_params, m_result_info);
        }

        // verifica si se recibió un QoS diferente de 60
        if (err == ESP_OK && config.qos != 0) {
            cmd_params.clear();
            cmd_params.append("=\"QOS\",")
                .append(std::to_string(config.qos));
            err = modem->execute_cmd(at_cmd_t::SMCONF, cmd_params, m_result_info);
        }

        if (err != ESP_OK) {
            cmd_params.erase(0, 1);

            ESP_LOGE(
                TAG,
                "Failed to set MQTT configuration (param: %s)",
                cmd_params.c_str()
            );
        }
         
        return err;
    }

    esp_err_t SIM7000_MQTT::connect(TickType_t ticks_to_wait)
    {
        if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
        auto modem = m_modem.lock();
        bool current_state = false;
        esp_err_t err;
        // consulta el estado de la conexión MQTT
        err = get_state(current_state);

        if (err == ESP_OK) {
            // verifica si el cliente no está conectado
            if (!current_state) {
                // asegura que la red este activa
                err = modem->activate_network();
            
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Connecting to MQTT broker...");
                    err = modem->execute_cmd(
                        at_cmd_t::SMCONN, 
                        m_result_info, 
                        ticks_to_wait);

                    if (err == ESP_OK) {
                        ESP_LOGI(TAG, "MQTT connection successful");
                    } else {
                        ESP_LOGE(
                            TAG,
                            "Failed to connect to MQTT broker (%s)",
                            esp_err_to_name(err)
                        );
                    }
                } else {
                    ESP_LOGE(
                        TAG,
                        "Failed to activate network (%s)",
                        esp_err_to_name(err)
                    );
                }
            } else {
                ESP_LOGI(TAG, "MQTT client is already connected");
            }
        }
        
        return err;
    }

    esp_err_t SIM7000_MQTT::disconnect()
    {
        if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
        auto modem = m_modem.lock();
        bool current_state = false;
        esp_err_t err;

        // consulta el estado de la conexión MQTT
        err = get_state(current_state);
        if (err == ESP_OK) {
            if (current_state) {
                ESP_LOGI(TAG, "Disconnecting from MQTT broker...");
                err = modem->execute_cmd(at_cmd_t::SMDISC, m_result_info);
                
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "MQTT disconnection successful");
                } else {
                    ESP_LOGE(
                        TAG,
                        "Failed to disconnect from MQTT broker (%s)",
                        esp_err_to_name(err)
                    );
                }
            } else {
                ESP_LOGI(TAG, "MQTT client is already disconnected");
            }
        }
        
        return err;
    }

    esp_err_t SIM7000_MQTT::get_state(bool &state)
    {
        if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
        auto modem = m_modem.lock();
        esp_err_t err;
        std::string &res = m_result_info->response;

        err = modem->execute_cmd(at_cmd_t::SMSTATE, "?", m_result_info);
        if (err == ESP_OK) {
            // verifica si la respuesta termina en 1
            state = res.ends_with("1");
        } else {
            ESP_LOGE(TAG, "Failed to get MQTT state");
        }

        return err;
    }

    esp_err_t SIM7000_MQTT::publish(const char *topic, uint8_t qos, bool retain)
    {
        return publish(topic, std::span<const char>(), qos , retain);
    }

    esp_err_t SIM7000_MQTT::publish(
        const char *topic,
        std::span<const char> msg,
        uint8_t qos,
        bool retain)
    {
        if (!topic || strlen(topic) == 0) return ESP_ERR_INVALID_ARG;
        if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
        
        auto modem = m_modem.lock();
        esp_err_t err;
        std::string params = "=\"";
        params.append(topic);
        params.append("\",");
        params.append(std::to_string(msg.size()));
        params.append(",");
        params.append(std::to_string(qos));
        params.append(",");
        params.append(retain ? "1" : "0");
        
        sim7000_cmd_context_t context;
        context.command = at_cmd_t::SMPUB;
        context.params = params;
        context.payload = msg;
        context.send_payload = true;
        context.result_info = m_result_info;

        err = modem->execute_cmd(context);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to publish MQTT message");
        }

        return err;
    }

    esp_err_t SIM7000_MQTT::subscribe(const char *topic, uint8_t qos)
    {
        if (!topic || strlen(topic) == 0) return ESP_ERR_INVALID_ARG;
        if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
        
        auto modem = m_modem.lock();
        esp_err_t err;
        std::string params = "=\"";
        params.append(topic);
        params.append("\",");
        params.append(std::to_string(qos));

        err = modem->execute_cmd(at_cmd_t::SMSUB, params, m_result_info);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to subscribe to MQTT topic");
        }

        return err;
    }

    esp_err_t SIM7000_MQTT::unsubscribe(const char *topic)
    {
        if (!topic || strlen(topic) == 0) return ESP_ERR_INVALID_ARG;
        if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
        
        auto modem = m_modem.lock();
        esp_err_t err;
        std::string params = "=\"";
        params.append(topic);
        params.append("\"");

        err = modem->execute_cmd(at_cmd_t::SMUNSUB, params, m_result_info);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to unsubscribe from MQTT topic");
        }

        return err;
    }

} // namespace axomotor::lte_modem

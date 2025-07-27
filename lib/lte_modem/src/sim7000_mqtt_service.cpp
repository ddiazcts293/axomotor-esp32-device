#include "sim7000_mqtt_service.hpp"
#include <esp_log.h>

namespace axomotor::lte_modem
{
    static const char *TAG = "sim7000:mqtt";

    SIM7000_MQTT::SIM7000_MQTT(std::weak_ptr<SIM7000_Modem> modem, const int rx_bufsize) : 
        SIM7000_Service(modem),
        m_rx_bufsize{rx_bufsize},
        m_rx_buffer{new char[rx_bufsize]}
    { }

    SIM7000_MQTT::~SIM7000_MQTT()
    {
        delete[] m_rx_buffer;
    }

    esp_err_t SIM7000_MQTT::init()
    {
        esp_err_t result = check_modem_ptr();
        if (result != ESP_OK)
            return result;

        // obtiene una referencia a la instancia del modem
        SIM7000_Modem &modem = *m_modem.lock();

        // activa la red
        result = modem.activate_network();
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize MQTT client");
        }

        return result;
    }

    esp_err_t SIM7000_MQTT::set_config(const mqtt_config_t &config)
    {
        std::string payload;
        esp_err_t result = check_modem_ptr();
        if (result != ESP_OK)
            return result;
        /*
            // veifica la URL del broker
            if (strlen(config.broker) == 0) {
                ESP_LOGE(TAG, "Missing MQTT broker address");
                return ESP_ERR_INVALID_ARG;
            }

            // obtiene una referencia a la instancia del modem
            SIM7000_Modem &modem = *m_modem.lock();

            // establece la url y el puerto
            payload.append("=\"URL\",\"").append(config.broker).append("\"");
            if (config.port != 0) {
                payload.append(",\"")
                    .append(std::to_string(config.port))
                    .append("\"");
            }

            result = modem.execute_internal_cmd_no_answer(at_cmd_t::SMCONF, payload);

            // verifica si se recibió un identificador de cliente
            if (result == ESP_OK && strlen(config.client_id) > 0) {
                payload.clear();
                payload.append("=\"CLIENTID\",\"")
                    .append(config.client_id)
                    .append("\"");

                result = modem.execute_internal_cmd_no_answer(at_cmd_t::SMCONF, payload);
            }

            // verifica si se recibió un nombre de usuario
            if (result == ESP_OK && strlen(config.username) > 0) {
                payload.clear();
                payload.append("=\"USERNAME\",\"")
                    .append(config.username)
                    .append("\"");

                result = modem.execute_internal_cmd_no_answer(at_cmd_t::SMCONF, payload);
            }

            // verifica si se recibió una contraseña
            if (result == ESP_OK && strlen(config.password) > 0) {
                payload.clear();
                payload.append("=\"PASSWORD\",\"")
                    .append(config.password)
                    .append("\"");

                result = modem.execute_internal_cmd_no_answer(at_cmd_t::SMCONF, payload);
            }

            // verifica si se recibió un keep time diferente de 60
            if (result == ESP_OK && config.keep_time != 60) {
                payload.clear();
                payload.append("=\"KEEPTIME\",")
                    .append(std::to_string(config.keep_time));

                result = modem.execute_internal_cmd_no_answer(at_cmd_t::SMCONF, payload);
            }

            // verifica si se recibió un session cleaning diferente de 60
            if (result == ESP_OK && config.session_cleaning != 0) {
                payload.clear();
                payload.append("=\"CLEANSS\",")
                    .append(std::to_string(config.session_cleaning));

                result = modem.execute_internal_cmd_no_answer(at_cmd_t::SMCONF, payload);
            }

            // verifica si se recibió un QoS diferente de 60
            if (result == ESP_OK && config.qos != 0) {
                payload.clear();
                payload.append("=\"QOS\",")
                    .append(std::to_string(config.qos));

                result = modem.execute_internal_cmd_no_answer(at_cmd_t::SMCONF, payload);
            }

            if (result != ESP_OK) {
                ESP_LOGE(
                    TAG,
                    "Failed to set MQTT configuration (%s)",
                    esp_err_to_name(result)
                );
            }
         */
        return result;
    }

    esp_err_t SIM7000_MQTT::connect(TickType_t ticks_to_wait)
    {
        std::string payload;
        esp_err_t result = check_modem_ptr();
        if (result != ESP_OK)
            return result;
        /*
        // obtiene una referencia a la instancia del modem
        SIM7000_Modem &modem = *m_modem.lock();

        // consulta el estado actual de la red
        result = modem.set_net_active_status(network_active_mode_t::ACTIVE);

        if (result == ESP_OK) {
            // obtiene el estado de conexión
            index = response.find_first_of(": ") + 2;
            uint8_t mode = helpers::to_number<uint8_t>(response, index);

            // verifica si el estado actual es desactivado
            if (mode == 0) {
                // activa la red
                ESP_LOGI(TAG, "Network is not active, activating...");
                result = execute_internal_cmd(at_cmd_t::CNACT, 0, "=1");
            }
        }

        if (result == ESP_OK) {
            // consulta el estado de la conexión MQTT
            result = execute_internal_cmd(at_cmd_t::SMSTATE, 0, "?");
        }

        if (result == ESP_OK) {
            // obtiene el estado de conexión MQTT
            index = response.find_first_of(": ") + 2;
            uint8_t status = helpers::to_number<uint8_t>(response, index);

            // verifica si no está conectado con el broker
            if (status == 0) {
                ESP_LOGI(TAG, "MQTT is disconnected, connecting...");

                // intenta conectar con el broker
                result = execute_internal_cmd(at_cmd_t::SMCONN, pdMS_TO_TICKS(10000));
            }
        }

        if (result == ESP_OK) {
            ESP_LOGI(TAG, "MQTT connection success");
        } else {
            ESP_LOGI(
                TAG,
                "Failed to connect to MQTT broker (%s)",
                esp_err_to_name(result)
            );
        }
    */
        return result;
    }

    esp_err_t SIM7000_MQTT::disconnect()
    {
        esp_err_t result = ESP_OK;
        /*
            // espera hasta que el modulo esté disponible
            ESP_LOGI(TAG, "Disconnecting from MQTT broker...");
            m_event_group.wait_for_availability();

            // intenta desconectar del broker
            result = execute_internal_cmd(at_cmd_t::SMDISC, 0);
            if (result == ESP_OK) {
                // desactiva la red
                result = execute_internal_cmd(at_cmd_t::CNACT, 0, "=0");
            }

            if (result == ESP_OK) {
                ESP_LOGI(TAG, "MQTT disconnection success");
            } else {
                ESP_LOGI(
                    TAG,
                    "Failed to disconnect from MQTT broker (%s)",
                    esp_err_to_name(result)
                );
            }

            // hace que el modulo este disponible
            m_event_group.leave();
        */
        return result;
    }

    esp_err_t SIM7000_MQTT::get_status()
    {
        std::string payload;
        esp_err_t result = check_modem_ptr();
        if (result != ESP_OK)
            return result;

        // result = modem.execute_internal_cmd(at_cmd_t::SMSTATE, "?", payload);

        return ESP_OK;
    }

    esp_err_t SIM7000_MQTT::publish(
        const char *topic,
        const char *msg,
        size_t msg_length,
        uint8_t qos)
    {
        return ESP_OK;
    }

} // namespace axomotor::lte_modem

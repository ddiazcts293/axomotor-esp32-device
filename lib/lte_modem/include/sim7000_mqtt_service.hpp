#pragma once

#include <sim7000_modem.hpp>

namespace axomotor::lte_modem
{
    static const int DEFAULT_MQTT_MSG_BUFFER_SIZE = 512;
    static const int DEFAULT_MQTT_RES_BUFFER_SIZE = 128;

    /**
     * @brief Servicio MQTT de SIM7000.
     *
     */
    class SIM7000_MQTT : public SIM7000_Service
    {
    public:
        SIM7000_MQTT(
            std::weak_ptr<SIM7000_Modem> modem, 
            int message_buffer_size = DEFAULT_MQTT_MSG_BUFFER_SIZE,
            int response_buffer_size = DEFAULT_MQTT_RES_BUFFER_SIZE
        );

        esp_err_t init() override;
        esp_err_t deinit() override;

        esp_err_t set_config(const mqtt_config_t &config);
        esp_err_t connect(TickType_t ticks_to_wait = 0);
        esp_err_t disconnect();
        esp_err_t get_state(bool &state);
        esp_err_t publish(const char *topic, uint8_t qos = 1, bool retain = false);
        esp_err_t publish(const char *topic, std::span<const char> msg, uint8_t qos = 1, bool retain = false);
        esp_err_t subscribe(const char *topic, uint8_t qos = 1);
        esp_err_t unsubscribe(const char *topic);

    private:
    };

} // namespace axomotor::lte_modem

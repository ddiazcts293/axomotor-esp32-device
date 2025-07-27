#pragma once

#include <sim7000_modem.hpp>

namespace axomotor::lte_modem
{
    /**
     * @brief Servicio MQTT de SIM7000.
     *
     */
    class SIM7000_MQTT : public SIM7000_Service
    {
    public:
        SIM7000_MQTT(std::weak_ptr<SIM7000_Modem> modem, const int rx_bufsize);

        ~SIM7000_MQTT();

        esp_err_t init() override;
        esp_err_t set_config(const mqtt_config_t &config);
        esp_err_t connect(TickType_t ticks_to_wait = portMAX_DELAY);
        esp_err_t disconnect();
        esp_err_t get_status();
        esp_err_t publish(const char *topic, const char *msg, size_t msg_length, uint8_t qos = 1);
        esp_err_t subscribe(const char *topic, uint8_t qos);
        esp_err_t unsubscribe(const char *topic);

    private:
        const int m_rx_bufsize;
        char *m_rx_buffer;
    };

#if 0
class SIM7000_HTTP : public SIM7000_Service
{
public:
    esp_err_t init() override;
    esp_err_t deinit() override;
};

class SIM7000_NTP : public SIM7000_Service
{
public:
    esp_err_t init() override;
    esp_err_t deinit() override;
};

#endif

} // namespace axomotor::lte_modem

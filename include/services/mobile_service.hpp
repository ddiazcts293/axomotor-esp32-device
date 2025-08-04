#pragma once

#include <service_base.hpp>
#include <sim7000_modem.hpp>
#include <sim7000_gnss_service.hpp>
#include <sim7000_mqtt_service.hpp>

#include <memory>
#include <mutex>

#include "events/event_queue.hpp"

namespace axomotor::services {

class MobileService : public threading::ServiceBase
{
public:
    MobileService();

private:
    std::shared_ptr<lte_modem::SIM7000_Modem> m_modem;
    std::shared_ptr<lte_modem::SIM7000_GNSS> m_gnss;
    std::shared_ptr<lte_modem::SIM7000_MQTT> m_mqtt;

    bool m_gps_enabled;
    bool m_gps_signal_lost;

    esp_err_t setup() override;
    void loop() override;

    esp_err_t publish_position(events::position_event_t &event);
    esp_err_t publish_pong(events::ping_event_t &event);
    esp_err_t publish_event(events::device_event_t &event);

    static void on_event(void *args, esp_event_base_t base, int32_t id, void *data);
};

} // namespace axomotor::services

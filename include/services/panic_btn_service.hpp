#pragma once

#include <service_base.hpp>

namespace axomotor::services {

class PanicBtnService : public threading::ServiceBase
{
public:
    PanicBtnService();

private:
    bool m_was_pressed;
    TickType_t m_last_timestamp;
    uint32_t m_press_events;

    uint32_t m_led_duty;
    bool m_increment;

    esp_err_t setup() override;
    void loop() override;
    void update_led();
};

} // namespace axomotor::services

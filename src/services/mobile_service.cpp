#include "services/mobile_service.hpp"
#include "esp_log.h"

namespace axomotor::services {

using namespace axomotor::constants::hw::modem;

constexpr static const char *TAG = "mobile_service";

MobileService::MobileService() : 
    ServiceBase{TAG, 8 * 1024, 10},
    m_modem{UART_PORT, PIN_MODEM_RX, PIN_MODEM_TX, PIN_MODEM_PWR}
{ }

esp_err_t MobileService::setup()
{
    return m_modem.init();
}

void MobileService::loop()
{
    ESP_LOGI(TAG, "running");
    vTaskDelay(pdMS_TO_TICKS(3000));
}

} // namespace axomotor::services

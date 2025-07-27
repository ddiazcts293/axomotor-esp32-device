#include "services/mobile_service.hpp"
#include "constants/hw.hpp"

#include <esp_log.h>

namespace axomotor::services {

using namespace axomotor::constants::hw::modem;
using namespace axomotor::lte_modem;

constexpr static const char *TAG = "mobile_service";

MobileService::MobileService() : ServiceBase{TAG, 8 * 1024, 10}
{ 
    m_modem = std::make_shared<SIM7000_Modem>(UART_PORT, PIN_U1_RX, PIN_U1_TX, PIN_PWR);
    m_gnss = std::make_shared<SIM7000_GNSS>(m_modem);
    
    apn_config_t config = {
        .apn = "internet.itelcel.com",
        .user = "",
        .pwd = "",
        .cid = 1,
    };

    m_modem->set_apn(config);
}

esp_err_t MobileService::setup()
{
    esp_err_t result = m_modem->init();
    if (result == ESP_OK) {
        m_modem->enable_comm();
    }

    /*
    vTaskDelay(pdMS_TO_TICKS(5000));

    mqtt_config_t config = 
    {
        .client_id = "esp32-0323105860",
        .broker = "broker.emqx.io",
        .port = 1883,
        .session_cleaning = true,
        .qos = 1
    };

    m_modem.set_mqtt_config(config);
    m_modem.connect_to_mqtt(); */

    return result;
}

void MobileService::loop()
{
    gnss_nav_info_t info{};
    int8_t signal_quality;
    std::string op_name;
    network_reg_status_t reg_status;

    m_modem->get_signal_strength(signal_quality);
    m_modem->get_current_operator(op_name);
    m_modem->get_network_reg_status(reg_status);

    m_gnss->get_nav_info(info);
    ESP_LOGI(TAG, "latitude=%.6f, longitude=%.6f", info.latitude, info.longitude);

    vTaskDelay(pdMS_TO_TICKS(10000));
}

} // namespace axomotor::services

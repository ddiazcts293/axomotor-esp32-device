#include "sim7000_gnss_service.hpp"
#include "sim7000_helpers.hpp"
#include <esp_log.h>

namespace axomotor::lte_modem {

using namespace axomotor::lte_modem::internal;

static const char *TAG = "sim7000:gnss";

SIM7000_GNSS::SIM7000_GNSS(std::weak_ptr<SIM7000_Modem> modem) :
    SIM7000_Service(modem)
{ 
    m_result_info->response.reserve(120);
}

esp_err_t SIM7000_GNSS::turn_on()
{
    if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
    auto modem = m_modem.lock();

    ESP_LOGI(TAG, "Turning GNSS on");
    esp_err_t err = modem->execute_cmd(at_cmd_t::CGNSPWR, "=1", m_result_info);
        
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to turn on GNSS");
    }
    
    return err;
}

esp_err_t SIM7000_GNSS::turn_off()
{
    if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
    auto modem = m_modem.lock();

    ESP_LOGI(TAG, "Turning GNSS off");
    esp_err_t err = modem->execute_cmd(at_cmd_t::CGNSPWR, "=0", m_result_info);
        
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to turn off GNSS");
    }
    
    return err;
}

esp_err_t SIM7000_GNSS::get_state(bool &state)
{
    if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
    auto modem = m_modem.lock();

    ESP_LOGI(TAG, "Getting GNSS state...");
    esp_err_t err = modem->execute_cmd(at_cmd_t::CGNSPWR, "?", m_result_info);
    std::string &response = m_result_info->response;
    
    if (err == ESP_OK) {
        helpers::remove_before(response, ": ");
        state = response == "1" ? true : false;
    } else {
        ESP_LOGE(TAG, "Failed to get GNSS state");
    }

    return err;
}

esp_err_t SIM7000_GNSS::enable_nav_urc(uint8_t interval)
{
    if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
    auto modem = m_modem.lock();
    std::string params = "=";
    params.append(std::to_string(interval));

    ESP_LOGI(TAG, "Enabling GNSS reporting...");

    esp_err_t err = modem->execute_cmd(at_cmd_t::CGNSURC, params, m_result_info);        
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable GNSS reporting");
    }
    
    return err;
}

esp_err_t SIM7000_GNSS::disable_nav_urc()
{
    if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
    auto modem = m_modem.lock();

    ESP_LOGI(TAG, "Disabling GNSS reporting...");

    esp_err_t err = modem->execute_cmd(at_cmd_t::CGNSURC, "=0", m_result_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable GNSS reporting");
    }
    
    return err;
}

esp_err_t SIM7000_GNSS::get_nav_urc_state(bool &state)
{
    if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
    auto modem = m_modem.lock();
    std::string &res = m_result_info->response;

    ESP_LOGI(TAG, "Getting GNSS reporting state...");

    esp_err_t err = modem->execute_cmd(at_cmd_t::CGNSURC, "?", m_result_info);
    if (err == ESP_OK) {
        state = !res.ends_with("0");
    } else {
        ESP_LOGE(TAG, "Failed to get GNSS reporting state");
    }
    
    return err;
}

esp_err_t SIM7000_GNSS::get_nav_info(gnss_nav_info_t &info)
{
    if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
    auto modem = m_modem.lock();

    // espera hasta que el modulo esté disponible
    ESP_LOGI(TAG, "Getting GNSS navigation information...");
    esp_err_t result = modem->execute_cmd(at_cmd_t::CGNSINF, m_result_info);
    std::string &response = m_result_info->response;
    std::string aux;

    if (result == ESP_OK) {
        helpers::parse_gnss_info(response, info);
    } else {
        ESP_LOGE(TAG, "Failed to retrieve GNSS information");
    }

    return result;
}

} // namespace axomotor::lte_modem

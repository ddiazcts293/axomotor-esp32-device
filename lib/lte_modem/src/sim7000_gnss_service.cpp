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

esp_err_t SIM7000_GNSS::set_state(bool state)
{
    if (m_modem.expired()) return ESP_ERR_INVALID_STATE;
    auto modem = m_modem.lock();

    ESP_LOGI(TAG, "Turning GNSS %s...",  state ? "on" : "off");
    esp_err_t err = modem->execute_cmd(
        at_cmd_t::CGNSPWR,
        state ? "=1" : "=0",
        m_result_info);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to change GNSS state");
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
        // quita el inicio del comando
        helpers::remove_before(response, ": ");
        // obtiene el estado de ejecución
        helpers::extract_token(response, 0, ",", aux, true);
        info.run_status = helpers::to_number<uint8_t>(aux);
        // obtiene el indicador FIX
        helpers::extract_token(response, 1, ",", aux, true);
        info.fix_status = helpers::to_number<uint8_t>(aux);//fields[1]);
        // obtiene la fecha y hora
        helpers::extract_token(response, 2, ",", aux, true);
        info.date_time = helpers::to_number<uint64_t>(aux);//fields[2]);
        // obtiene la latitud
        helpers::extract_token(response, 3, ",", aux, true);
        info.latitude = helpers::to_number<float>(aux);//fields[3]);
        // obtiene la longitud
        helpers::extract_token(response, 4, ",", aux, true);
        info.longitude = helpers::to_number<float>(aux);//fields[4]);
        // obtiene la altitud
        helpers::extract_token(response, 5, ",", aux, true);
        info.msl_altitude = helpers::to_number<float>(aux);//fields[5]);
        // obtiene la velocidad
        helpers::extract_token(response, 6, ",", aux, true);
        info.speed_over_ground = helpers::to_number<float>(aux);//fields[6]);
        // obtiene el curso sobre la tierra
        helpers::extract_token(response, 7, ",", aux, true);
        info.course_over_ground = helpers::to_number<float>(aux);//fields[7]);
        // obtiene el indicador FIX MODE
        helpers::extract_token(response, 8, ",", aux, true);
        info.fix_mode = helpers::to_number<uint8_t>(aux);//fields[8]);
        // obtiene la cantidad de satelites de GNSS en vista
        helpers::extract_token(response, 14, ",", aux, true);
        info.gnss_satellites = helpers::to_number<uint8_t>(aux);//fields[14]);
        // obtiene la cantidad de satelites de GPS en vista
        helpers::extract_token(response, 15, ",", aux, true);
        info.gps_satellites = helpers::to_number<uint8_t>(aux);//fields[15]);
    } else {
        ESP_LOGE(TAG, "Failed to retrieve GNSS information");
    }

    return result;
}

} // namespace axomotor::lte_modem

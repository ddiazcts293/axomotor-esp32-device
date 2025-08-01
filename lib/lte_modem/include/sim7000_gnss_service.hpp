#pragma once

#include <sim7000_modem.hpp>

namespace axomotor::lte_modem
{
    class SIM7000_GNSS : public SIM7000_Service
    {
    public:
        SIM7000_GNSS(std::weak_ptr<SIM7000_Modem> modem);

        esp_err_t turn_on();
        esp_err_t turn_off();
        esp_err_t get_state(bool &state);
        esp_err_t enable_nav_urc(uint8_t interval);
        esp_err_t disable_nav_urc();
        esp_err_t get_nav_urc_state(bool &state);
        esp_err_t get_nav_info(gnss_nav_info_t &info);
    };

} // namespace axomotor::lte_modem

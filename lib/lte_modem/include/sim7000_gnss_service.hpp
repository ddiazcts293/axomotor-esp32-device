#pragma once

#include <sim7000_modem.hpp>

namespace axomotor::lte_modem
{
    class SIM7000_GNSS : public SIM7000_Service
    {
    public:
        SIM7000_GNSS(std::weak_ptr<SIM7000_Modem> modem);

        esp_err_t set_state(bool state);
        esp_err_t get_state(bool &state);
        esp_err_t set_urc_state(bool state);
        esp_err_t get_nav_info(gnss_nav_info_t &info);
    };

} // namespace axomotor::lte_modem

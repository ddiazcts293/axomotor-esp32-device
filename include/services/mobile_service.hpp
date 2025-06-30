#pragma once

#include <service_base.hpp>
#include <sim7000_modem.hpp>
#include "constants/hw.hpp"

namespace axomotor::services {

class MobileService : public threading::ServiceBase
{
public:
    MobileService();

private:
    esp_err_t setup() override;
    void loop() override;

    

    axomotor::lte_modem::SIM7000Modem m_modem;
};

} // namespace axomotor::services

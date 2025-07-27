#pragma once

#include <service_base.hpp>
#include <sim7000_modem.hpp>
#include <sim7000_gnss_service.hpp>
#include <memory>

namespace axomotor::services {

class MobileService : public threading::ServiceBase
{
public:
    MobileService();

private:
    esp_err_t setup() override;
    void loop() override;

    std::shared_ptr<axomotor::lte_modem::SIM7000_Modem> m_modem;
    std::shared_ptr<axomotor::lte_modem::SIM7000_GNSS> m_gnss;
};

} // namespace axomotor::services

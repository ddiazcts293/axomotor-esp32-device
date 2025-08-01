#pragma once

#include <service_base.hpp>

namespace axomotor::services {

class CameraService : public threading::ServiceBase
{
public:
    CameraService();

private:
    esp_err_t setup() override;
    void loop() override;
};

} // namespace axomotor::services

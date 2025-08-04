#pragma once

#include <service_base.hpp>

namespace axomotor::services {

class ImageService : public threading::ServiceBase
{
public:
    ImageService();

private:
    esp_err_t setup() override;
    void loop() override;
};

} // namespace axomotor::services

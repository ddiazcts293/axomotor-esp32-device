#pragma once

#include <service_base.hpp>

namespace axomotor::services {

class BluetoothService : public threading::ServiceBase
{
public:
    BluetoothService();

private:
    esp_err_t setup() override;
    void loop() override;
};

} // namespace axomotor::services

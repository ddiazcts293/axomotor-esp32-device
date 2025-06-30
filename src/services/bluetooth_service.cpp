#include "services/bluetooth_service.hpp"
#include "esp_log.h"

namespace axomotor::services {

constexpr static const char *TAG = "bt_service";

BluetoothService::BluetoothService() : 
    ServiceBase{TAG, 8 * 1024, 10}
{ }

esp_err_t BluetoothService::setup()
{
    return ESP_OK;
}

void BluetoothService::loop()
{

    ESP_LOGI(TAG, "running");
    vTaskDelay(pdMS_TO_TICKS(5000));
}

} // namespace axomotor::services

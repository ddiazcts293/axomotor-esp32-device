#include "services/camera_service.hpp"
#include "esp_log.h"

namespace axomotor::services {

constexpr static const char *TAG = "camera_service";

CameraService::CameraService() : 
    ServiceBase{TAG, 8 * 1024, 10}
{ }

esp_err_t CameraService::setup()
{
    return ESP_OK;
}

void CameraService::loop()
{
    ESP_LOGI(TAG, "running");
    vTaskDelay(pdMS_TO_TICKS(2000));
}

} // namespace axomotor::services

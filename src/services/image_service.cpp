#include "services/image_service.hpp"
#include "constants/hw.hpp"

#include <esp_log.h>
#include <esp_camera.h>

namespace axomotor::services {

using namespace axomotor::constants::hw::camera;

constexpr static const char *TAG = "image_service";

CameraService::CameraService() : 
    ServiceBase{TAG, 8 * 1024, 10}
{ 
    const camera_config_t camera_config = {
        .pin_pwdn = PIN_PWDN,
        .pin_reset = PIN_RESET,
        .pin_xclk = PIN_XCLK,
        .pin_sccb_sda = PIN_SIOD,
        .pin_sccb_scl = PIN_SIOC,
        .pin_d7 = PIN_Y9,
        .pin_d6 = PIN_Y8,
        .pin_d5 = PIN_Y7,
        .pin_d4 = PIN_Y6,
        .pin_d3 = PIN_Y5,
        .pin_d2 = PIN_Y4,
        .pin_d1 = PIN_Y3,
        .pin_d0 = PIN_Y2,
        .pin_vsync = PIN_VSYNC,
        .pin_href = PIN_HREF,
        .pin_pclk = PIN_PCLK,

        .xclk_freq_hz = XCLK_FREQ_HZ,
        .ledc_timer = LEDC_TIMER,
        .ledc_channel = LEDC_CHANNEL,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_SVGA,
        .jpeg_quality = 12,
        .fb_count = FB_COUNT,
        .fb_location = FB_LOCATION,
        .grab_mode = GRAB_MODE
    };

    


}

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

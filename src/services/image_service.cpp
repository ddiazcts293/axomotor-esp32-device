#include "services/image_service.hpp"
#include "services/axomotor_service.hpp"
#include "constants/hw.hpp"

#include <esp_log.h>
#include <esp_camera.h>

namespace axomotor::services {

using namespace axomotor::constants::hw::camera;

constexpr static const char *TAG = "image_service";

ImageService::ImageService() : 
    ServiceBase{TAG, 4 * 1024, 10}
{ }

esp_err_t ImageService::setup()
{
    ESP_LOGI(TAG, "Initializing camera...");

    uint8_t attempt_num = 5;
    esp_err_t err;
    camera_config_t config{};
    config.pin_pwdn = PIN_PWDN;
    config.pin_reset = PIN_RESET;
    config.pin_xclk = PIN_XCLK;
    config.pin_sccb_sda = PIN_SIOD;
    config.pin_sccb_scl = PIN_SIOC;
    config.pin_d7 = PIN_Y9;
    config.pin_d6 = PIN_Y8;
    config.pin_d5 = PIN_Y7;
    config.pin_d4 = PIN_Y6;
    config.pin_d3 = PIN_Y5;
    config.pin_d2 = PIN_Y4;
    config.pin_d1 = PIN_Y3;
    config.pin_d0 = PIN_Y2;
    config.pin_vsync = PIN_VSYNC;
    config.pin_href = PIN_HREF;
    config.pin_pclk = PIN_PCLK;
    config.xclk_freq_hz = XCLK_FREQ_HZ;
    config.ledc_timer = LEDC_TIMER;
    config.ledc_channel = LEDC_CHANNEL;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = FB_COUNT;
    config.fb_location = FB_LOCATION;
    config.grab_mode = GRAB_MODE;
    
    do
    {
        err = esp_camera_init(&config);
        if (err == ESP_OK) {
            break;
        }

        ESP_LOGW(TAG, "Attempting to initialize camera...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        attempt_num--;
    } while (attempt_num);
    
    if (err != ESP_OK) {
        ESP_LOGE(
            TAG, 
            "Failed to initialize camera (%s)",
            esp_err_to_name(err)
        );
        
        return err;
    }

    return err;
}

void ImageService::loop()
{
    // espera hasta que haya un viaje disponible
    AxoMotor::event_group.wait_until_system_is_ready();

    ESP_LOGI(TAG, "Taking picture...");
    camera_fb_t *pic = esp_camera_fb_get();

    // use pic->buf to access the image
    ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
    esp_camera_fb_return(pic);

    vTaskDelay(pdMS_TO_TICKS(5000));
}

} // namespace axomotor::services

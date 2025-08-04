#include "services/image_service.hpp"
#include "services/axomotor_service.hpp"
#include "constants/hw.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <esp_log.h>
#include <esp_camera.h>

namespace axomotor::services {

using namespace axomotor::events;
using namespace axomotor::constants::hw::camera;

constexpr static const char *TAG = "image_service";

ImageService::ImageService() : 
    ServiceBase{TAG, 4 * 1024, 10},
    m_is_active{false}
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
    bool is_ready = AxoMotor::event_group.wait_until_system_is_ready(pdMS_TO_TICKS(1000));
    
    if (is_ready && !m_is_active) {
        m_is_active = true;
        m_current_dir = SD_MOUNT_POINT "/";
        m_current_dir.append(std::to_string(AxoMotor::get_trip_count()));
        m_current_dir.append("/");

        ensure_dir_exists();
        AxoMotor::queue_set.device.send_to_back(event_code_t::VIDEO_RECORDING_STARTED);
    } else if (!is_ready && m_is_active) {
        m_is_active = false;
        AxoMotor::queue_set.device.send_to_back(event_code_t::VIDEO_RECORDING_STOPPED);
    }

    if (is_ready) {
        ESP_LOGI(TAG, "Taking picture...");
        camera_fb_t *pic = esp_camera_fb_get();
        save_image(pic);
        esp_camera_fb_return(pic);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

bool ImageService::ensure_dir_exists()
{
    struct stat st;

    if (stat(m_current_dir.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return true;
        } else {
            ESP_LOGW(TAG, "'%s' is not a directory", m_current_dir.c_str());
            return false;
        }
    }

    if (mkdir(m_current_dir.c_str(), 0775) == 0) {
        return true;
    } else {
        ESP_LOGE(
            TAG, 
            "Could not create directory: %s (errno=%d)", 
            m_current_dir.c_str(), 
            errno
        );
        
        return false;
    }
}

bool ImageService::save_image(camera_fb_t *fb)
{
    if (!fb || fb->len == 0) {
        ESP_LOGW(TAG, "Corrupted image");
        return false;
    } 

    char path[32];
    snprintf(
        path, 
        sizeof(path), 
        "%s%llX.jpg", 
        m_current_dir.c_str(), 
        events::get_timestamp()
    );

    ESP_LOGI(TAG, "Save image to '%s'", path);

    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing (errno=%d)", errno);
        return false;
    }

    size_t wlen = fwrite(fb->buf, sizeof(uint8_t), fb->len, f);
    fclose(f);

    if (wlen == fb->len) {
        ESP_LOGI(TAG, "Image saved successfully (%u bytes)", wlen);
    } else {
        ESP_LOGI(TAG, "Failed to save image (%u/%u bytes written)", wlen, fb->len);
        return false;
    }

    return true;
}

} // namespace axomotor::services

#include <memory>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "services/camera_service.hpp"
#include "services/mobile_service.hpp"
#include "services/bluetooth_service.hpp"

using namespace std;
using namespace axomotor::services;

static const char *TAG = "app_main";

extern "C" void app_main() 
{
    auto camera_service = make_shared<CameraService>();
    auto mobile_service = make_shared<MobileService>();
    auto bluetooth_service = make_shared<BluetoothService>();
    camera_service->start();
    mobile_service->start();
    bluetooth_service->start();

    vTaskDelay(portMAX_DELAY);
    
    while (true)
    {
        ESP_LOGI(TAG, "Hello world");
    }
}

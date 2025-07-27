#include <memory>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_memory_utils.h>

#include "services/camera_service.hpp"
#include "services/mobile_service.hpp"
#include "services/bluetooth_service.hpp"

using namespace std;
using namespace axomotor::services;

static const char *TAG = "app_main";

extern "C" void app_main() 
{
    vTaskDelay(pdMS_TO_TICKS(5000));

    //auto camera_service = make_shared<CameraService>();
    auto mobile_service = make_shared<MobileService>();
    //auto bluetooth_service = make_shared<BluetoothService>();
    //camera_service->start();
    mobile_service->start();
    //bluetooth_service->start();

    while (true)
    {
        uint32_t free_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        uint32_t max_heap_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        uint32_t heap_total_size = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);

        ESP_LOGI(TAG, "HEAP: free=%lu, max_block=%lu, total=%lu", 
            free_heap, max_heap_block, heap_total_size);
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

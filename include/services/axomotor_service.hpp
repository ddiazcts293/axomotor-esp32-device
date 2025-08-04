#pragma once

#include <memory>
#include <string>

#include <event_group.hpp>
#include <service_base.hpp>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_http_server.h>

#include "services/image_service.hpp"
#include "services/mobile_service.hpp"
#include "services/panic_btn_service.hpp"
#include "services/sensor_service.hpp"
#include "events/event_queue.hpp"
#include "events/global_event_group.hpp"
#include "constants/general.hpp" 

namespace axomotor::services
{
    class AxoMotorService
    {    
    public:
        static const events::EventQueueSet queue_set;
        static const events::GlobalEventGroup event_group;
        
        static void init();
        static const char *get_current_trip_id();
        static uint32_t get_trip_count();

    private:
        static char s_current_trip_id[constants::general::TRIP_ID_LENGTH + 1];
        static char *s_buffer;
        static bool s_is_initialized;
        static uint32_t s_trip_count;
        static vprintf_like_t s_default_writer;
        static httpd_handle_t s_httpd_handle;

        static void init_wifi();
        static void init_httpd();
        static esp_err_t init_sd();
        static esp_err_t read_trip_id();
        static esp_err_t write_trip_id();
        static esp_err_t delete_trip_id();
        static esp_err_t send_response(httpd_req_t *req, bool success, const char *message = NULL);
        static int64_t get_query_param_i64(httpd_req_t *req, const char *key, int64_t def = 0);

        static void enable_log_to_file();
        static void disable_log_to_file();
        static void restart();
        
        static esp_err_t on_index_req(httpd_req_t *req);
        static esp_err_t on_start_trip_req(httpd_req_t *req);
        static esp_err_t on_stop_trip_req(httpd_req_t *req);
        static esp_err_t on_list_images_req(httpd_req_t *req);
        static esp_err_t on_get_image_req(httpd_req_t *req);
        
        static void on_wifi_event(void *, esp_event_base_t, int32_t, void *);
        static int on_write_log(const char *str, va_list args);
    };

    using AxoMotor = AxoMotorService;
}

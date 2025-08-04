#pragma once

#include <freertos/FreeRTOS.h>

#define SD_MOUNT_POINT "/sdcard"

namespace axomotor::constants::general {

constexpr const size_t MAX_JSON_LENGTH = 8192;
constexpr const size_t FILE_CHUNK_SIZE = 1024;
constexpr const size_t TRIP_ID_LENGTH = 24;
constexpr const size_t HTTPD_STACK_SIZE = 8192;

constexpr const size_t POSITION_EVENTS_QUEUE_LENGTH = 30;
constexpr const size_t DEVICE_EVENTS_QUEUE_LENGTH = 10;
constexpr const size_t PING_EVENTS_QUEUE_LENGTH = 1;
constexpr const size_t NETWORK_EVENTS_QUEUE_LENGTH = 1;

constexpr const int POSITION_REPORT_INTERVAL = 20;

constexpr const int PANIC_BTN_EVENTS_TO_CONFIRM = 3;

}

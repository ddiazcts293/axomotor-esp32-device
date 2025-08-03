#pragma once

#include <freertos/FreeRTOS.h>

namespace axomotor::constants::general {

constexpr const size_t POSITION_EVENTS_QUEUE_LENGTH = 30;
constexpr const size_t DEVICE_EVENTS_QUEUE_LENGTH = 10;
constexpr const size_t PING_EVENTS_QUEUE_LENGTH = 1;
constexpr const size_t NETWORK_EVENTS_QUEUE_LENGTH = 1;

constexpr const int POSITION_REPORT_INTERVAL = 20;

}

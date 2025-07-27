#pragma once

#include <hal/gpio_types.h>
#include <hal/uart_types.h>
#include <hal/ledc_types.h>
#include <esp_camera.h>

namespace axomotor::constants::hw {

namespace modem {

constexpr const uart_port_t UART_PORT = UART_NUM_1;
constexpr const int UART_BUFFER_SIZE = 1024;
constexpr const int UART_BAUD_RATE = 115200;

constexpr const int PIN_U1_TX = 1;
constexpr const int PIN_U1_RX = 2;
constexpr const int PIN_PWR = 42;

} // namespace modem

namespace camera {

constexpr const int PIN_PWDN   = -1;
constexpr const int PIN_RESET  = -1;
constexpr const int PIN_SIOD   = 4;
constexpr const int PIN_SIOC   = 5;
constexpr const int PIN_VSYNC  = 6;
constexpr const int PIN_HREF   = 7;
constexpr const int PIN_XCLK   = 15;
constexpr const int PIN_Y9 = 16;
constexpr const int PIN_Y8 = 17;
constexpr const int PIN_Y7 = 18;
constexpr const int PIN_Y4 = 8;
constexpr const int PIN_Y3 = 9;
constexpr const int PIN_Y5 = 10;
constexpr const int PIN_Y2 = 11;
constexpr const int PIN_Y6 = 12;
constexpr const int PIN_PCLK   = 13;

constexpr const int XCLK_FREQ_HZ = 20000000;
constexpr const int FB_COUNT = 2;
constexpr const camera_fb_location_t FB_LOCATION = CAMERA_FB_IN_PSRAM;
constexpr const camera_grab_mode_t GRAB_MODE = CAMERA_GRAB_LATEST;
constexpr const ledc_timer_t LEDC_TIMER = LEDC_TIMER_0;
constexpr const ledc_channel_t LEDC_CHANNEL = LEDC_CHANNEL_0;

} // namespace camera


} // namespace axomotor::constants::hw

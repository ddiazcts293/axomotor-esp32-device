#pragma once

#include <hal/gpio_types.h>
#include <hal/uart_types.h>

namespace axomotor::constants::hw {

namespace modem {

constexpr const uart_port_t UART_PORT = UART_NUM_1;
constexpr const int UART_BUFFER_SIZE = 1024;
constexpr const int UART_BAUD_RATE = 115200;

constexpr const int PIN_MODEM_TX = 18;
constexpr const int PIN_MODEM_RX = 8;
constexpr const int PIN_MODEM_PWR = 7;

} // namespace modem

} // namespace axomotor::constants::hw

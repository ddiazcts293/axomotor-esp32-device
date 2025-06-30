#pragma once

#include "sim7000_types.hpp"

#include <string>

#include <esp_err.h>
#include <hal/uart_types.h>
#include <hal/gpio_types.h>
#include <service_base.hpp>

namespace axomotor::lte_modem {

class SIM7000Modem : private axomotor::threading::ServiceBase
{
public:
    static const int DEFAULT_BAUD_RATE = 115200;
    static const int DEFAULT_RX_BUF_SIZE = 1024;
    static const int DEFAULT_EVENT_QUEUE_SIZE = 16;

    SIM7000Modem(uart_port_t port, 
        int pin_rx, 
        int pin_tx,
        int pin_pwr,
        int baud_rate = DEFAULT_BAUD_RATE,
        int rx_buf_size = DEFAULT_RX_BUF_SIZE,
        int event_queue_size = DEFAULT_EVENT_QUEUE_SIZE
    );
    
    esp_err_t init();

private:
    esp_err_t setup() override;
    void loop() override;
    
    void process_uart_data(size_t size);

    esp_err_t send_command(at_cmd_t command);


    result_code_t read_response_code(const std::string &response);

    const uart_port_t m_port;
    const gpio_num_t m_pin_pwr;
    
    QueueHandle_t m_uart_event_queue;
};

} // namespace axomotor::lte_modem

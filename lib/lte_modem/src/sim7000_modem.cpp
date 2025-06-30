#include "sim7000_modem.hpp"

#include <cstring>

#include <string>

#include <esp_log.h>
#include <driver/uart.h>
#include <driver/gpio.h>

namespace axomotor::lte_modem {

static const char *TAG = "lte_modem";

SIM7000Modem::SIM7000Modem(
    uart_port_t port, 
    int pin_rx, 
    int pin_tx,
    int pin_pwr,
    int baud_rate,
    int rx_buf_size,
    int event_queue_size
) : 
    ServiceBase{TAG, 4 * 1024, 20},
    m_port{port}, m_pin_pwr{(gpio_num_t)pin_pwr}
{ 
    // configuración del puerto UART
    const uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE, 
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // configura el pint de control
    gpio_reset_pin(m_pin_pwr);
    gpio_set_direction(m_pin_pwr, GPIO_MODE_OUTPUT);

    // instalación de drivers para cada puerto UART
    ESP_ERROR_CHECK(uart_driver_install(
        m_port, 
        rx_buf_size, 
        0, 
        event_queue_size, 
        &m_uart_event_queue, 
        0
    ));
    // configuración de cada puerto UART
    ESP_ERROR_CHECK(uart_param_config(m_port, &uart_config));
    // asignación de pines para el puerto UART
    ESP_ERROR_CHECK(uart_set_pin(
        m_port, 
        pin_tx, 
        pin_rx, 
        UART_PIN_NO_CHANGE, 
        UART_PIN_NO_CHANGE
    ));
}

esp_err_t SIM7000Modem::init()
{
    // inicia el servicio subyacente encargado de recibir datos del modulo 
    return start();
}

/* Métodos privados */

esp_err_t SIM7000Modem::setup()
{
    // configura el pin de control
    gpio_set_level(m_pin_pwr, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(m_pin_pwr, 1);
    
    return ESP_OK;
} 

void SIM7000Modem::loop()
{
    uart_event_t event;

    // espera hasta recibir un evento
    if (xQueueReceive(m_uart_event_queue, &event, portMAX_DELAY)) {
        switch (event.type) {
            case UART_DATA:
                if (event.size != 0) {
                    ESP_LOGI(TAG, "Received data from UART (%u bytes)", event.size);
                    process_uart_data(event.size);
                }

                break;
            case UART_FIFO_OVF:
                ESP_LOGW(TAG, "UART FIFO overflow");
                break;
            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "UART buffer full");
                break;
            case UART_PARITY_ERR:
            case UART_FRAME_ERR:
                ESP_LOGE(TAG, "UART error");
                break;
            default:
                break;
        }
    }

    /*
    // lee los datos del buffer UART y obtiene la cantidad de bytes leídos
    int bytes_read = uart_read_bytes(
        m_port, 
        m_buffer, 
        sizeof(m_buffer), 
        pdMS_TO_TICKS(1000)); // por un segundo

    // verifica si se obtuvieron datos
    if (bytes_read > 0) {
        m_buffer[bytes_read] = '\0';
        std::string response{m_buffer, (size_t)bytes_read};

        ESP_LOGI(TAG, "UART received: %s", m_buffer);

        if (response.starts_with('+')) {
            ESP_LOGI(TAG, "Unsolicited Result Code");
        } else {
            ESP_LOGI(TAG, "Response");
        }
    }*/
}

void SIM7000Modem::process_uart_data(size_t size)
{
    // crea un buffer para almacenar el contenido de los datos
    char *buffer = new char[size];
    uart_read_bytes(m_port, buffer, size, portMAX_DELAY);

    // lee el código
    

    // elimina el buffer
    delete[] buffer;
}

result_code_t SIM7000Modem::read_response_code(const std::string &response)
{
    const std::size_t npos = std::string::npos;
    const char line_end[] = "\r\n";
    std::size_t length = response.length();
    std::size_t index;
    result_code_t code = result_code_t::Unknown;
/*
    // verifica si la respuesta es válida
    if (!response.ends_with(line_end)) {
        ESP_LOGE("modem", "Invalid response code received");
        return code;
    }

    length -= 2;
    for (size_t i = 0; i < RESPONSE_CODE_TABLE_SIZE; i++)
    {
        auto value = &RESPONSE_CODE_TABLE[i];

        index = response.find_last_of(value->string, length);
        if (index != npos) 
        {
            ESP_LOGE("modem", "Response code: %s", value->string);

            code = value->code;
            length = npos;
            break;
        }
    }
    */
    return code;
}

} // namespace axomotor::sim7000

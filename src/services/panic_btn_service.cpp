#include "services/panic_btn_service.hpp"
#include "services/axomotor_service.hpp"
#include "constants/hw.hpp"
#include "constants/general.hpp"

#include <driver/gpio.h>

namespace axomotor::services {

using namespace axomotor::constants::hw::panic_btn;
using namespace axomotor::constants::general;

static const char *TAG = "panic_btn_service";

PanicBtnService::PanicBtnService() : 
    ServiceBase(TAG, 4096, 2),
    m_was_pressed{false},
    m_last_timestamp{0},
    m_press_events{0},
    m_led_duty{0},
    m_increment{true}
{ 
    gpio_reset_pin(PIN_BUTTON);
    gpio_set_direction(PIN_BUTTON, GPIO_MODE_INPUT);
    //gpio_reset_pin(PIN_LED_BUTTON);
    //gpio_set_direction(PIN_LED_BUTTON, GPIO_MODE_OUTPUT);

    ledc_timer_config_t ledc_timer{};
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
    ledc_timer.timer_num = LEDC_TIMER;
    ledc_timer.freq_hz = 4000;
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel{};
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel = LEDC_CHANNEL;
    ledc_channel.timer_sel = LEDC_TIMER;
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num = PIN_LED_BUTTON;
    ledc_channel.duty = 0;
    ledc_channel.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

esp_err_t PanicBtnService::setup()
{
    m_was_pressed = gpio_get_level(PIN_BUTTON) == 0;
    m_last_timestamp = xTaskGetTickCount();
    return ESP_OK;
}

void PanicBtnService::loop()
{
    TickType_t timestamp, interval;
    bool is_pressed = gpio_get_level(PIN_BUTTON) == 0;

    // verifica si el botón ha cambiado de estado
    if (is_pressed != m_was_pressed) {
        // establece el nuevo estado
        m_was_pressed = is_pressed;
        timestamp = xTaskGetTickCount();
        // intervalo desde el último cambio de estado
        interval = timestamp - m_last_timestamp;

        if (is_pressed && interval > BUTTON_DEBOUNCE_DELAY) {
            if (interval <= BUTTON_MAX_DELAY_BETWEEN) {
                ESP_LOGI(TAG, "Button times pressed: %lu", m_press_events);
                m_press_events++;
            }
        }

        m_last_timestamp = timestamp;
    }

    if (m_press_events >= PANIC_BTN_EVENTS_TO_CONFIRM) {
        ESP_LOGI(TAG, "Panic Button activated");
        AxoMotor::queue_set.device.send_to_front(events::event_code_t::PANIC_BUTTON_PRESSED);
        m_press_events = 0;
    }

    update_led();
    vTaskDelay(pdMS_TO_TICKS(1));
}

void PanicBtnService::update_led()
{
    if (m_led_duty == 0 && !m_increment) {
        m_increment = true;
    } else if (m_led_duty > (1 << 13) - 2 && m_increment) {
        m_increment = false;
    }

    m_led_duty += (m_increment ? 2 : -2);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, m_led_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
}

}

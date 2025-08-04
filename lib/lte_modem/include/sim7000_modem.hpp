#pragma once

#include "sim7000_types.hpp"
#include "sim7000_basic_modem.hpp"

#include <memory>
#include <mutex>
#include <freertos/FreeRTOS.h>

#include <esp_err.h>
#include <esp_event.h>
#include <hal/uart_types.h>
#include <hal/gpio_types.h>
#include <service_base.hpp>
#include <event_group.hpp>

namespace axomotor::lte_modem
{
    ESP_EVENT_DECLARE_BASE(MODEM_EVENTS);

    enum {
        MODEM_EVENT_STARTED,
        MODEM_EVENT_STOPPED,
        MODEM_EVENT_DST_UPDATED,
        MODEM_EVENT_DATE_TIME_UPDATED,
        MODEM_EVENT_PDP_DEACTIVE,
        MODEM_EVENT_APP_PDP_ACTIVE,
        MODEM_EVENT_APP_PDP_DEACTIVE,
        MODEM_EVENT_FUNC_CHANGED,
        MODEM_EVENT_SIM_CARD_STATUS_CHANGED,
        MODEM_EVENT_SIM_CARD_NOT_READY,
        MODEM_EVENT_SIM_CARD_NOT_INSERTED,
        MODEM_EVENT_MQTT_MESSAGE_RECEIVED,
        MODEM_EVENT_GNSS_NAVIGATION_REPORT,
    };

    /* Constantes */

    static const int DEFAULT_BAUD_RATE = 115200;
    static const int DEFAULT_RX_BUF_SIZE = 1024;
    static const int DEFAULT_RESPONSE_BUF_SIZE = 1024;
    static const int DEFAULT_PARSER_BUFFER_SIZE = 1024;
    static const int DEFAULT_EVENT_QUEUE_SIZE = 16;

    static const int DEFAULT_MQTT_RX_BUF_SIZE = 128;
    static const int DEFAULT_MQTT_TX_BUF_SIZE = 128;

    /* SIM7000 services */

    class SIM7000_MQTT;
    class SIM7000_HTTP;
    class SIM7000_NTP;
    class SIM7000_GNSS;

    /* SIM7000 Modem */

    /**
     * @brief Modem SIM7000
     *
     */
    class SIM7000_Modem : 
        private threading::ServiceBase, 
        public SIM7000_BasicModem,
        public std::enable_shared_from_this<SIM7000_Modem>
    {
    public:
        SIM7000_Modem(
            uart_port_t port,
            int pin_rx,
            int pin_tx,
            int pin_pwr,
            int baud_rate = DEFAULT_BAUD_RATE,
            int uart_buffer_size = DEFAULT_RX_BUF_SIZE,
            int uart_queue_size = DEFAULT_EVENT_QUEUE_SIZE,
            size_t parser_buffer_size = DEFAULT_PARSER_BUFFER_SIZE
        );

        /* General */

        esp_err_t init();
        esp_err_t enable_events(esp_event_loop_handle_t *event_loop = nullptr);
        esp_err_t disable_events();
        esp_err_t set_apn(const apn_config_t &config);
        esp_err_t get_apn(apn_config_t &config);
        esp_err_t get_sim_status(sim_status_t &status);
        esp_err_t get_imei(std::string &imei);
        esp_err_t get_system_time(std::string &date_time);
        esp_err_t sync_time();

        /* Control de red */

        esp_err_t get_signal_strength(int8_t &value, signal_strength_t *strength = nullptr);
        esp_err_t get_network_reg_status(network_reg_status_t &status);
        esp_err_t get_connection_status(connection_status_t &status);
        esp_err_t get_current_operator(std::string &op_name, operator_netact_t *op_nectact = nullptr);
        esp_err_t get_local_ip(std::string &ip);
        esp_err_t activate_network();
        esp_err_t deactivate_network();
        esp_err_t enable_comm();
        esp_err_t disable_comm();

        /* Control de GRPS */

    protected:
        /* Métodos generales de inicialización */

        esp_err_t config_gprs();
        esp_err_t config_tcp_tk();
        esp_err_t config_ip_app();

        /* Métodos individuales */

        esp_err_t read_tcptk_apn(std::string &apn);
        esp_err_t set_tcptk_apn();
        esp_err_t bring_up_tcptk_conn();
        esp_err_t deact_tcptk_pdp_context();
        esp_err_t close_bearer();
        esp_err_t open_bearer();
        esp_err_t query_bearer(bearer_status_t &status, std::string *ip = nullptr);
        esp_err_t set_bearer_param(const char *param, const char *value);
        esp_err_t get_net_active_status(network_active_status_t &status, std::string *ip = nullptr);
        esp_err_t set_net_active_mode(network_active_mode_t mode, TickType_t tick_count = pdMS_TO_TICKS(1000));

    private:
        /* Constantes */

        const uart_port_t m_port;
        const gpio_num_t m_pin_pwr;

        /* Datos */

        char m_rx_buffer[DEFAULT_RX_BUF_SIZE];
        apn_config_t m_apn;
        internal::sim7000_status_t m_status;
        std::shared_ptr<internal::sim7000_cmd_result_info_t> m_result_info;
        std::recursive_mutex m_mutex;
        QueueHandle_t m_uart_event_queue;
        threading::EventGroup m_event_group;
        esp_event_loop_handle_t *m_event_loop;
        bool m_enable_events;

        esp_err_t setup() override;
        void loop() override;
        
        /* Funciones de bajo nivel */

        void receive_uart_data(size_t length);
        void on_urc_message(std::string &payload) override;
        int on_cmd_write(const char *data, size_t length) override;
        
        void post_event(int32_t id, TickType_t ticks_to_wait = portMAX_DELAY);
        
        void post_event(
            int32_t id,
            void *data, 
            size_t size, 
            TickType_t ticks_to_wait = portMAX_DELAY);

        void post_cpin_event(std::string &payload);
        void post_gnss_event(std::string &payload);
        void post_mqtt_event(std::string &payload);

        esp_err_t execute_internal_cmd_no_answer(
            internal::at_cmd_t command,
            TickType_t ticks_to_wait = 0);

        esp_err_t execute_internal_cmd_no_answer(
            internal::at_cmd_t command,
            const std::span<const char> &payload,
            TickType_t ticks_to_wait = 0);

        esp_err_t execute_internal_cmd(
            internal::at_cmd_t command,
            TickType_t ticks_to_wait = 0,
            bool is_partial = false,
            bool is_raw = false);

        esp_err_t execute_internal_cmd(
            internal::at_cmd_t command,
            const std::span<const char> &payload,
            TickType_t ticks_to_wait = 0,
            bool is_partial = false,
            bool is_raw = false);
    };

    /**
     * @brief Clase base para servicios de SIM7000.
     *
     */
    class SIM7000_Service
    {
    public:
        SIM7000_Service(std::weak_ptr<SIM7000_Modem> modem);
        virtual esp_err_t init() { return ESP_OK; }
        virtual esp_err_t deinit() { return ESP_OK; }
    protected:
        esp_err_t check_modem_ptr();

        std::weak_ptr<SIM7000_Modem> m_modem;
        std::shared_ptr<internal::sim7000_cmd_result_info_t> m_result_info;
    };

} // namespace axomotor::lte_modem

#include "sim7000_modem.hpp"
#include "sim7000_types.hpp"
#include "sim7000_helpers.hpp"

#include <cstring>
#include <string>
#include <array>

#include <esp_log.h>
#include <driver/uart.h>
#include <driver/gpio.h>

#define PDP_DEACT_BIT                   BIT0
#define APP_PDP_ACTIVE_BIT              BIT1
#define APP_PDP_DEACTIVE_BIT            BIT2

#define NULL_CH     '\0'
#define CRLF        "\r\n"
#define CR          '\r'
#define LF          '\n'

namespace axomotor::lte_modem {

ESP_EVENT_DEFINE_BASE(MODEM_EVENTS);

using namespace axomotor::lte_modem::internal;
using Parser = SIM7000_BasicModem;

static const char *TAG = "sim7000:modem";

/* SIM7000 Modem */

SIM7000_Modem::SIM7000_Modem(
    uart_port_t port,
    int pin_rx,
    int pin_tx,
    int pin_pwr,
    int baud_rate,
    int uart_buffer_size,
    int uart_queue_size,
    size_t parser_buffer_size
) :
    ServiceBase{TAG, 4 * 1024, 20},
    SIM7000_BasicModem(parser_buffer_size),
    m_port{port},
    m_pin_pwr{(gpio_num_t)pin_pwr},
    m_apn{},
    m_status{},
    m_uart_event_queue{nullptr},
    m_event_loop{nullptr},
    m_enable_events{false}
{
    // crea la instancia compartida de resultado de comando
    m_result_info = std::make_shared<internal::sim7000_cmd_result_info_t>();
    m_result_info->response.reserve(DEFAULT_RESPONSE_BUF_SIZE);
    
    // configuración del puerto UART
    uart_config_t uart_config{};
    uart_config.baud_rate = baud_rate;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    // configura el pin de control (PWR)
    gpio_reset_pin(m_pin_pwr);
    gpio_set_direction(m_pin_pwr, GPIO_MODE_OUTPUT);

    // instalación de controlador UART
    ESP_ERROR_CHECK(uart_driver_install(
        m_port,
        uart_buffer_size,
        0,
        uart_queue_size,
        &m_uart_event_queue,
        0
    ));

    // configuración del puerto UART
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

esp_err_t SIM7000_Modem::init()
{
    if (m_is_running) return ESP_ERR_INVALID_STATE;

    std::lock_guard lock(m_mutex);
    esp_err_t err;
    at_cmd_t cmd;
    std::string &res = m_result_info->response;

    ESP_LOGI(TAG, "Waiting for module starting...");
    
    // inicia el servicio subyacente y espera hasta que este termine de configurarse
    err = start(true);
    if (err != ESP_OK) {
        return err;
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Module has started");
        // desactiva el eco
        cmd = at_cmd_t::E;
        err = execute_internal_cmd_no_answer(cmd, "0", pdMS_TO_TICKS(1000));
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Echo is disabled");
        // lee el nombre del dispositivo
        cmd = at_cmd_t::CGMM;
        err = execute_internal_cmd(cmd);
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Module name: %s", res.c_str());
        // lee la versión de Firmware
        cmd = at_cmd_t::CGMR;
        err = execute_internal_cmd(cmd);
    }
    if (err == ESP_OK) {
        helpers::remove_before(res, ":");
        ESP_LOGI(TAG, "Firmware: %s", res.c_str());
        // lee el IMEI
        cmd = at_cmd_t::CGSN;
        err = execute_internal_cmd(cmd);
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "IMEI: %s", res.c_str());
        // lee la hora del sistema
        cmd = at_cmd_t::CCLK;
        err = execute_internal_cmd(cmd, "?");
    }
    if (err == ESP_OK) {
        helpers::extract_token(res, 1, "\"");
        ESP_LOGI(TAG, "Current time is %s", res.c_str());
        // lee el estado de la tarjeta SIM
        cmd = at_cmd_t::CPIN;
        err = execute_internal_cmd(cmd, "?");
    }
    if (err == ESP_OK) {
        helpers::remove_before(res, ": ");
        ESP_LOGI(TAG, "SIM status: %s", res.c_str());
    } else {
        ESP_LOGE(
            TAG,
            "Failed to execute %s (%s)",
            get_command_def(cmd)->string,
            esp_err_to_name(err)
        );
    }

    return err;
}

esp_err_t SIM7000_Modem::enable_events(esp_event_loop_handle_t *event_loop)
{
    if (m_enable_events) return ESP_ERR_INVALID_STATE;

    esp_err_t err;

    // verifica si se recibió el puntero de un loop de eventos
    if (event_loop) {
        esp_event_loop_args_t args{};
        args.queue_size = 10;
        args.task_name = "sim7000_evt_task";
        args.task_priority = 5;
        args.task_stack_size = 1024 * 3;
        args.task_core_id = tskNO_AFFINITY;

        err = esp_event_loop_create(&args, event_loop);
        m_event_loop = event_loop;
    } else {
        // crear el loop predeterminado de eventos 
        err = esp_event_loop_create_default();
        // verifica si el loop ya había sido creado
        if (err == ESP_ERR_INVALID_STATE) {
            err = ESP_OK;
        }
    }

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG, 
            "Failed to create event loop (%s)",
            esp_err_to_name(err)
        );
    }
    
    m_enable_events = err == ESP_OK;
    return err;
}

esp_err_t SIM7000_Modem::disable_events()
{
    if (!m_enable_events) return ESP_ERR_INVALID_STATE;

    esp_err_t err = esp_event_loop_delete(m_event_loop);
    m_event_loop = nullptr;
    m_enable_events = false;

    return err;
}

esp_err_t SIM7000_Modem::set_apn(const apn_config_t &config)
{
    // espera hasta que el modulo esté disponible
    std::lock_guard lock(m_mutex);
    esp_err_t err = ESP_ERR_INVALID_STATE;
    // copia la configuración
    memcpy(&m_apn, &config, sizeof(apn_config_t));

    if (m_is_running) {
        // verifica si GRPS está activo
        if (m_status.is_grps_active) {
            err = config_gprs();
        }
        if (err == ESP_OK && m_status.is_tcp_active) {
            err = config_tcp_tk();
        }
        if (err == ESP_OK && m_status.is_ip_active) {
            err = config_ip_app();
        }
    }

    return err;
}

esp_err_t SIM7000_Modem::get_sim_status(sim_status_t &status)
{
    // espera hasta que el modulo esté disponible
    std::lock_guard lock(m_mutex);
    esp_err_t err;
    std::string &res = m_result_info->response;

    err = execute_internal_cmd(at_cmd_t::CPIN, "?");

    if (err == ESP_OK) {
        helpers::remove_before(res, ": ");

        if (res == "READY") {
            status = sim_status_t::READY;
        } else if (res == "SIM PIN") {
            status = sim_status_t::PIN_REQUIRED;
        } else if (res == "SIM PUK") {
            status = sim_status_t::PUK_REQUIRED;
        } else {
            status = sim_status_t::PHONE_REQUIRED;
        }

        ESP_LOGI(TAG, "SIM status: %s", res.c_str());
    }

    return err;
}

esp_err_t SIM7000_Modem::get_imei(std::string &imei)
{
    // espera hasta que el modulo esté disponible
    std::lock_guard lock(m_mutex);
    esp_err_t err;
    std::string &res = m_result_info->response;

    err = execute_internal_cmd(at_cmd_t::CGSN);
    if (err == ESP_OK) {
        imei.assign(res);
        ESP_LOGI(TAG, "IMEI: %s", imei.c_str());
    }

    return err;
}

esp_err_t SIM7000_Modem::get_system_time(std::string &date_time)
{
    // espera hasta que el modulo esté disponible
    std::lock_guard lock(m_mutex);
    esp_err_t err;
    std::string &res = m_result_info->response;

    // espera hasta que el modulo esté disponible
    err = execute_internal_cmd(at_cmd_t::CCLK, "?");
    if (err == ESP_OK) {
        helpers::extract_token(res, 1, "\"");
        date_time.assign(res);

        ESP_LOGI(TAG, "Current time is %s", date_time.c_str());
    }

    return err;
}

esp_err_t SIM7000_Modem::sync_time()
{
    std::string date_time;
    esp_err_t err = get_system_time(date_time);

    if (err == ESP_OK) {
        time_t epoch = helpers::parse_to_epoch(date_time);
        if (epoch != 0) {
            struct timeval val{};
            val.tv_sec = epoch;

            // establece la hora del sistema
            if (settimeofday(&val, NULL) == 0) {
                epoch = 0;
                time(&epoch);
                
                ESP_LOGI(
                    TAG, 
                    "The system time is set | current timestamp: %lld",
                    epoch
                );
            } else {
                ESP_LOGW(TAG, "Failed to set system time (%d)", errno);
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGW(TAG, "Unrecognized datetime format");
            err = ESP_FAIL;
        }
    }

    return err;
}

esp_err_t SIM7000_Modem::get_signal_strength(int8_t &value, signal_strength_t *strength)
{
    // espera hasta que el modulo esté disponible
    std::lock_guard lock(m_mutex);
    esp_err_t result;
    uint8_t rssi;
    std::string &response = m_result_info->response;

    // espera hasta que el modulo esté disponible;
    result = execute_internal_cmd(at_cmd_t::CSQ);
    if (result == ESP_OK) {
        helpers::remove_before(response, ": ");
        rssi = helpers::to_number<uint8_t>(response);

        if (rssi == 0) {
            value = -115;
        } else if (rssi == 1) {
            value = -111;
        } else if (rssi >= 2 && rssi <= 30) {
            float factor = (rssi - 2) / 28.0f;
            float delta = (110 - 54) * factor;
            value = -110 + (int8_t)(delta);
        } else if (rssi == 31) {
            value = -52;
        } else {
            value = 0;
        }

        if (strength != nullptr) {
            if (value == 0) {
                *strength = signal_strength_t::NOT_DETECTABLE;
            } else if (value > -60) {
                *strength = signal_strength_t::EXCELLENT;
            } else if (value > -85) {
                *strength = signal_strength_t::GOOD;
            } else {
                *strength = signal_strength_t::MARGINAL;
            }
        }

        ESP_LOGI(TAG, "RSSI: %u (%d dBm)", rssi, value);
    }

    return result;
}

esp_err_t SIM7000_Modem::get_network_reg_status(network_reg_status_t &status)
{
    // espera hasta que el modulo esté disponible
    std::lock_guard lock(m_mutex);
    esp_err_t result;
    uint8_t value;
    std::string &response = m_result_info->response;

    // espera hasta que el modulo esté disponible
    result = execute_internal_cmd(at_cmd_t::CGREG, "?");
    if (result == ESP_OK) {
        helpers::remove_before(response, ",");
        value = helpers::to_number<uint8_t>(response);

        switch (value)
        {
            case 0:
                status = network_reg_status_t::NOT_REGISTERED;
                ESP_LOGI(TAG, "Module is not registered in network");
                break;
            case 1:
                status = network_reg_status_t::REGISTERED;
                ESP_LOGI(TAG, "Module is registered in network");
                break;
            case 2:
                status = network_reg_status_t::TRYING_TO_REGISTER;
                ESP_LOGI(TAG, "Module is trying to register in network");
                break;
            case 3:
                status = network_reg_status_t::REGISTRATION_DENIED;
                ESP_LOGI(TAG, "Module registration in network was denied");
                break;
            case 5:
                status = network_reg_status_t::REGISTERED_ROAMING;
                ESP_LOGI(TAG, "Module is registered in network (roaming)");
                break;
            default:
                ESP_LOGI(TAG, "Module is in unknown resgistration state");
                status = network_reg_status_t::UNKNOWN;
                break;
        }
    }

    return result;
}

esp_err_t SIM7000_Modem::get_connection_status(connection_status_t &status)
{
    std::lock_guard lock(m_mutex);
    esp_err_t result;
    std::string &response = m_result_info->response;

    // obtiene el estado de conexión
    result = execute_internal_cmd(at_cmd_t::CIPSTATUS, 0, false, true);
    if (result == ESP_OK) {
        //ESP_LOG_BUFFER_HEX(TAG, response.c_str(), response.length());

        // verifica si la respuesta comienza con OK
        bool is_valid = response.starts_with(CRLF "OK");
        if (is_valid) {
            // extrae el estado de la conexión
            helpers::remove_before(response, ": ");
            helpers::rtrim(response);
        
            ESP_LOGI(TAG, "Connection status: %s", response.c_str());

            if (response == "IP INITIAL") {
                status = connection_status_t::IP_INITIAL;
            } else if (response == "IP START") {
                status = connection_status_t::IP_START;
            } else if (response == "IP CONFIG") {
                status = connection_status_t::IP_CONFIG;
            } else if (response == "IP GPRSACT") {
                status = connection_status_t::IP_GPRSACT;
            } else if (response == "IP STATUS") {
                status = connection_status_t::IP_STATUS;
            } else if (response.contains("CONNECTING")) {
                status = connection_status_t::CONNECTING;
            } else if (response == "CONNECT OK") {
                status = connection_status_t::CONNECT_OK;
            } else if (response == "CLOSING") {
                status = connection_status_t::CLOSING;
            } else if (response == "CLOSED") {
                status = connection_status_t::CLOSED;
            } else if (response == "PDP DEACT") {
                status = connection_status_t::PDP_DEACT;
            } 
        } else {
            status = connection_status_t::UNKNOWN;
            result = ESP_ERR_INVALID_RESPONSE;
        }
    }

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read connection status");
    }

    return result;
}

esp_err_t SIM7000_Modem::get_current_operator(std::string &op_name, operator_netact_t *nectact)
{
    // espera hasta que el modulo esté disponible
    std::lock_guard lock(m_mutex);
    esp_err_t result;
    std::string &response = m_result_info->response;

    // espera hasta que el modulo esté disponible
    result = execute_internal_cmd(at_cmd_t::COPS, "?");
    if (result == ESP_OK) {
        // remueve el comando de la cadena
        helpers::remove_before(response, ": ");
        // extrae el nombre del operador y quita comillas (si las hay)
        helpers::extract_token(response, 2, ",", op_name, true);
        helpers::trim(op_name);
        // extrae el indicador tipo de conexión
        helpers::extract_token(response, 3, ",", true);
        
        if (nectact != nullptr) {
            if (response == "0") {
                *nectact = operator_netact_t::USER_SPECIFIED;
            } else if (response == "1") {
                *nectact = operator_netact_t::GSM_COMPACT;
            } else if (response == "3") {
                *nectact = operator_netact_t::GSM_EGPRS;
            } else if (response == "7") {
                *nectact = operator_netact_t::LTE_M1_A_GB;
            } else if (response == "9") {
                *nectact = operator_netact_t::LTE_NB_S1;
            } else {
                *nectact = operator_netact_t::UNKNOWN;
            }
        }

        if (op_name.empty()) {
            op_name = "Unknown";
        }

        ESP_LOGI(TAG, "Current operator: %s", op_name.c_str());
    }

    return result;
}

esp_err_t SIM7000_Modem::get_local_ip(std::string &ip)
{
    std::lock_guard lock(m_mutex);
    esp_err_t result;
    std::string &response = m_result_info->response;

    // obtiene el APN actual
    result = execute_internal_cmd(at_cmd_t::CIFSR, 0, false, true);
    if (result == ESP_OK) {
        helpers::trim(response);
        if (response != "ERROR") {
            ip.assign(response);
            ESP_LOGI(TAG, "Current IP: %s", ip.c_str());
        } else {
            result = ESP_FAIL;
        }
    } 
    
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get local IP");
    }

    return result;
}

esp_err_t SIM7000_Modem::activate_network()
{
    std::lock_guard lock(m_mutex);
    network_active_status_t status;
    ESP_LOGI(TAG, "Activating network");

    esp_err_t result = get_net_active_status(status);

    if (result == ESP_OK && status == network_active_status_t::DEACTIVED) {
        result = set_net_active_mode(network_active_mode_t::ACTIVE);
    }

    return result;
}

esp_err_t SIM7000_Modem::deactivate_network()
{
    std::lock_guard lock(m_mutex);
    network_active_status_t status;
    esp_err_t result = get_net_active_status(status);

    if (result == ESP_OK && status != network_active_status_t::DEACTIVED) {
        result = set_net_active_mode(network_active_mode_t::DEACTIVE);
    }

    return result;
}

esp_err_t SIM7000_Modem::enable_comm()
{
    ESP_LOGI(TAG, "Configuring GRPS...");
    esp_err_t err = config_gprs();

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Configuring TCP APP Toolkit...");
        err = config_tcp_tk();
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Configuring IP...");
        err = config_ip_app();
    }

    return err;
}

/* Métodos generales de inicialización */

esp_err_t SIM7000_Modem::config_gprs()
{
    std::lock_guard lock(m_mutex);
    std::string &response = m_result_info->response;
    std::string payload;
    std::string cid = std::to_string(m_apn.cid);
    std::string cid_str = ": " + cid + ",";
    esp_err_t result;
    size_t index, len;
    uint8_t value;

    // valida valor de CID
    if (m_apn.cid == 0 || m_apn.cid > 24) {
        ESP_LOGW(TAG, "Invalid CID value, setting to 1...");
        m_apn.cid = 1;
    }

    // obtiene los estados de activación de contexto PDP
    result = execute_internal_cmd(at_cmd_t::CGACT, "?");
    if (result == ESP_OK) {

        index = response.find_first_of(cid_str) + cid_str.length();
        value = helpers::to_number<uint8_t>(response, index, 1);

        if (value == 1) {
            ESP_LOGI(TAG, "PDP Context is active");
        } else {
            ESP_LOGW(TAG, "PDP Context is not active");
        }

        // obtiene la dirección del PDP
        result = execute_internal_cmd(at_cmd_t::CGPADDR, "=" + cid);
    }

    if (result == ESP_OK) {
        index = response.find_first_of(cid_str) + cid_str.length();
        payload = response.substr(index);

        ESP_LOGI(TAG, "PDP address: %s", payload.c_str());
        // obtiene la definición de contexto PDP
        result = execute_internal_cmd(at_cmd_t::CGDCONT, "?", 0);
    }

    if (result == ESP_OK) {
        index = response.find_first_of(cid_str) + cid_str.length();
        len = response.find_first_of(CR, index) - index;
        payload = response.substr(index, len);

        ESP_LOGI(TAG, "PDP context: %s", payload.c_str());

        if (payload.find_first_of(m_apn.apn) == std::string::npos) {
            ESP_LOGI(TAG, "APN is not set in PDP context, setting it...");
            payload = "=" + cid + ",\"" + m_apn.apn + "\"";
            result = execute_internal_cmd(at_cmd_t::CGDCONT, payload);

            if (result == ESP_OK) {
                ESP_LOGI(TAG, "APN setting success");
            } else {
                ESP_LOGW(TAG, "APN setting fail");
            }
        }

        // verifica el APN provisto por la red
        result = execute_internal_cmd(at_cmd_t::CGNAPN);
    }

    if (result == ESP_OK) {
        ESP_LOGI(TAG, "%s", response.c_str());
        index = response.find_first_of(":") + 2;
        value = helpers::to_number<uint8_t>(response, index, 1);

        if (value == 1) {
            index = response.find_first_of('\"') + 1;
            len = response.find_last_of('\"') - index;
            payload = response.substr(index, len);

            ESP_LOGI(TAG, "Network APN: %s", payload.c_str());
        } else {
            ESP_LOGW(TAG, "Network did not send APN parameter");
            result = ESP_FAIL;
        }
    }

    return result;
}

esp_err_t SIM7000_Modem::config_tcp_tk()
{
    std::lock_guard lock(m_mutex);
    std::string params;
    esp_err_t err;
    connection_status_t conn_status;

    // obtiene el estado de conexión
    err = get_connection_status(conn_status);
    if (err == ESP_OK) {
        // lee el APN actual
        err = read_tcptk_apn(params);
    }

    if (err == ESP_OK) {
        // verifica si el APN actualmente establecido es diferente al 
        // provisto por la aplicación
        if (params != m_apn.apn || 
            conn_status == connection_status_t::PDP_DEACT || 
            conn_status == connection_status_t::IP_INITIAL
        ) {
            ESP_LOGI(TAG, "TCP APP Toolkit APN is not set");

            // verifica si el estado de conexión no está inicializado
            if (conn_status != connection_status_t::IP_INITIAL) {
                // desactiva GPRS
                err = deact_tcptk_pdp_context();
            }

            if (err == ESP_OK) {
                // establece el TCP provisto
                err = set_tcptk_apn();
            }
        } 
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "TCP APP Toolkit APN is set to: %s", m_apn.apn);
        }

        // obtiene nuevamente el estado de conexión
        err = get_connection_status(conn_status);
    }

    if (err == ESP_OK) {
        //  el estado de la conexión debe ser IP start
        if (conn_status == connection_status_t::IP_START) {
            err = bring_up_tcptk_conn();
        } else if (conn_status == connection_status_t::IP_STATUS) {
            ESP_LOGI(TAG, "Wireless Connection is already active");
        }
    }

    if (err == ESP_OK) {
        err = get_local_ip(params);
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "TCP APP Toolkit activation success | Local IP: %s", params.c_str());
    } else {
        ESP_LOGE(TAG, "Failed to activate GPRS (%s)", esp_err_to_name(err));
    }

    return err;
}

esp_err_t SIM7000_Modem::config_ip_app()
{
    std::lock_guard lock(m_mutex);
    std::string ip;
    esp_err_t err;
    bearer_status_t bearer_status;

    // obtiene el estado del portador
    err = query_bearer(bearer_status, &ip);
    if (err == ESP_OK) {
        // verifica si el portador no está conectado
        if (bearer_status != bearer_status_t::CONNECTED) {
            ESP_LOGI(TAG, "Bearer is not connected, setting parameters...");

            if (strlen(m_apn.apn) > 0) {
                err = set_bearer_param("APN", m_apn.apn);
            }
            if (err == ESP_OK && strlen(m_apn.user) > 0) {
                err = set_bearer_param("USER", m_apn.user);
            }
            if (err == ESP_OK && strlen(m_apn.user) > 0) {
                err = set_bearer_param("PWD", m_apn.user);
            }
            if (err == ESP_OK) {
                err = open_bearer();
            }
            if (err == ESP_OK) {
                err = query_bearer(bearer_status, &ip);
            }
        } else {
            ESP_LOGI(TAG, "Bearer is already open");
        }
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Bearer IP address: %s", ip.c_str());
    } else {
        ESP_LOGE(TAG, "Failed to configure bearer");
    }

    return ESP_OK;
}

/* Métodos individuales */

esp_err_t SIM7000_Modem::read_tcptk_apn(std::string &apn)
{
    std::lock_guard lock(m_mutex);
    esp_err_t err;
    std::string &response = m_result_info->response;

    ESP_LOGI(TAG, "Reading TCP APN...");
    err = execute_internal_cmd(at_cmd_t::CSTT, "?");
    if (err == ESP_OK) {
        helpers::remove_before(response, ": ");
        helpers::extract_token(response, 0, ",", apn, true);
        helpers::trim(apn);
    } 

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Read TCP APN succeed");
    } else {
        ESP_LOGE(TAG, "Failed to read TCP APN");
    }

    return err;
}

esp_err_t SIM7000_Modem::set_tcptk_apn()
{
    std::lock_guard lock(m_mutex);
    esp_err_t err;
    std::string params = "=\"";
    params.append(m_apn.apn);
    params.append("\"");
    
    ESP_LOGI(TAG, "Setting TCP APN...");
    err = execute_internal_cmd_no_answer(at_cmd_t::CSTT, params);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Set TCP APN succeed");
    } else {
        ESP_LOGE(TAG, "Failed to set TCP APN");
    }

    return err;
}

esp_err_t SIM7000_Modem::bring_up_tcptk_conn()
{
    std::lock_guard lock(m_mutex);
    esp_err_t err;

    ESP_LOGI(TAG, "Bringing up Wireless Connection...");
    err = execute_internal_cmd_no_answer(at_cmd_t::CIICR);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Bring up Wireless Connection with GPRS succeed");
    } else {
        ESP_LOGE(TAG, "Failed to activate GPRS");
    }

    return err;
}

esp_err_t SIM7000_Modem::deact_tcptk_pdp_context()
{
    std::lock_guard lock(m_mutex);
    esp_err_t result;

    ESP_LOGI(TAG, "Deactivating TCP PDP Context...");
    result = execute_internal_cmd_no_answer(at_cmd_t::CIPSHUT);
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Deactivate TCP PDP Context succeed");
    } else {
        ESP_LOGE(TAG, "Failed to deactivate TCP PDP Context");
    }

    return result;
}

esp_err_t SIM7000_Modem::close_bearer()
{
    std::lock_guard lock(m_mutex);
    esp_err_t err;
    std::string payload = "=0,";
    payload.append(std::to_string(m_apn.cid));

    ESP_LOGI(TAG, "Closing bearer...");
    err = execute_internal_cmd_no_answer(at_cmd_t::SAPBR, payload);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to close bearer");
    }

    return err;
}

esp_err_t SIM7000_Modem::open_bearer()
{
    std::lock_guard lock(m_mutex);
    esp_err_t err;
    std::string payload = "=1,";
    payload.append(std::to_string(m_apn.cid));

    ESP_LOGI(TAG, "Opening bearer...");
    err = execute_internal_cmd_no_answer(at_cmd_t::SAPBR, payload);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open bearer");
    }

    return err;
}

esp_err_t SIM7000_Modem::query_bearer(bearer_status_t &status, std::string *ip)
{
    std::lock_guard lock(m_mutex);
    esp_err_t err;
    std::string &response = m_result_info->response;
    std::string payload = "=2,";
    payload.append(std::to_string(m_apn.cid));
    
    ESP_LOGI(TAG, "Querying bearer...");
    err = execute_internal_cmd(at_cmd_t::SAPBR, payload);
    if (err == ESP_OK) {
        helpers::remove_before(response, ": ");
        // verifica y extrae la IP del portador (si se requiere)
        if (ip != nullptr) {
            helpers::extract_token(response, 2, ",", *ip, true);
            helpers::trim(*ip);
        }
        
        // extrae el estado del portador
        helpers::extract_token(response, 1, ",", true);

        if (response == "0") {
            status = bearer_status_t::CONNECTING;
        } else if (response == "1") {
            status = bearer_status_t::CONNECTED;
        } else if (response == "2") {
            status = bearer_status_t::CLOSING;
        } else if (response == "3") {
            status = bearer_status_t::CLOSED;
        } else {
            status = bearer_status_t::UNKNOWN;
        }
    } else {
        ESP_LOGE(TAG, "Failed to query bearer");
    }

    return err;
}

esp_err_t SIM7000_Modem::set_bearer_param(const char *param_name, const char *param_value)
{
    if (!param_name || !param_value) return ESP_ERR_INVALID_ARG;

    esp_err_t err;
    std::lock_guard lock(m_mutex);
    std::string cmd_params = "=3," + std::to_string(m_apn.cid);
    cmd_params.append(",\"");
    cmd_params.append(param_name);
    cmd_params.append("\",\"");
    cmd_params.append(param_value);
    cmd_params.append("\"");

    ESP_LOGI(TAG, "Setting bearer parameter...");

    err = execute_internal_cmd_no_answer(at_cmd_t::SAPBR, cmd_params);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set bearer parameter '%s'", param_name);
    }

    return err;
}

esp_err_t SIM7000_Modem::get_net_active_status(network_active_status_t &status, std::string *ip)
{
    std::lock_guard lock(m_mutex);
    std::string &response = m_result_info->response;
    esp_err_t err;

    ESP_LOGI(TAG, "Getting network active status...");
    err = execute_internal_cmd(at_cmd_t::CNACT, "?");
    if (err == ESP_OK) {
        helpers::remove_before(response, ": ");
        // verifica y extrae la dirección ip
        if (ip != nullptr) {
            helpers::extract_token(response, 1, ",", *ip);
            helpers::trim(*ip);
        }

        // extrae el estado de la red
        helpers::remove_after(response, ",");
        if (response == "0") {
            status = network_active_status_t::DEACTIVED;
        } else if (response == "1") {
            status = network_active_status_t::ACTIVED;
        } else if (response == "2") {
            status = network_active_status_t::IN_OPERATION;
        } else {
            status = network_active_status_t::UNKNOWN;
        }
    } 
    
    if (err != ESP_OK || status == network_active_status_t::UNKNOWN) {
        ESP_LOGE(TAG, "Failed to get network active status");
    }

    return err;
}

esp_err_t SIM7000_Modem::set_net_active_mode(network_active_mode_t mode, TickType_t tick_count)
{
    std::lock_guard lock(m_mutex);
    std::string cmd_params;
    esp_err_t err;
    uint32_t flag = 0;

    ESP_LOGI(TAG, "Setting network active mode...");

    switch (mode)
    {
        case network_active_mode_t::ACTIVE:
            cmd_params = "=1";
            flag = APP_PDP_ACTIVE_BIT;
            break;
        case network_active_mode_t::DEACTIVE:
            cmd_params = "=0";
            flag = APP_PDP_DEACTIVE_BIT;
            break;
        case network_active_mode_t::AUTO_ACTIVE:
            cmd_params = "=2";
            break;
        default:
            ESP_LOGE(TAG, "Invalid network active mode");
            return ESP_ERR_INVALID_ARG;
    }

    err = execute_internal_cmd_no_answer(at_cmd_t::CNACT, cmd_params);

    if (err == ESP_OK && flag != 0) {
        // esperar confirmación de activación
        if (m_event_group.wait_for_flags(flag, false, true, tick_count) == 0) {
            ESP_LOGW(TAG, "Network active mode confirmation timed out");
        }
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set network active mode");
    }

    return err;
}

/* Funciones privadas */

esp_err_t SIM7000_Modem::setup()
{
    // tiempo de espara de 7.5 segundos para recibir una respuesta
    const TickType_t timeout_ms = pdMS_TO_TICKS(7500);
    // intervalo de muestreo
    const TickType_t poll_interval = pdMS_TO_TICKS(100); // ms
    // envía el comando AT\r para verifica si el módulo está activo
    const char *at_cmd = "AT\r";
    esp_err_t result = ESP_ERR_TIMEOUT;
    int elapsed = 0;
    int bytes_read = 0;
    uart_event_t event;
    std::string response;

    // enciende el módulo
    //gpio_set_level(m_pin_pwr, 1);
    // espera para estabilizar
    vTaskDelay(pdMS_TO_TICKS(100));

    // limpia el buffer UART antes de enviar el comando
    uart_flush(m_port);

    // bucle que se ejecuta hasta que haya transcurrido el tiempo de espera máximo
    while (elapsed < timeout_ms) {
        uart_write_bytes(m_port, at_cmd, strlen(at_cmd));

        // espera hasta que se produzca un evento de datos recibidos
        if (xQueueReceive(m_uart_event_queue, &event, poll_interval) && event.type == UART_DATA) {
            // lee los datos recibidos
            bytes_read = uart_read_bytes(m_port, m_rx_buffer, sizeof(m_rx_buffer) - 1, poll_interval);
            // comprueba si la cantidad de bytes leídos corresponde con la esperada
            if (bytes_read == event.size) {
                m_rx_buffer[bytes_read] = '\0';
                // toma la porción de bytes leidos para crear una cadena
                response = std::string(m_rx_buffer, bytes_read);

                // busca "OK" en la respuesta
                if (response.find("OK") != std::string::npos) {
                    result = ESP_OK;
                    break;
                }
            }
        }

        // incrementa el tiempo transcurrido
        elapsed += poll_interval;
    }

    // limpia el buffer UART
    uart_flush(m_port);
    // reestablece la cola de eventos
    xQueueReset(m_uart_event_queue);

    if (result == ESP_OK) {
        m_is_running = true;
        ESP_LOGI(TAG, "SIM7000 module ready");
    } else {
        ESP_LOGE(TAG, "Failed to initialize SIM7000 module");
    }

    return result;
}

void SIM7000_Modem::loop()
{
    uart_event_t event;

    // espera hasta recibir un evento
    if (xQueueReceive(m_uart_event_queue, &event, portMAX_DELAY)) {
        switch (event.type) 
        {
            case UART_DATA:
                if (event.size != 0) {
                    ESP_LOGD(TAG, "Received data from UART (%u bytes)", event.size);
                    receive_uart_data(event.size);
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
}

void SIM7000_Modem::receive_uart_data(size_t length)
{
    if (uart_read_bytes(m_port, m_rx_buffer, length, 0) != length) {
        ESP_LOGE(TAG, "Failed to read data");
        return;
    }
    
    feed_buffer(m_rx_buffer, length);
}

void SIM7000_Modem::on_urc_message(std::string &payload)
{
    helpers::trim(payload, CRLF " ");
    if (payload.empty()) return;

    ESP_LOGI(TAG, "URC message received (%u bytes)", payload.length());
    ESP_LOGI(TAG, "URC: %s", payload.c_str());

    auto def = get_urc_def(payload);
    if (def == nullptr) {
        return;
    }
    
    switch (def->urc)
    {
        case urc_t::DST:
        {
            ESP_LOGI(TAG, "Refresh time zone by network");
            post_event(MODEM_EVENT_DST_UPDATED);
            break;
        }
        case urc_t::PSUTTZ:
            ESP_LOGI(TAG, "Refresh date and time by network");
            post_event(MODEM_EVENT_DATE_TIME_UPDATED);
            break;
        case urc_t::PDP_DEACT:
            ESP_LOGI(TAG, "GRPS is disconnected by network");
            m_event_group.set_flags(PDP_DEACT_BIT);
            post_event(MODEM_EVENT_PDP_DEACTIVE);
            break;
        case urc_t::APP_PDP_ACTIVE:
            ESP_LOGI(TAG, "TCP APP Tookit is now active");
            m_event_group.set_flags(APP_PDP_ACTIVE_BIT);
            m_event_group.clear_flags(APP_PDP_DEACTIVE_BIT);
            post_event(MODEM_EVENT_APP_PDP_ACTIVE);
            break;
        case urc_t::APP_PDP_DEACTIVE:
            ESP_LOGI(TAG, "TCP APP Tookit is now active");
            m_event_group.set_flags(APP_PDP_DEACTIVE_BIT);
            m_event_group.clear_flags(APP_PDP_ACTIVE_BIT);
            post_event(MODEM_EVENT_APP_PDP_DEACTIVE);
            break;
        case urc_t::CFUN:
            ESP_LOGI(TAG, "Phone functionality has changed");
            post_event(MODEM_EVENT_FUNC_CHANGED);
            break;
        case urc_t::CPIN:
            ESP_LOGI(TAG, "SIM Card status has changed");
            post_cpin_event(payload);
            break;
        case urc_t::SMSUB:
            ESP_LOGI(TAG, "MQTT message received");
            post_mqtt_event(payload);
            break;
        case urc_t::UGNSINF:
            ESP_LOGI(TAG, "GNSS Navigation Report");
            post_gnss_event(payload);
            break;
        default:
            break;
    }
}

int SIM7000_Modem::on_cmd_write(const char *data, size_t length)
{
    return uart_write_bytes(m_port, data, length); 
}

void SIM7000_Modem::post_event(int32_t id, TickType_t ticks_to_wait)
{
    post_event(id, NULL, 0, ticks_to_wait);
}

void SIM7000_Modem::post_event(int32_t id, void *data, size_t size, TickType_t ticks_to_wait)
{
    if (!m_enable_events) return;
    esp_err_t err;

    if (m_event_loop) {
        err = esp_event_post_to(
            m_event_loop, 
            MODEM_EVENTS,
            id,
            data,
            size,
            ticks_to_wait
        );
    } else {
        err = esp_event_post(
            MODEM_EVENTS,
            id,
            data,
            size,
            ticks_to_wait
        );
    }

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG, 
            "Failed to post event (%s)",
            esp_err_to_name(err)
        );
    }
}

void SIM7000_Modem::post_cpin_event(std::string &payload)
{
    
}

void SIM7000_Modem::post_gnss_event(std::string &payload)
{
    gnss_nav_info_t info{};
    helpers::parse_gnss_info(payload, info);
    
    post_event(
        MODEM_EVENT_GNSS_NAVIGATION_REPORT, 
        &info,
        sizeof(gnss_nav_info_t)
    );
}

void SIM7000_Modem::post_mqtt_event(std::string &payload)
{
    mqtt_message_t message{};
    helpers::remove_before(payload, ": \"");

    // obtiene la longitud del topico
    size_t pos, length = payload.find_first_of("\",\"");
    pos = length;
    
    if (length == std::string::npos) return;
    if (length >= sizeof(message.topic)) {
        ESP_LOGW(TAG, "Topic length is longer than expected (%u bytes)", length);
        length = sizeof(message.topic) - 1;
    }

    // copia el tópico
    payload.copy(message.topic, length, 0);

    pos += 3; // desplaza despues del separador
    length = payload.length() - pos - 1;

    if (length >= sizeof(message.content)) {
        ESP_LOGW(TAG, "Content length is longer than expected (%u bytes)", length);
    }

    // copia el contenido
    payload.copy(message.content, length, pos);

    post_event(
        MODEM_EVENT_MQTT_MESSAGE_RECEIVED, 
        &message, 
        sizeof(message),
        portMAX_DELAY
    );
}

esp_err_t SIM7000_Modem::execute_internal_cmd_no_answer(
    internal::at_cmd_t command,
    TickType_t ticks_to_wait)
{
    return execute_cmd(
        command,
        m_result_info,
        ticks_to_wait,
        true
    );
}

esp_err_t SIM7000_Modem::execute_internal_cmd_no_answer(
    internal::at_cmd_t command,
    const std::span<const char> &payload,
    TickType_t ticks_to_wait)
{
    return execute_cmd(
        command,
        payload,
        m_result_info,
        ticks_to_wait,
        true
    );
}

esp_err_t SIM7000_Modem::execute_internal_cmd(
    internal::at_cmd_t command,
    TickType_t ticks_to_wait,
    bool is_partial,
    bool is_raw)
{
    return execute_cmd(
        command,
        m_result_info,
        ticks_to_wait,
        false,
        is_partial,
        is_raw
    );
}

esp_err_t SIM7000_Modem::execute_internal_cmd(
    internal::at_cmd_t command,
    const std::span<const char> &payload,
    TickType_t ticks_to_wait,
    bool is_partial,
    bool is_raw)
{
    return this->execute_cmd(
        command,
        payload,
        m_result_info,
        ticks_to_wait,
        false,
        is_partial,
        is_raw
    );   
}

/* SIM7000_Service */

SIM7000_Service::SIM7000_Service(std::weak_ptr<SIM7000_Modem> modem) : m_modem(modem)
{ 
    m_result_info = std::make_shared<internal::sim7000_cmd_result_info_t>();
}

esp_err_t SIM7000_Service::check_modem_ptr()
{
    return m_modem.expired() ? ESP_ERR_INVALID_STATE : ESP_OK;
}

} // namespace axomotor::sim7000

#include "sim7000_modem.hpp"
#include "sim7000_types.hpp"
#include "sim7000_helpers.hpp"

#include <cstring>
#include <string>
#include <esp_log.h>

namespace axomotor::lte_modem {

#define MODEM_AVAILABLE_BIT     BIT0
#define RESPONSE_STARTED_BIT    BIT1
#define RESPONSE_COMPLETED_BIT  BIT2
#define WAITING_CMD_PAYLOAD_BIT BIT3

#define NULL_CH     '\0'
#define CRLF        "\r\n"
#define CR          "\r"
#define LF          '\n'

using namespace axomotor::lte_modem::internal;
using Parser = SIM7000_BasicModem;

static const char *TAG = "sim7000:basic_modem";

/* SIM7000 Basic Modem */

SIM7000_BasicModem::SIM7000_BasicModem(size_t buffer_size) :
    m_is_running{false},
    m_cmd_context{nullptr},
    m_parser_event_group{}
{
    m_parser_buffer.reserve(buffer_size);
    m_parser_event_group.set_flags(MODEM_AVAILABLE_BIT);
}

esp_err_t SIM7000_BasicModem::execute_cmd(
    internal::at_cmd_t command,
    std::shared_ptr<internal::sim7000_cmd_result_info_t> result_info,
    TickType_t ticks_to_wait,
    bool ignore_response,
    bool is_partial,
    bool is_raw
)
{
    sim7000_cmd_context_t context;
    context.command = command;
    context.result_info = result_info;
    context.ignore_response = ignore_response;
    context.is_raw = is_raw;
    context.is_partial = is_partial;
    
    return execute_cmd(context, ticks_to_wait);
}

esp_err_t SIM7000_BasicModem::execute_cmd(
    internal::at_cmd_t command,
    const std::span<const char> &params,
    std::shared_ptr<internal::sim7000_cmd_result_info_t> result_info,
    TickType_t ticks_to_wait,
    bool ignore_response,
    bool is_partial,
    bool is_raw
)
{
    sim7000_cmd_context_t context;
    context.command = command;
    context.params = params;
    context.result_info = result_info;
    context.ignore_response = ignore_response;
    context.is_raw = is_raw;
    context.is_partial = is_partial;
    
    return execute_cmd(context, ticks_to_wait);
}

esp_err_t SIM7000_BasicModem::execute_cmd(
    internal::sim7000_cmd_context_t &context,
    TickType_t ticks_to_wait
)
{
    // verifica si el receptor está en ejecución
    if (!m_is_running) return ESP_ERR_NOT_ALLOWED;

    // verifica si se recibió un puntero en donde guardar el resultado del
    // comando
    if (!context.result_info) return ESP_ERR_INVALID_ARG;
    auto result_info = context.result_info;
    result_info->reset();
    
    // obtiene la definición del comando recibido
    const at_cmd_def_t *cmd_def = get_command_def(context.command);
    if (cmd_def == nullptr) return ESP_ERR_INVALID_ARG;
    
    // espera hasta que el modem esté disponible
    m_parser_event_group.wait_for_flags(MODEM_AVAILABLE_BIT, true, true);

    std::string cmd_line = "AT";
    const char *cmd_string = cmd_def->string;
    esp_err_t err;
    bool ack;
    bool is_query_cmd = !context.params.empty() && context.params.front() == '?';

    // establece el contexto del comando
    m_cmd_context = &context;

    switch (cmd_def->type) {
        case at_cmd_type_t::BASIC:
            cmd_line.append(cmd_string);
            break;
        case at_cmd_type_t::S_PARAM:
            cmd_line.append(cmd_line);
            cmd_line.append("=");
            break;
        case at_cmd_type_t::EXTENDED:
            cmd_line.append("+");
            cmd_line.append(cmd_string);
            break;
        default:
            break;
    }

    // verifica si hay datos adicionales a enviar
    if (!context.params.empty()) {
        cmd_line.append(context.params.data(), context.params.size());
    }

    // escribe un caracter de retorno de carro para indicar ejecutar el comando
    ESP_LOGD(TAG, "Executing '%s'...", cmd_line.c_str());
    cmd_line.append(CR);
    on_cmd_write(cmd_line.data(), cmd_line.length());

    // verifica si el comando es de consulta (no hay espera)
    if (is_query_cmd) {
        ticks_to_wait = 150;
    }
    // de lo contrario, verifica si no se establecio un tiempo de espera
    else if (ticks_to_wait == 0)
    {
        // verifica si el comando define un tiempo máximo de respuesta
        if (cmd_def->max_response_time != 0) {
            // ignora el tiempo indicado en la función y  en su lugar utiliza
            // el que está definido para el comando (está en segundos)
            ticks_to_wait = cmd_def->max_response_time * 1000;
        }

        // añade 100 ms al tiempo de espera
        ticks_to_wait += pdMS_TO_TICKS(100);
    }

    // espera hasta recibir la confirmación de que se ha empezado a leer la 
    // respuesta o hasta que el tiempo de espera se haya agotado
    ack = m_parser_event_group.wait_for_flags(
        RESPONSE_STARTED_BIT, // bit de confirmación
        true, // borra el bit una vez recibido
        true, // solo espera un bit
        ticks_to_wait // tiempo de espera
    );

    // verifica si el comando requiere enviar una carga útil de datos
    if (ack && context.send_payload) {
        // espera hasta recibir el bit de confirmación
        m_parser_event_group.wait_for_flags(
            WAITING_CMD_PAYLOAD_BIT,
            true,
            true,
            portMAX_DELAY
        );

        ESP_LOGI(
            TAG, 
            "Writing command payload (%u bytes)",
            context.payload.size()
        );

        // escribe la carga útil
        on_cmd_write(context.payload.data(), context.payload.size());
        // envia un enter
        on_cmd_write(CR, 1);
    }

    // verifica si se recibio la confirmación de respuesta recibida
    if (ack) {
        // espera hasta recibir la confirmación de respuesta completada
        ack = m_parser_event_group.wait_for_flags(
            RESPONSE_COMPLETED_BIT, // bit de confirmación
            true, // borra el bit una vez recibido
            true, // solo espera un bit
            ticks_to_wait // tiempo de espera
        );
    }

    // verifica si se ha recibido la respuesta
    if (ack) {
        switch (result_info->result) {
            case at_cmd_result_t::OK:
                if (result_info->response.length() > 0) {
                    ESP_LOGD(
                        TAG,
                        "Command execution successful (%u bytes received)",
                        result_info->response.length()
                    );

                    /* ESP_LOG_BUFFER_HEX(
                        TAG, 
                        result_info->response.c_str(),
                        result_info->response.length()
                    ); */
                } else {
                    ESP_LOGD(TAG, "Command execution successful");
                }
                err = ESP_OK;
                break;
            case at_cmd_result_t::ERROR:
                ESP_LOGE(TAG, "Command execution failed");
                err = ESP_FAIL;
                break;
            case at_cmd_result_t::CME_ERROR:
                ESP_LOGE(
                    TAG,
                    "Command execution failed due to Mobbile Equipment error (%d)",
                    result_info->error_code
                );
                err = ESP_ERR_INVALID_STATE;
                break;
            case at_cmd_result_t::CMS_ERROR:
                ESP_LOGE(
                    TAG,
                    "Command execution failed due to Message or Network error (%d)",
                    result_info->error_code
                );
                err = ESP_ERR_NOT_ALLOWED;
                break;
            case at_cmd_result_t::BUFFER_OVF:
                ESP_LOGE(TAG, "Failed to read command response (buffer overflow)");
                err = ESP_ERR_INVALID_SIZE;
                break;
            default:
                ESP_LOGE(TAG, "Unrecognized command response");
                err = ESP_ERR_INVALID_RESPONSE;
                break;
        }
    } else {
        ESP_LOGW(TAG, "Response timed out");
        err = ESP_ERR_TIMEOUT;
    }

    // restablece el contexto del comando
    //context.reset();
    m_cmd_context = nullptr;
    // hace que el modem esté disponible de nuevo
    m_parser_event_group.set_flags(MODEM_AVAILABLE_BIT);

    return err;
}

void SIM7000_BasicModem::feed_buffer(const char *buffer, size_t length)
{
    // agrega la parte recibida al buffer
    m_parser_buffer.append(buffer, length);
    try_parse();
}

void SIM7000_BasicModem::try_parse()
{
    bool is_completed = false;
    size_t position;
    
    // verifica si actualmente se está ejecutando un comando
    if (m_cmd_context != nullptr) {
        // verifica si no se ha marcado el comienzo de la respuesta
        if (!m_cmd_context->response_received) {
            // notifica al receptor que se ha comenzado a recibir una respuesta
            m_parser_event_group.set_flags(RESPONSE_STARTED_BIT);
            m_cmd_context->response_received = true;
        }

        // verifica si se está ejecutando un comando que requiere enviar datos
        if (m_cmd_context->send_payload && 
            (position = m_parser_buffer.find(CRLF "> ")) != std::string::npos) {
            m_parser_buffer.erase(position, 4);
            m_parser_event_group.set_flags(WAITING_CMD_PAYLOAD_BIT);
        }

        // verifica si se debe recibir una respuesta en crudo
        if (m_cmd_context->is_raw) {
            auto cmd_result = m_cmd_context->result_info;
            // agrega el contenido obtenido
            cmd_result->response.append(m_parser_buffer);
            
            // verifica si el contenido del buffer termina en \r\n
            if (m_parser_buffer.ends_with(CRLF)) {
                // establece los valores de resultado
                cmd_result->result = at_cmd_result_t::OK;
                // notifica que se ha recibido una respuesta
                m_parser_event_group.set_flags(RESPONSE_COMPLETED_BIT);  
            } 

            // borra el contenido del buffer
            m_parser_buffer.clear();

            // termina la función puesto que no necesita comprobar el formato
            // de la respuesta
            return;
        }
    }

    // busca el siguiente salto de línea
    while ((position = m_parser_buffer.find(CRLF)) != std::string::npos) {
        // copia la linea y la borra del buffer
        std::string line = m_parser_buffer.substr(0, position);
        m_parser_buffer.erase(0, position + 2);

        // omite el proceso si la línea está vacía
        if (line.empty()) continue;

        // verifica si el mensaje es un URC
        if (check_if_is_urc(line)) {
            on_urc_message(line);
        }
        // de lo contrario,verifica si actualmente se está ejecutando un comando
        else if (m_cmd_context != nullptr) { 
            auto cmd_result = m_cmd_context->result_info;
            std::string &response = cmd_result->response;

            // verifica el contenido de la línea
            if (line == "OK") {
                cmd_result->result = at_cmd_result_t::OK;
                is_completed = true;
            } else if (line == "ERROR") {
                cmd_result->result = at_cmd_result_t::ERROR;
                is_completed = true;
            } else if (line.starts_with("+CME ERROR")) {
                cmd_result->result = at_cmd_result_t::CME_ERROR;
                cmd_result->error_code = read_error_code(line);
                is_completed = true;
            } else if (line.starts_with("+CMS ERROR")) {
                cmd_result->result = at_cmd_result_t::CMS_ERROR;
                cmd_result->error_code = read_error_code(line);
                is_completed = true;
            } 
                
            // verifica si el comando se ha completado
            if (is_completed) {
                // borra del final cualquier salto de línea excedente
                while (response.ends_with(CRLF)) {
                    response.erase(response.length() -2, 2);
                }
                
                // notifica que se ha recibido una respuesta
                m_parser_event_group.set_flags(RESPONSE_COMPLETED_BIT);
            } else {
                // si no ha terminado, agrega la línea al final de la respuesta
                // junto a un final de la línea, puesto que forma parte de la
                // respuesta
                response.append(line);
                response.append(CRLF);
            }
        } 
    }
}

bool SIM7000_BasicModem::is_urc(const std::string &line)
{
    return check_if_is_urc(line);
}

int SIM7000_BasicModem::read_error_code(const std::string &line)
{
    int offset = line.find_first_of(": ");
    int code = -1;

    if (offset != std::string::npos && offset + 2 < line.length()) {
        code = helpers::to_number<int>(line, offset + 2);
    }

    return code;
}

} // namespace axomotor::lte_modem

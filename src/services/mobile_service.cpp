#include "services/mobile_service.hpp"
#include "services/axomotor_service.hpp"
#include "constants/hw.hpp"
#include "constants/secrets.hpp"
#include "constants/general.hpp"
#include "sim7000_helpers.hpp"

#include <esp_log.h>

namespace axomotor::services {

using namespace axomotor::constants::hw::modem;
using namespace axomotor::constants::general;
using namespace axomotor::events;
using namespace axomotor::lte_modem;

constexpr static const char *TAG = "mobile_service";

MobileService::MobileService() : 
    ServiceBase{TAG, 8 * 1024, 10},
    m_gps_enabled{false},
    m_gps_signal_lost{false}
{
    m_modem = std::make_shared<SIM7000_Modem>(UART_PORT, PIN_U1_RX, PIN_U1_TX, PIN_PWR);
    m_gnss = std::make_shared<SIM7000_GNSS>(m_modem);
    m_mqtt = std::make_shared<SIM7000_MQTT>(m_modem);
    
    apn_config_t config = {
        .apn = "internet.itelcel.com",
        .user = "",
        .pwd = "",
        .cid = 1,
    };

    m_modem->set_apn(config);
}

esp_err_t MobileService::setup()
{
    network_reg_status_t status;
    esp_err_t err;
 
    // espera 5 segundos para esperar a que el modulo se estabilice
    vTaskDelay(pdMS_TO_TICKS(5000));
    // inicia el modulo y habilita la comunicación
    err = m_modem->init();
    if (err == ESP_OK) {
        err = m_modem->enable_comm();
    }

    // verifica si el modulo se inicio correctamente
    if (err != ESP_OK) return err;

    // verifica si el dispositivo está registrado en la red
    do
    {
        m_modem->get_network_reg_status(status);
    } while (status != network_reg_status_t::REGISTERED);

    err = m_modem->sync_time();
    if (err == ESP_OK) {
        AxoMotor::event_group.set_flags(TIME_SYNC_COMPLETED_BIT);
    } else {
        AxoMotor::event_group.set_flags(TIME_SYNC_FAILED_BIT);
    }

    // habilita el reporte de eventos
    m_modem->enable_events();
    esp_event_handler_register(MODEM_EVENTS, ESP_EVENT_ANY_ID, on_event, this);

    // habilita el modulo gps
    m_gnss->turn_on();
    
    // establece la configuración de MQTT
    mqtt_config_t config;
    config.client_id = MQTT_CLIENT_ID;
    config.username = MQTT_USERNAME;
    config.password = MQTT_PASSWORD;
    config.broker = SERVER_HOSTNAME;
    
    err = m_mqtt->set_config(config);
    if (err == ESP_OK) {
        err = m_mqtt->connect();
    }
    
    if (err == ESP_OK) {
        AxoMotor::event_group.set_flags(MOBILE_SERVICE_STARTED_BIT);
        m_mqtt->subscribe("device/1/ping");
    }

    return err;
}

void MobileService::loop()
{
    // determina si hay un viaje activo
    bool trip_active = (AxoMotor::event_group.get_flags() & TRIP_ACTIVE_BIT) != 0;
    // verifica si hay un viaje activo y el gps no está activado
    if (trip_active && !m_gps_enabled) {
        // habilita el reporte de posiciones
        m_gnss->enable_nav_urc(POSITION_REPORT_INTERVAL);
        m_gps_enabled = true;
    } 
    // de lo contrario verifica si no hay un viaje activo y el gps está activado
    else if (!trip_active && m_gps_enabled) {
        m_gnss->disable_nav_urc();
        m_gps_enabled = false;
    }

    int8_t signal_quality;
    std::string op_name;
    network_reg_status_t reg_status;
    bool is_mqtt_active;
    esp_err_t err;

    err = m_mqtt->get_state(is_mqtt_active);
    if (err != ESP_OK || !is_mqtt_active) {
        err = m_mqtt->connect();
        m_mqtt->subscribe("device/1/ping");
    }

    event_type_t type = AxoMotor::queue_set.wait_for_event(pdMS_TO_TICKS(30000));

    switch (type) 
    {
        case event_type_t::POSITION: 
        {
            position_event_t event{};
            AxoMotor::queue_set.position.receive(event, 0);
            err = publish_position(event);
            break;
        } 
        case event_type_t::DEVICE: 
        {
            device_event_t event{};
            AxoMotor::queue_set.device.receive(event, 0);
            err = publish_event(event);
            break;
        }
        case event_type_t::SERVER_PING:
        {
            ping_event_t event{};
            AxoMotor::queue_set.ping.receive(event, 0);
            err = publish_pong(event);
            break;
        }
        default:
            break;
    }
    
    m_modem->get_signal_strength(signal_quality);
    m_modem->get_current_operator(op_name);
    m_modem->get_network_reg_status(reg_status);
}

esp_err_t MobileService::publish_position(position_event_t &event)
{
    char payload[180];
    int length;
    const char topic[] = "trip/688f1945c608c74ce292b148/position";
    const char format[] = "{\"source\":\"vehicleDevice\",\"latitude\":%.6f,\"longitude\":%.6f,\"speed\":%.2f,\"timestamp\":%lld}";

    // escribe el mensaje en formato JSON
    length = snprintf(payload, sizeof(payload), format,
        event.latitude, event.longitude, event.speed_over_ground, event.timestamp
    );
    
    ESP_LOGI(TAG, "Publishing current position...");

    // publica el mensaje
    std::span<char> span(payload);
    return m_mqtt->publish(topic, span.subspan(0, length), 1, 1);
}

esp_err_t MobileService::publish_pong(events::ping_event_t &event)
{
    char payload[32];
    int length;
    const char topic[] = "device/1/ping/pong";
    const char format[] = "{\"timestamp\":%lld}";

    // escribe el mensaje en formato JSON
    length = snprintf(payload, sizeof(payload), format, event.timestamp);
    
    ESP_LOGI(TAG, "Publishing pong...");

    // publica el mensaje
    std::span<char> span(payload);
    return m_mqtt->publish(topic, span.subspan(0, length));
}

esp_err_t MobileService::publish_event(events::device_event_t &event)
{
    char payload[62];
    int length;
    const char topic[] = "device/1/event";
    const char format[] = "{\"code\":\"%s\",\"timestamp\":%lld}";
    const char *event_code;

    switch (event.code)
    {
        case event_code_t::DEVICE_RESET:
            event_code = "deviceReset";
            break;
        case event_code_t::STORAGE_FULL:
            event_code = "storageFull";
            break;
        case event_code_t::STORAGE_FAILURE:
            event_code = "storageFailure";
            break;
        case event_code_t::VIDEO_RECORDING_STARTED:
            event_code = "videoRecordingStarted";
            break;
        case event_code_t::VIDEO_RECORDING_STOPPED:
            event_code = "videoRecordingStopped";
            break;
        case event_code_t::CAMERA_FAILURE:
            event_code = "cameraFailure";
            break;
        case event_code_t::IMPACT_DETECTED:
            event_code = "impactDetected";
            break;
        case event_code_t::HARSH_ACCELERATION:
            event_code = "harshAcceleration";
            break;
        case event_code_t::HARSH_BRAKING:
            event_code = "harshBraking";
            break;
        case event_code_t::HARSH_CORNERING:
            event_code = "harshCornering";
            break;
        case event_code_t::GPS_SIGNAL_LOST:
            event_code = "gpsSignalLost";
            break;
        case event_code_t::GPS_SIGNAL_RESTORED:
            event_code = "gpsSignalRestored";
            break;
        case event_code_t::PANIC_BUTTON_PRESSED:
            event_code = "panicButtonPressed";
            break;
        case event_code_t::TAMPERING_DETECTED:
            event_code = "tamperingDetected";
            break;
        case event_code_t::APP_CONNECTED:
            event_code = "appConnected";
            break;
        case event_code_t::APP_DISCONNECTED:
            event_code = "appDisconnected";
            break;
        case event_code_t::SENSOR_FAILURE:
            event_code = "sensorFailure";
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    // escribe el mensaje en formato JSON
    length = snprintf(
        payload, 
        sizeof(payload), 
        format, 
        event_code, 
        event.timestamp
    );
    
    ESP_LOGI(TAG, "Publishing event '%s'...", event_code);

    // publica el mensaje
    std::span<char> span(payload);
    return m_mqtt->publish(topic, span.subspan(0, length), 2);
}

void MobileService::on_event(void *args, esp_event_base_t base, int32_t id, void *data)
{
    auto instance = reinterpret_cast<MobileService *>(args);

    if (id == MODEM_EVENT_MQTT_MESSAGE_RECEIVED) {
        auto message = reinterpret_cast<mqtt_message_t *>(data);

        ESP_LOGI(
            TAG, 
            "Topic: %s | Content: %s",
            message->topic,
            message->content
        );

        ping_event_t event{};
        event.ping_timestamp = 0;
        event.timestamp = xTaskGetTickCount();
        AxoMotor::queue_set.ping.overwrite(event);
    } else if (id == MODEM_EVENT_GNSS_NAVIGATION_REPORT) {
        auto info = reinterpret_cast<gnss_nav_info_t *>(data);
        
        ESP_LOGI(
            TAG, 
            "Coordinates: latitude=%.6f, longitude=%.6f, speed=%.2f km/h", 
            info->latitude, 
            info->longitude,
            info->speed_over_ground
        );

        // verifica si se recibieron las coordenadas
        if (info->fix_status) {
            position_event_t event{};
            event.latitude = info->latitude;
            event.longitude = info->longitude;
            event.speed_over_ground = info->speed_over_ground;
            event.course_over_ground = info->course_over_ground;
            event.timestamp = helpers::parse_to_epoch(info->date_time);
            
            // registra la posición actual
            AxoMotor::queue_set.position.send_to_back(event, 0);

            // verifica si se habia perdido la señal
            if (instance->m_gps_signal_lost) {
                instance->m_gps_signal_lost = false;

                // registra un evento de recuperación de señal
                AxoMotor::queue_set.device.send_to_back(
                    event_code_t::GPS_SIGNAL_RESTORED
                );
            }
        } else {
            // verifica si no se habia perdido la señal
            if (!instance->m_gps_signal_lost) {
                instance->m_gps_signal_lost = true;

                // registra un evento de perdida de señal
                AxoMotor::queue_set.device.send_to_back(
                    event_code_t::GPS_SIGNAL_LOST
                );
            }
        }
    }
}

} // namespace axomotor::services

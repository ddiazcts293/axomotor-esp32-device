#pragma once

#include <freertos/FreeRTOS.h>
#include <ctime>

namespace axomotor::events
{
    enum class event_code_t : uint8_t
    {
        NONE = 0,
        DEVICE_RESET,               // Reinicio del dispositivo
        STORAGE_FULL,               // Memoria llena
        STORAGE_FAILURE,            // Fallo de almacenamiento
        SENSOR_FAILURE,             // Fallo de sensores
        VIDEO_RECORDING_STARTED,    // Inicio de grabación de video
        VIDEO_RECORDING_STOPPED,    // Fin de grabación
        CAMERA_FAILURE,             // Fallo de cámara
        IMPACT_DETECTED,            // Golpe detectado
        HARSH_ACCELERATION,         // Aceleración brusca
        HARSH_BRAKING,              // Frenado brusco
        HARSH_CORNERING,            // Giro violento o brusco
        GPS_SIGNAL_LOST,            // Señal GPS perdida
        GPS_SIGNAL_RESTORED,        // Señal GPS recuperada
        PANIC_BUTTON_PRESSED,       // Botón de pánico activado
        TAMPERING_DETECTED,         // Manipulación del dispositivo
        APP_CONNECTED,              // Aplicación conectada
        APP_DISCONNECTED,           // Aplicación desconectada
    };

    enum class event_type_t
    {
        NONE,
        DEVICE,
        POSITION,
        NETWORK,
        SERVER_PING
    };

    struct device_event_t
    {
        time_t timestamp;
        event_code_t code;
    };
    
    struct position_event_t
    {
        time_t timestamp;
        float latitude;
        float longitude;
        float speed_over_ground;
        float course_over_ground;
    };

    struct ping_event_t
    {
        uint64_t ping_timestamp;
        time_t timestamp;
    };
    
} // namespace axomotor::event

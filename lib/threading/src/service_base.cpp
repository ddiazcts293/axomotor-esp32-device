#include "service_base.hpp"
#include <esp_log.h>

#define SERVICE_STARTED_BIT             BIT0
#define SERVICE_SETUP_COMPLETED_BIT     BIT1
#define SERVICE_SETUP_FAILED_BIT        BIT2
#define SERVICE_STOP_REQUESTED_BIT      BIT3

namespace axomotor::threading {

constexpr static const char *TAG = "service_base";

ServiceBase::ServiceBase(const char *name, size_t stack_size, uint32_t priority, int core) :
    m_task_handle(nullptr)
    //loop_delay(DEFAULT_TASK_DELAY)
{
    m_pthread_config = esp_pthread_get_default_config();
    m_pthread_config.thread_name = name,
    m_pthread_config.stack_size = stack_size;
    m_pthread_config.prio = priority;
    
    if (core != tskNO_AFFINITY)
    {
        m_pthread_config.pin_to_core = core;
    }
}
    
ServiceBase::ServiceBase(const esp_pthread_cfg_t &config)
    : m_pthread_config{config}
{ }

ServiceBase::~ServiceBase()
{
    if (is_active())
    {
        stop();
    }
}

esp_err_t ServiceBase::start()
{
    // obtiene el nombre del servicio
    const char *name = m_pthread_config.thread_name;
    // obtiene los indicadores de eventos activos
    uint32_t flags = m_event_group.get_flags();

    // verifica si el servicio ya fue iniciado anteriormente
    if (flags & SERVICE_STARTED_BIT)
    {
        ESP_LOGW(TAG, "Service '%s' is already running", name);
        return ESP_ERR_INVALID_STATE;
    }

    // establece la configuración del servicio para usarse en la 
    // inicialización posterior del mismo
    esp_err_t result = esp_pthread_set_cfg(&m_pthread_config);
    if (result)
    {
        ESP_LOGE(
            TAG,
            "Service '%s' pthread configuration failed (%s)",
            name,
            esp_err_to_name(result)
        );

        return result;
    }

    ESP_LOGD(TAG, "Starting service '%s'...", name);

    // crea un nuevo hilo que ejecutará el servicio
    m_thread = std::thread{&ServiceBase::run, this};
    // espera a que el hilo marque el indicador de inicio
    flags = m_event_group.wait_for_flags(
        SERVICE_STARTED_BIT,
        false,
        true,
        SERVICE_START_TIMEOUT
    );

    // verifica si no se pudo inicializar el servicio
    if ((flags & SERVICE_STARTED_BIT) == 0)
    {
        ESP_LOGW(TAG, "Task start timed out");
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t ServiceBase::stop()
{
    const char *name = m_pthread_config.thread_name;
    uint32_t flags = m_event_group.get_flags();

    // verifica si el servicio no está en ejecución comprobando si el puntero
    // de la tarea es nulo o si no se establecio el indicador de inicio
    if (m_task_handle == nullptr || (flags & SERVICE_STARTED_BIT) == 0)
    {
        ESP_LOGW(TAG, "Service '%s' is not running", name);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGD(TAG, "Stopping service '%s'", name);
    
    // indica al hilo que deberá finalizar el servicio
    m_event_group.set_flags(SERVICE_STOP_REQUESTED_BIT);

    // verifica si el puntero de la tarea es diferente al de la tarea actual
    // y si es posible esperar a que el hilo finalice (evita un bloqueo o 
    // deadlock cuando se solicita detener el servicio desde si mismo)
    if (xTaskGetCurrentTaskHandle() != m_task_handle && m_thread.joinable())
    {
        m_thread.join();
    }

    return ESP_OK;
}

bool ServiceBase::is_active() const
{
    return (m_event_group.get_flags() & SERVICE_STARTED_BIT) != 0;
}

const char *ServiceBase::get_name() const
{
    return m_pthread_config.thread_name;
}

uint32_t ServiceBase::get_free_stack_size() const
{
    // devuelve la cantidad mínima de espacio libre en la memoria stack para el
    // hilo actual
    return uxTaskGetStackHighWaterMark(m_task_handle);
}

esp_err_t ServiceBase::setup()
{
    return ESP_OK;
}

void ServiceBase::finish()
{ }

void ServiceBase::run()
{
    // obtiene el puntero de la tarea asociada al hilo actual
    m_task_handle = xTaskGetCurrentTaskHandle();
    const char *name = pcTaskGetName(m_task_handle);

    // establece el indicador de servicio iniciado
    ESP_LOGI(TAG, "Service '%s' has been started", name);
    m_event_group.set_flags(SERVICE_STARTED_BIT);

    // configura el servicio
    esp_err_t setup_result = setup();
    // establece el indicador de configuración de servicio
    m_event_group.set_flags(setup_result == ESP_OK ? 
        SERVICE_SETUP_COMPLETED_BIT : 
        SERVICE_SETUP_FAILED_BIT);

    if (setup_result == ESP_OK) {
        ESP_LOGD(TAG, "Service '%s' setup is completed", name);

        // ejecuta el bucle del servicio mientras no se solicite deternerlo
        while ((m_event_group.get_flags() & SERVICE_STOP_REQUESTED_BIT) == 0) {
            loop();
        }

        // finaliza el servicio
        finish();
        ESP_LOGI(TAG, "Service '%s' has been stopped", name);
    } else {
        ESP_LOGE(
            TAG, 
            "Service '%s' setup has failed (%s)", 
            name, 
            esp_err_to_name(setup_result));
    }

    // crea una máscara de bits con los indicadores que deberán desactivarse
    uint32_t flags =
        SERVICE_STARTED_BIT |
        SERVICE_SETUP_COMPLETED_BIT |
        SERVICE_SETUP_FAILED_BIT |
        SERVICE_STOP_REQUESTED_BIT;

    // desactiva los indicadores de servicio en ejecución
    m_event_group.clear_flags(flags);
    m_task_handle = nullptr;
}

} // namespace axomotor::threading

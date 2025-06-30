#pragma once

#include <atomic>
#include <mutex>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

namespace axomotor::threading {

template <typename T, size_t TBufferSize>
class QueueBase
{
public:
    QueueBase()
    {
        static_assert(TBufferSize > 0);
        assert(m_handle = xRingbufferCreate(TBufferSize, RINGBUF_TYPE_BYTEBUF));
    }

    ~QueueBase()
    {
        vRingbufferDelete(m_handle);
    }

    template <typename T>
    esp_err_t retrieve(T &data)
    {
        // activa el mutex para evitar que se ejecute la función desde otro 
        // hilo
        std::unique_lock lock{mutex};
        esp_err_t result;

        // verifica si no hay datos sin leer o si actualmente se están leyendo
        if (!m_has_unread_data || m_is_retrieving_data)
        {
            // no se permite obtener datos cuando no hay nada
            result = ESP_ERR_NOT_ALLOWED;
        }
        // de lo contrario verifica si la longitud de los datos es diferente
        // al tamaño esperado por la función
        else if (sizeof(T) != m_unread_data_length)
        {
            // tamaño no válido
            result = ESP_ERR_INVALID_SIZE;
        }
        else
        {
            // establece el indicador de obtención de datos en curso
            m_is_retrieving_data = true;

            // intenta leer los datos
            result = read(&data, sizeof(T), 0);

            // verifica si la longitud leídos es mayor que cero
            if (result == ESP_ERR_TIMEOUT)
            {
                result = ESP_ERR_INVALID_SIZE;
            }

            // restablece las variables a sus valores predeterminados
            m_unread_data_length = 0;
            m_has_unread_data = false;
            m_is_retrieving_data = false;
        }

        return result;
    }

    esp_err_t discard()
    {
        // activa el mutex para evitar que se ejecute la función desde otro 
        // hilo
        std::unique_lock<std::mutex> lock{mutex};
        esp_err_t result;

        // verifica si no hay datos sin leer o si se están leyendo
        if (!m_has_unread_data || m_is_retrieving_data)
        {
            // no se permite obtener datos cuando no los hay
            result = ESP_ERR_NOT_ALLOWED;
        }
        else
        {
            // establece el indicador de obtención de datos en curso
            m_is_retrieving_data = true;
            size_t expected_length = m_unread_data_length;
            size_t length = 0;

            // obtiene el puntero a los datos en la cola
            void *data_ptr = xRingbufferReceiveUpTo(
                m_handle,
                &length,
                0, // no hay tiempo de espera porque ya están los datos allí
                expected_length);

            // verifica si la longitud de los datos leídos es menor que la
            // esperada
            if (length < expected_length)
            {
                expected_length -= length;

                // retorna el puntero para leer los datos restantes
                vRingbufferReturnItem(m_handle, data_ptr);

                // lee los datos restantes
                data_ptr = xRingbufferReceiveUpTo(
                    m_handle,
                    &length,
                    0,
                    expected_length);

                // establece el valor de salida
                result = length == expected_length ? ESP_OK : ESP_ERR_INVALID_SIZE;
            }
            else
            {
                result = ESP_OK;
            }

            // retorna el puntero a la cola
            vRingbufferReturnItem(m_handle, data_ptr);
            
            // restablece las varias a sus valores predeterminados
            m_unread_data_length = 0;
            m_has_unread_data = false;
            m_is_retrieving_data = false;
        }

        return result;
    }

    size_t get_available_space()
    {
        UBaseType_t bytes_used = 0;
        // obtiene la cantidad de bytes utilizados en la cola
        vRingbufferGetInfo(m_handle, NULL, NULL, NULL, NULL, &bytes_used);
        return TBufferSize - bytes_used;
    }

    void print_info()
    {
        xRingbufferPrintInfo(m_handle);
    }

protected:
    /// @brief Longitud máxima de los datos en la cola
    constexpr static const size_t MAX_DATA_LENGTH = UINT8_MAX;

    esp_err_t write(
        void *buffer, 
        const size_t buffer_length, 
        TickType_t ticks_to_wait)
    {
        return xRingbufferSend(m_handle, buffer, buffer_length, ticks_to_wait) ?
            ESP_OK : ESP_ERR_TIMEOUT;
    }

    esp_err_t read(
        void *buffer, 
        const size_t expected_length, 
        TickType_t ticks_to_wait)
    {
        size_t length = 0;
        esp_err_t result;

        void *data_ptr = xRingbufferReceiveUpTo(
            m_handle,
            &length,
            ticks_to_wait,
            expected_length);

        // verifica si la longitud leídos es mayor que cero
        if (length > 0)
        {
            // copia los datos leídos
            memcpy(buffer, data_ptr, length);

            // verifica si la longitud de los datos leídos es menor que la
            // esperada
            if (length < expected_length)
            {
                // obtiene la longitud restante
                size_t remaining_length = expected_length - length;
                // reposiciona la dirección del puntero
                buffer = (void *)((size_t)buffer + length);

                // retorna el puntero para leer los datos restantes
                vRingbufferReturnItem(m_handle, data_ptr);

                // lee los datos restantes
                data_ptr = xRingbufferReceiveUpTo(
                    m_handle,
                    &length,
                    0,
                    remaining_length);

                // verifica si la longitud de los datos leídos es igual que
                // la esperada
                if (length == remaining_length)
                {
                    // copia los datos restantes
                    memcpy(buffer, data_ptr, length);
                    result = ESP_OK;
                }
                else
                {
                    result = ESP_ERR_INVALID_SIZE;
                }
            }
            else
            {
                result = ESP_OK;
            }

            // retorna el puntero a la cola
            vRingbufferReturnItem(m_handle, data_ptr);
        }
        else
        {
            result = ESP_ERR_TIMEOUT;
        }

        return result;
    }

    /// @brief Mutex para restringir la obtención de datos a un solo hilo
    std::mutex mutex;
    /// @brief Indicador de datos sin leer
    std::atomic_bool m_has_unread_data;
    /// @brief Indicador de obtención de datos en curso
    std::atomic_bool m_is_retrieving_data;
    /// @brief Longitud de los datos sin leer
    std::atomic_size_t m_unread_data_length;

private:    
    /// @brief Puntero de la cola
    RingbufHandle_t m_handle;
};

} // namespace axomotor::threading

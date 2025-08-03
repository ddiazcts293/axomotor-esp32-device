#pragma once

#include <memory>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "definitions.hpp"

namespace axomotor::events {

template <typename T>
class EventQueue
{
friend class EventQueueSet;

public:
    explicit EventQueue(size_t size) : m_size{size}, m_handle{nullptr}
    {
        assert(m_handle = xQueueCreate(size, sizeof(T)));
    }

    EventQueue(const EventQueue &) = delete;
    EventQueue(EventQueue &&) = delete;

    ~EventQueue()
    {
        vQueueDelete(m_handle);
    }

    bool send_to_back(const T &item, TickType_t ticks_to_wait = portMAX_DELAY) const
    {
        return xQueueSendToBack(m_handle, &item, ticks_to_wait);
    }

    bool send_to_front(const T &item, TickType_t ticks_to_wait = portMAX_DELAY) const
    {
        return xQueueSendToFront(m_handle, &item, ticks_to_wait);
    }

    bool receive(T &item, TickType_t ticks_to_wait = portMAX_DELAY) const
    {
        return xQueueReceive(m_handle, &item, ticks_to_wait);
    }

    size_t count_items_waiting() const
    {
        return uxQueueMessagesWaiting(m_handle);
    }

    size_t count_free_spaces() const
    {
        return uxQueueSpacesAvailable(m_handle);
    }

    size_t size() const
    {
        return m_size;
    }

    void reset()
    {
        xQueueReset(m_handle);
    }

    bool peek(T &item, TickType_t ticks_to_wait = portMAX_DELAY) const
    {
        return xQueuePeek(m_handle, &item, ticks_to_wait);
    }

    bool overwrite(const T &item)
    {
        if (m_size != 1)
            return send_to_front(item, 0);

        xQueueOverwrite(m_handle, &item);
        return true;
    }

    EventQueue &operator=(const EventQueue &) = delete;
    EventQueue &operator=(EventQueue &&) = delete;

private:
    const size_t m_size;
    QueueHandle_t m_handle;
};

using DeviceEventQueue = EventQueue<device_event_t>;
using PositionEventQueue = EventQueue<position_event_t>;
using PingEventQueue = EventQueue<ping_event_t>;

class EventQueueSet
{
public:
    EventQueueSet();
    
    event_type_t wait_for_event(TickType_t ticks_to_wait = portMAX_DELAY);
    DeviceEventQueue &device();
    PositionEventQueue &position();
    PingEventQueue &ping();
    
private:
    DeviceEventQueue m_device_queue;
    PositionEventQueue m_position_queue;
    PingEventQueue m_ping_queue;
    QueueSetHandle_t m_handle;
};


} // namespace axomotor::events

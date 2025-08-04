#pragma once

#include <memory>
#include <ctime>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "definitions.hpp"

namespace axomotor::events {

time_t get_timestamp();

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

    void reset() const
    {
        xQueueReset(m_handle);
    }

    bool peek(T &item, TickType_t ticks_to_wait = portMAX_DELAY) const
    {
        return xQueuePeek(m_handle, &item, ticks_to_wait);
    }

    bool overwrite(const T &item) const
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

class DeviceEventQueue : public EventQueue<device_event_t>
{
friend class EventQueueSet;

public:
    DeviceEventQueue(size_t length);
    bool send_to_back(const event_code_t code, TickType_t ticks_to_wait = portMAX_DELAY) const;
    bool send_to_front(const event_code_t code, TickType_t ticks_to_wait = portMAX_DELAY) const;
};

using PositionEventQueue = EventQueue<position_event_t>;
using PingEventQueue = EventQueue<ping_event_t>;

class EventQueueSet
{
public:
    EventQueueSet();
    
    event_type_t wait_for_event(TickType_t ticks_to_wait = portMAX_DELAY) const;
    
    const DeviceEventQueue device;
    const PositionEventQueue position;
    const PingEventQueue ping;
private:
    QueueSetHandle_t m_handle;
};


} // namespace axomotor::events

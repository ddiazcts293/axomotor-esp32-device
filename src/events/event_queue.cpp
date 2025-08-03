#include "events/event_queue.hpp"
#include "constants/general.hpp"

namespace axomotor::events {

using namespace axomotor::constants::general;

EventQueueSet::EventQueueSet() :
    m_device_queue{DEVICE_EVENTS_QUEUE_LENGTH},
    m_position_queue{POSITION_EVENTS_QUEUE_LENGTH},
    m_ping_queue{PING_EVENTS_QUEUE_LENGTH}
{ 
    const size_t combined_length = 
        DEVICE_EVENTS_QUEUE_LENGTH +
        POSITION_EVENTS_QUEUE_LENGTH +
        PING_EVENTS_QUEUE_LENGTH;

    assert(m_handle = xQueueCreateSet(combined_length));
    xQueueAddToSet(m_device_queue.m_handle, m_handle);
    xQueueAddToSet(m_position_queue.m_handle, m_handle);
    xQueueAddToSet(m_ping_queue.m_handle, m_handle);
}

event_type_t EventQueueSet::wait_for_event(TickType_t ticks_to_wait)
{
    auto activated_member = xQueueSelectFromSet(m_handle, ticks_to_wait);

    if (activated_member == m_device_queue.m_handle) {
        return event_type_t::DEVICE;
    } else if (activated_member == m_position_queue.m_handle) {
        return event_type_t::POSITION;
    } else if (activated_member == m_ping_queue.m_handle) {
        return event_type_t::SERVER_PING;
    } else {
        return event_type_t::NONE;
    }
}

DeviceEventQueue &EventQueueSet::device()
{
    return m_device_queue;
}

PositionEventQueue &EventQueueSet::position()
{
    return m_position_queue;
}

PingEventQueue &EventQueueSet::ping()
{
    return m_ping_queue;
}

}

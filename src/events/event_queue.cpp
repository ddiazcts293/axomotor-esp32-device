#include "events/event_queue.hpp"
#include "constants/general.hpp"

namespace axomotor::events {

using namespace axomotor::constants::general;

time_t get_timestamp()
{
    time_t epoch = 0;
    time(&epoch);
    return epoch;
}

DeviceEventQueue::DeviceEventQueue(size_t length) : EventQueue<device_event_t>(length)
{ }

bool DeviceEventQueue::send_to_back(const event_code_t code, TickType_t ticks_to_wait) const
{
    device_event_t event{};
    event.code = code;
    event.timestamp = get_timestamp();
    return EventQueue<device_event_t>::send_to_back(event, ticks_to_wait);
}

bool DeviceEventQueue::send_to_front(const event_code_t code, TickType_t ticks_to_wait) const
{
    device_event_t event{};
    event.code = code;
    event.timestamp = get_timestamp();
    
    return EventQueue<device_event_t>::send_to_front(event, ticks_to_wait);
}

EventQueueSet::EventQueueSet() :
    device{DEVICE_EVENTS_QUEUE_LENGTH},
    position{POSITION_EVENTS_QUEUE_LENGTH},
    ping{PING_EVENTS_QUEUE_LENGTH}
{
    const size_t combined_length = 
        DEVICE_EVENTS_QUEUE_LENGTH +
        POSITION_EVENTS_QUEUE_LENGTH +
        PING_EVENTS_QUEUE_LENGTH;

    assert(m_handle = xQueueCreateSet(combined_length));
    xQueueAddToSet(device.m_handle, m_handle);
    xQueueAddToSet(position.m_handle, m_handle);
    xQueueAddToSet(ping.m_handle, m_handle);
}

event_type_t EventQueueSet::wait_for_event(TickType_t ticks_to_wait) const
{
    auto activated_member = xQueueSelectFromSet(m_handle, ticks_to_wait);

    if (activated_member == device.m_handle) {
        return event_type_t::DEVICE;
    } else if (activated_member == position.m_handle) {
        return event_type_t::POSITION;
    } else if (activated_member == ping.m_handle) {
        return event_type_t::SERVER_PING;
    } else {
        return event_type_t::NONE;
    }
}

}

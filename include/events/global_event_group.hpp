#pragma once

#include <event_group.hpp>

#define MOBILE_SERVICE_STARTED_BIT          BIT0
#define SD_LOADED_BIT                       BIT4
#define TIME_SYNC_COMPLETED_BIT             BIT5
#define TIME_SYNC_FAILED_BIT                BIT6
#define APP_CONNECTED_BIT                   BIT7
#define TRIP_ACTIVE_BIT                     BIT8

namespace axomotor::events
{
    class GlobalEventGroup : public threading::EventGroup
    {
    public:
        void wait_until_system_is_ready(TickType_t ticks_to_wait = portMAX_DELAY) const
        {
            wait_for_flags(IS_READY_FLAGS, false, true, ticks_to_wait);
        }

        bool is_trip_active() const
        {
            return (get_flags() & TRIP_ACTIVE_BIT) != 0;
        }

        bool is_system_ready() const
        {
            return (get_flags() & IS_READY_FLAGS) == IS_READY_FLAGS;
        }
    private:
        const uint32_t IS_READY_FLAGS = TRIP_ACTIVE_BIT | SD_LOADED_BIT | TIME_SYNC_COMPLETED_BIT;
    };

} // namespace axomotor::events

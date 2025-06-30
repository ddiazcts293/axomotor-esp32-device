#include "event_group.hpp"

namespace axomotor::threading {

EventGroup::EventGroup()
{
    assert(m_handle = xEventGroupCreate());
}

EventGroup::~EventGroup()
{
    vEventGroupDelete(m_handle);
}

uint32_t EventGroup::get_flags() const
{
    return xEventGroupGetBits(m_handle);
}

uint32_t EventGroup::set_flags(uint32_t flags) const
{
    return xEventGroupSetBits(m_handle, flags);
}

uint32_t EventGroup::clear_flags(uint32_t flags) const
{
    return xEventGroupClearBits(m_handle, flags);
}

uint32_t EventGroup::wait_for_flags(
    uint32_t flags,
    bool clear_on_exit,
    bool wait_for_all_flags,
    TickType_t ticks_to_wait) const
{
    return xEventGroupWaitBits(
        m_handle,
        flags,
        clear_on_exit,
        wait_for_all_flags,
        ticks_to_wait
    );
}

uint32_t EventGroup::sync_flags(
    uint32_t flags_to_set,
    uint32_t flags_to_wait_for,
    TickType_t ticks_to_wait) const
{
    return xEventGroupSync(
        m_handle,
        flags_to_set,
        flags_to_wait_for,
        ticks_to_wait
    );
}

} // namespace axomotor::threading

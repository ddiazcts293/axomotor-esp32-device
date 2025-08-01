#pragma once

#include <atomic>
#include <thread>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_pthread.h>
#include <esp_timer.h>

#include "event_group.hpp"

namespace axomotor::threading {

class ServiceBase
{
public:
    constexpr static const TickType_t SERVICE_START_TIMEOUT = pdMS_TO_TICKS(500);

    ServiceBase(
        const char *name,
        size_t stack_size = CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT,
        uint32_t priority = CONFIG_PTHREAD_TASK_PRIO_DEFAULT,
        int core = tskNO_AFFINITY
    );
    
    ServiceBase(const esp_pthread_cfg_t &);
    ServiceBase(const ServiceBase &) = delete;
    ServiceBase(ServiceBase &&) = delete;
    ~ServiceBase();

    esp_err_t start(bool wait_for_setup = false);
    esp_err_t stop();
    
    bool is_active() const;
    const char *get_name() const;
    uint32_t get_free_stack_size() const;

    ServiceBase &operator=(const ServiceBase &) = delete;
    ServiceBase &operator=(ServiceBase &&) = delete;

protected:
    virtual esp_err_t setup();
    virtual void loop() = 0;
    virtual void finish();

private:
    TaskHandle_t m_task_handle;
    EventGroup m_event_group;
    esp_pthread_cfg_t m_pthread_config;
    esp_err_t m_setup_err;
    std::thread m_thread;

    void run();
};

} // namespace axomotor::threading

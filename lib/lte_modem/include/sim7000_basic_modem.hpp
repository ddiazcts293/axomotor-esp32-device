#pragma once

#include "sim7000_types.hpp"

#include <memory>
#include <mutex>

#include <esp_err.h>
#include <event_group.hpp>

namespace axomotor::lte_modem
{
    class SIM7000_BasicModem
    {
    public:
        SIM7000_BasicModem(size_t buffer_size);        
        
        esp_err_t execute_cmd(
            internal::at_cmd_t command,
            std::shared_ptr<internal::sim7000_cmd_result_info_t> result_info,
            TickType_t ticks_to_wait = 0,
            bool ignore_response = false,
            bool is_partial = false,
            bool is_raw = false);

        esp_err_t execute_cmd(
            internal::at_cmd_t command,
            const std::span<const char> &payload,
            std::shared_ptr<internal::sim7000_cmd_result_info_t> result_info,
            TickType_t ticks_to_wait = 0,
            bool ignore_response = false,
            bool is_partial = false,
            bool is_raw = false);
    
    protected:
        void feed_buffer(const char *buffer, size_t length);
        virtual void on_urc_message(std::string &) = 0;
        virtual int on_cmd_write(const char *, size_t) = 0;
    private:
        internal::sim7000_cmd_context_t m_cmd_context;
        std::weak_ptr<internal::sim7000_cmd_result_info_t> m_result_info;
        std::string m_parser_buffer;
        threading::EventGroup m_parser_event_group;

        void try_parse();
        static bool is_urc(const std::string &line);
        static int read_error_code(const std::string &line);
    };

} // namespace axomotor::lte_modem

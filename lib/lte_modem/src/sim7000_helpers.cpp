#include "sim7000_helpers.hpp"
#include <string.h>

namespace axomotor::lte_modem::helpers
{
    bool extract_token(std::string &str, size_t token_idx, const char *delimiters, bool keep_empty_tokens)
    {
        return extract_token(str, token_idx, delimiters, str, keep_empty_tokens);
    }

    bool extract_token(const std::string &str, size_t token_idx, const char *delimiters, std::string &token_out, bool keep_empty_tokens)
    {
        if (!delimiters || *delimiters == '\0') return false;
        
        size_t current_token = 0;
        size_t start = 0;
        size_t len = str.length();
        size_t i = 0;

        while (i <= len) {
            // Si llegamos al final o encontramos delimitador
            if (i == len || strchr(delimiters, str[i])) {
                if (keep_empty_tokens || start != i) {
                    if (current_token == token_idx) {
                        token_out.assign(str, start, i - start);
                        return true;
                    }
                    ++current_token;
                }
                start = i + 1;
            }
            i++;
        }
        return false;
    }

    void ltrim(std::string &str, const char *chars)
    {
        if (!chars || str.empty()) return;
        size_t pos = 0;
        while (pos < str.size() && strchr(chars, str[pos])) ++pos;
        if (pos > 0) str.erase(0, pos);
    }

    void rtrim(std::string &str, const char *chars)
    {
        if (!chars || str.empty()) return;
        size_t pos = str.size();
        while (pos > 0 && strchr(chars, str[pos - 1])) --pos;
        if (pos < str.size()) str.erase(pos);
    }

    void trim(std::string &str, const char *chars)
    {
        ltrim(str, chars);
        rtrim(str, chars);
    }

    void remove_before(std::string &str, const char *seq, bool keep_seq)
    {
        if (str.empty() || !seq) return;
        size_t index = str.find_first_of(seq);
        
        if (index != std::string::npos) {
            if (!keep_seq) index += strlen(seq);
            str.erase(0, index);
        }
    }

    void remove_after(std::string &str,  const char *seq, bool keep_seq)
    {
        if (str.empty() || !seq) return;
        size_t index = str.find_first_of(seq);
        
        if (index != std::string::npos) {
            if (keep_seq) index += strlen(seq);
            
            str.erase(index);
        }
    }

    time_t gps_ts_to_epoch_ts(uint64_t gps_timestamp)
    {
        if (gps_timestamp == 0) return 0;
        time_t epoch;
        struct tm c_dt;

        // formato: 20250801000426 => 2025_08_01_000426
        //             10000000000

        c_dt.tm_year = (gps_timestamp / 10000000000) - 1900; // 2025
        gps_timestamp %= 10000000000;       // 801000426
        c_dt.tm_mon = (gps_timestamp / 100000000) - 1;  // 08
        gps_timestamp %= 100000000;         // 1000426
        c_dt.tm_mday = gps_timestamp / 1000000;      // 01
        gps_timestamp %= 1000000;           // 000426
        c_dt.tm_hour = gps_timestamp / 10000;       // 00
        gps_timestamp %= 10000;             // 0426
        c_dt.tm_min = gps_timestamp / 100;       // 04
        gps_timestamp %= 100;               // 26
        c_dt.tm_sec = (int)gps_timestamp;
        c_dt.tm_isdst = 0;

        epoch = mktime(&c_dt);
        return epoch;
    }

} // namespace axomotor::lte_modem

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

    time_t parse_to_epoch(uint64_t gps_timestamp)
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

    void parse_gnss_info(std::string &payload, gnss_nav_info_t &info)
    {
        std::string aux;

        // quita el inicio del comando
        remove_before(payload, ": ");
        // obtiene el estado de ejecución
        extract_token(payload, 0, ",", aux, true);
        info.run_status = to_number<uint8_t>(aux);
        // obtiene el indicador FIX
        extract_token(payload, 1, ",", aux, true);
        info.fix_status = to_number<uint8_t>(aux);//fields[1]);
        // obtiene la fecha y hora
        extract_token(payload, 2, ",", aux, true);
        info.date_time = to_number<uint64_t>(aux);//fields[2]);
        // obtiene la latitud
        extract_token(payload, 3, ",", aux, true);
        info.latitude = to_number<float>(aux);//fields[3]);
        // obtiene la longitud
        extract_token(payload, 4, ",", aux, true);
        info.longitude = to_number<float>(aux);//fields[4]);
        // obtiene la altitud
        extract_token(payload, 5, ",", aux, true);
        info.msl_altitude = to_number<float>(aux);//fields[5]);
        // obtiene la velocidad
        extract_token(payload, 6, ",", aux, true);
        info.speed_over_ground = to_number<float>(aux);//fields[6]);
        // obtiene el curso sobre la tierra
        extract_token(payload, 7, ",", aux, true);
        info.course_over_ground = to_number<float>(aux);//fields[7]);
        // obtiene el indicador FIX MODE
        extract_token(payload, 8, ",", aux, true);
        info.fix_mode = to_number<uint8_t>(aux);//fields[8]);
        // obtiene la cantidad de satelites de GNSS en vista
        extract_token(payload, 14, ",", aux, true);
        info.gnss_satellites = to_number<uint8_t>(aux);//fields[14]);
        // obtiene la cantidad de satelites de GPS en vista
        extract_token(payload, 15, ",", aux, true);
        info.gps_satellites = to_number<uint8_t>(aux);//fields[15]);
    }

    time_t parse_to_epoch(std::string &payload)
    {
        if (payload.empty()) return 0;
        int diff;
        time_t epoch;
        struct tm c_dt;
        std::string aux;

        //  25/08/04,03:04:29-28
        extract_token(payload, 0, "/", aux, true);  // 25
        c_dt.tm_year = 100 + to_number<int>(aux); // 125
        extract_token(payload, 1, "/", aux, true);  // 08
        c_dt.tm_mon = -1 + to_number<int>(aux);   // 7
        extract_token(payload, 2, "/,", aux, true); // 04
        c_dt.tm_mday = to_number<int>(aux);       // 4
        
        remove_before(payload, ",", false);
        // 03:04:29-28
        extract_token(payload, 0, ":", aux, true);  // 03
        c_dt.tm_hour = to_number<int>(aux);       // 3
        extract_token(payload, 1, ":", aux, true);  // 04
        c_dt.tm_min = to_number<int>(aux);        // 4
        extract_token(payload, 0, ":-", aux, true); // 29
        c_dt.tm_sec = to_number<int>(aux);        // 29
        c_dt.tm_isdst = 0;
        
        // obtiene la diferencia de horas
        aux = payload.substr(8);
        diff = to_number<int>(aux);
        
        c_dt.tm_hour -= (diff / 4);
        
        epoch = mktime(&c_dt);
        return epoch;
    }

} // namespace axomotor::lte_modem

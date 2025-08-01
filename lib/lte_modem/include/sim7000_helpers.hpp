#pragma once

#include <cstddef>
#include <ctime>
#include <string>
#include <array>
#include <type_traits>
#include <span>

namespace axomotor::lte_modem::helpers
{
    struct split_str_t
    {
        size_t index;
        size_t length;
    };

    // Función plantilla para convertir un fragmento de buffer a un valor numérico.
    // T: tipo de valor numérico (int, long, float, double, etc.)
    template <typename T>
    T to_number(const char *buffer, const size_t bufsize, size_t len = 0)
    {
        if (!buffer || bufsize == 0)
            return 0;
        size_t i = 0;

        // omite cualquier carácter que no sean dígitos
        while (i < bufsize && (buffer[i] < '0' || buffer[i] == '9'))
            ++i;
        if (i == bufsize)
            return 0;

        bool negative = false;
        bool found = false;
        bool decimal = false;
        T result = 0;
        T decimal_factor = 1;

        // verifica signo negativo
        if (buffer[i] == '-')
        {
            negative = true;
            ++i;
        }
        else if (buffer[i] == '+')
        {
            ++i;
        }

        // bucle que recorre el buffer en búsqueda de dígitos
        for (; i < bufsize; ++i)
        {
            char c = buffer[i];

            // verifica si el carácter es un dígito
            if (c >= '0' && c <= '9')
            {
                found = true;

                // verifica si el tipo especificado es de punto flotante
                if constexpr (std::is_floating_point<T>::value)
                {
                    // verifica si se ha llegado a la parte decimal
                    if (decimal)
                    {
                        // divide el valor del factor decimal entre 10
                        decimal_factor /= 10;
                        result += (c - '0') * decimal_factor;
                    }
                    else
                    {
                        // concatena el dígito en la parte derecha del número
                        result = (result * 10) + (c - '0');
                    }
                }
                // si es tipo entero
                else
                {
                    // verifica si se ha llegado a la parte decimal
                    if (decimal)
                    {
                        // si el tipo es entero y se encuentra un punto decimal, se
                        // termina el parseo
                        break;
                    }

                    // concatena el dígito en la parte derecha del número
                    result = (result * 10) + (c - '0');
                }
            }
            // verifica si el carácter es un punto y si el tipo especificado es de
            // punto flotante y si aun no se había llegado a la parte decimal
            else if (c == '.' && std::is_floating_point<T>::value && !decimal)
            {
                // permite solo un punto decimal para tipos flotantes
                decimal = true;
            }
            // de lo contario, si encuentra un carácter no numérico
            else
            {
                // termina el parseo
                break;
            }
        }

        // si no se encontró ningún dígito, retornar 0
        if (!found)
            return 0;
        return negative ? -result : result;
    }

    template <typename T>
    T to_number(const std::string &str, size_t pos = 0, size_t len = 0)
    {
        if (str.length() == 0 || pos >= str.length() || len > str.length())
            return 0;

        if (len == 0)
            len = str.length();
        bool negative = false;
        bool found = false;
        bool decimal = false;
        T result = 0;
        T decimal_factor = 1;

        // verifica signo negativo
        if (str[pos] == '-')
        {
            negative = true;
            ++pos;
        }
        else if (str[pos] == '+')
        {
            ++pos;
        }

        // bucle que recorre el buffer en búsqueda de dígitos
        for (; pos < str.length() && len > 0; ++pos)
        {
            char c = str[pos];

            // verifica si el carácter es un dígito
            if (c >= '0' && c <= '9')
            {
                found = true;
                len--;

                // verifica si el tipo especificado es de punto flotante
                if constexpr (std::is_floating_point<T>::value)
                {
                    // verifica si se ha llegado a la parte decimal
                    if (decimal)
                    {
                        // divide el valor del factor decimal entre 10
                        decimal_factor /= 10;
                        result += (c - '0') * decimal_factor;
                    }
                    else
                    {
                        // concatena el dígito en la parte derecha del número
                        result = (result * 10) + (c - '0');
                    }
                }
                // si es tipo entero
                else
                {
                    // verifica si se ha llegado a la parte decimal
                    if (decimal)
                    {
                        // si el tipo es entero y se encuentra un punto decimal, se
                        // termina el parseo
                        break;
                    }

                    // concatena el dígito en la parte derecha del número
                    result = (result * 10) + (c - '0');
                }
            }
            // verifica si el carácter es un punto y si el tipo especificado es de
            // punto flotante y si aun no se había llegado a la parte decimal
            else if (c == '.' && std::is_floating_point<T>::value && !decimal)
            {
                // permite solo un punto decimal para tipos flotantes
                decimal = true;
            }
            // de lo contario, si encuentra un carácter no numérico
            else
            {
                // termina el parseo
                break;
            }
        }

        // si no se encontró ningún dígito, retornar 0
        if (!found)
            return 0;
        return negative ? -result : result;
    }

    template <typename T>
    T to_number(const std::span<char> &buffer, size_t len = 0)
    {
        return to_number<T>(buffer.data(), buffer.size(), len);
    }

    /**
     * @brief Extrae el token N de una cadena delimitada por uno o más caracteres.
     * @param str Cadena de entrada.
     * @param token_idx Índice del token a extraer (comenzando en 0).
     * @param delimiters Caracteres delimitadores (ej: ",;" para coma o punto y coma).
     * @param keep_empty_tokens Si es true, considera tokens vacíos (por delimitadores 
     * consecutivos o al inicio/final).
     * @return true si se encontró el token, false en caso contrario.
     */
    bool extract_token(std::string &str, size_t token_idx, const char *delimiters, bool keep_empty_tokens = false);
    
    /**
     * @brief Extrae el token N de una cadena delimitada por uno o más caracteres.
     * @param str Cadena de entrada.
     * @param token_idx Índice del token a extraer (comenzando en 0).
     * @param delimiters Caracteres delimitadores (ej: ",;" para coma o punto y coma).
     * @param token_out Cadena de salida con el token extraído.
     * @param keep_empty_tokens Si es true, considera tokens vacíos (por delimitadores 
     * consecutivos o al inicio/final).
     * @return true si se encontró el token, false en caso contrario.
     */
    bool extract_token(const std::string &str, size_t token_idx, const char *delimiters, std::string &token_out, bool keep_empty_tokens = false);
    
    /**
     * @brief Elimina los caracteres indicados solo al inicio de la cadena.
     * @param str Cadena a modificar.
     * @param chars Caracteres a eliminar.
     */
    void ltrim(std::string &str, const char *chars = " \t\n\r\"");

    /**
     * @brief Elimina los caracteres indicados solo al final de la cadena.
     * @param str Cadena a modificar.
     * @param chars Caracteres a eliminar.
     */
    void rtrim(std::string &str, const char *chars = " \t\n\r\"");

    /**
     * @brief Elimina los caracteres indicados al inicio y final de la cadena.
     * @param str Cadena a modificar.
     * @param chars Caracteres a eliminar.
     */
    void trim(std::string &str, const char *chars = " \t\n\r\"");

    /**
     * @brief Elimina todos los caracteres que se encuentren justo antes de la
     * primera aparición de la secuencia indicada en una cadena.
     * 
     * @param str Cadena de entrada.
     * @param seq Secuencia de caracteres a buscar.
     * @param keep_seq Si es true, mantiene la secuencia en la cadena.
     */
    void remove_before(std::string &str, const char *seq, bool keep_seq = false);

    /**
     * @brief Elimina todos los caracteres que se encuentren justo después de la
     * primera aparición de la secuencia indicada en una cadena.
     * 
     * @param str Cadena de entrada.
     * @param seq Secuencia de caracteres a buscar.
     * @param keep_seq Si es true, mantiene la secuencia en la cadena.
     */
    void remove_after(std::string &str, const char *seq, bool keep_seq = false);

    /// @brief Convierte una marca de tiempo de GPS a Unix Epoch.
    /// @param gps_timestamp Marca de tiempo GPS.
    /// @return Marca de tiempo Epoch Unix.
    time_t gps_ts_to_epoch_ts(uint64_t gps_timestamp);
    
} // namespace axomotor::lte_modem

#include "services/axomotor_service.hpp"
#include "constants/secrets.hpp" 

#include <cstring>
#include <ArduinoJson.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_vfs_fat.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>

namespace axomotor::services {

static const char *TAG = "axomotor_service";

/**
 * Implementar:
 * 
 * - Endpoint para iniciar/terminar viaje
 *      Recibe: (id de viaje)
 *      Hace:
 *      - Habilita el servicio de sensores
 *      - Habilita la captura de fotografías
 *      - Habilita el registro de ubicación
 *      - Establece el identificador de viaje en configuración
 *      Envia: confirmación
 *      Nota:
 *      - Si el dispositivo sufre un reinicio, se deberá leer el Id guardado y 
 *      hacer como si lo haya recibido
 * 2. Endpoint para obtener listado de imagenes por periodo de tiempo
 *      Recibe: periodo de tiempo
 *      Envia: listado de imagenes en formato JSON
 * 
 * 3. Endpoint para obtener imagen
 */

/* Valores predeterminados */

const events::EventQueueSet AxoMotorService::queue_set = events::EventQueueSet();
const events::GlobalEventGroup AxoMotorService::event_group = events::GlobalEventGroup();

char AxoMotorService::s_current_trip_id[] = "";
char *AxoMotorService::s_buffer = nullptr;
bool AxoMotorService::s_is_initialized = false;
uint32_t AxoMotorService::s_trip_count = 0;
vprintf_like_t AxoMotorService::s_default_writer = NULL;
httpd_handle_t AxoMotorService::s_httpd_handle = nullptr;

/* Métodos públics */

void AxoMotorService::init()
{
    ESP_LOGI(TAG, "Initialization: Stage 1");
    assert(s_buffer = new char[constants::general::MAX_JSON_LENGTH]);

    // limita el nivel de registro de wifi a solo errores
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("wifi_init", ESP_LOG_ERROR);
    esp_log_level_set("esp_netif_lwip", ESP_LOG_ERROR);

    // inicializa la partición NVS
    esp_err_t err = nvs_flash_init();
    if (err) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    // verifica el resultado de la inicialización
    ESP_ERROR_CHECK(err);
    // crea el bucle de eventos predeterminado
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // inicia driver WiFi y servidor web
    init_wifi();
    init_httpd();

    // crea las instancias de los servicios
    auto mobile_service =  std::make_shared<MobileService>();
    auto sensor_service = std::make_shared<SensorService>();
    auto image_service = std::make_shared<ImageService>();
    auto panic_service = std::make_shared<PanicBtnService>();

    // iniciar el servicio del botón de pánico
    ESP_LOGI(TAG, "Starting Panic Button Service...");
    panic_service->start();

    // iniciar el servicio móvil
    ESP_LOGI(TAG, "Starting Mobile Service...");
    err = mobile_service->start();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Starting Image Service...");
        err = image_service->start(true);
    }
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Starting Sensor Service...");
        err = sensor_service->start(true);
    } else {
        ESP_LOGW(TAG, "Failed to start Image service");
        queue_set.device.send_to_back(events::event_code_t::CAMERA_FAILURE);
    }
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Loading SD storage...");
        err = init_sd();
    } else {
        ESP_LOGE(TAG, "Failed to start Sensor service...");
        queue_set.device.send_to_back(events::event_code_t::SENSOR_FAILURE);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load SD storage");
        queue_set.device.send_to_back(events::event_code_t::STORAGE_FAILURE);
    }
    
    uint32_t flags = event_group.wait_for_flags(MOBILE_SERVICE_STARTED_BIT, pdMS_TO_TICKS(35000));
    if ((flags & MOBILE_SERVICE_STARTED_BIT) == 0) {
        ESP_LOGE(TAG, "Failed to start Mobile Service");
        restart();
    }

    ESP_LOGI(TAG, "Initialization: Stage 2");

    // espera a que se sincronice la hora
    flags = TIME_SYNC_COMPLETED_BIT | TIME_SYNC_FAILED_BIT;
    flags = event_group.wait_for_flags(flags, false, false, pdMS_TO_TICKS(5000));
    if ((flags & TIME_SYNC_FAILED_BIT)) {
        restart();
    }

    // lanzar evento de inicio
    queue_set.device.send_to_back(events::event_code_t::DEVICE_RESET);

    // le el identificador del viaje actual (si lo hay)
    err = read_trip_id();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "A trip_id value was found");
        event_group.set_flags(TRIP_ACTIVE_BIT);
    } else {
        ESP_LOGI(TAG, "Waiting for new trip_id...");
    }

    while (true) 
    {
        uint32_t free_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        uint32_t max_heap_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        uint32_t heap_total_size = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);

        ESP_LOGI(TAG, "HEAP: free=%lu, max_block=%lu, total=%lu", 
            free_heap, max_heap_block, heap_total_size);
        
        vTaskDelay(pdMS_TO_TICKS(600000));
    }
}

const char *AxoMotorService::get_current_trip_id()
{
    return s_current_trip_id;
}

uint32_t AxoMotorService::get_trip_count()
{
    return s_trip_count;
}

void AxoMotorService::init_wifi()
{
    ESP_LOGI(TAG, "Initializing WiFi driver...");

    // inicializa la pila TCP/IP y la interfaz wifi
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();

    // obtiene la configuración de wifi predeterminada 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    // aplica la configuración tomada de la librería WiFi de Arduino para 
    // reducir el consumo de RAM por parte del controlador
    //cfg.static_tx_buf_num = 0;
    //cfg.dynamic_tx_buf_num = 32;
    //cfg.tx_buf_type = 1;
    //cfg.cache_tx_buf_num = 4;  // can't be zero!
    //cfg.static_rx_buf_num = 4;
    //cfg.dynamic_rx_buf_num = 32;

    // inicializa el controlador
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // registra el manejador de eventos de wifi
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            on_wifi_event,
            NULL,
            NULL));

    // registra el manejador de eventos de la red
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            on_wifi_event,
            NULL,
            NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t config{};
    strncpy((char *)config.sta.ssid, WIFI_SSID, sizeof(config.sta.ssid));
    strncpy((char *)config.sta.password, WIFI_PASSWORD, sizeof(config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void AxoMotorService::init_httpd()
{
    ESP_LOGI(TAG, "Initializing HTTPD Server...");
    
    /* uri de index */
    static const httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = on_index_req,
        .user_ctx = NULL
    };

    /* uri para iniciar viaje */
    static const httpd_uri_t start_trip_uri = {
        .uri = "/start",
        .method = HTTP_PUT,
        .handler = on_start_trip_req,
        .user_ctx = NULL
    };

    /* uri para detener viaje */
    static const httpd_uri_t stop_trip_uri = {
        .uri = "/stop",
        .method = HTTP_PUT,
        .handler = on_stop_trip_req,
        .user_ctx = NULL
    };

    /* uri para obtener lista de imagenes */
    static const httpd_uri_t list_images_uri = {
        .uri = "/list",
        .method = HTTP_GET,
        .handler = on_list_images_req,
        .user_ctx = NULL
    };

    /* uri para obtener imagen */
    static const httpd_uri_t get_image_uri = {
        .uri = "/image/*",
        .method = HTTP_GET,
        .handler = on_get_image_req,
        .user_ctx = NULL
    };

    // inicia y configura el servidor
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = constants::general::HTTPD_STACK_SIZE;
    ESP_ERROR_CHECK(httpd_start(&s_httpd_handle, &config));

    httpd_register_uri_handler(s_httpd_handle, &index_uri);
    httpd_register_uri_handler(s_httpd_handle, &start_trip_uri);
    httpd_register_uri_handler(s_httpd_handle, &stop_trip_uri);
    httpd_register_uri_handler(s_httpd_handle, &list_images_uri);
    httpd_register_uri_handler(s_httpd_handle, &get_image_uri);
}

esp_err_t AxoMotorService::init_sd()
{
    esp_err_t err;

    // opciones de montaje del sistema de archivos
    esp_vfs_fat_sdmmc_mount_config_t mount_config{};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;
    
    sdmmc_card_t *card;
    const char mount_point[] = SD_MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // opciones para el host sdmmc
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    ESP_LOGI(TAG, "Using SDMMC peripheral");

    // opciones para el slot
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk = (gpio_num_t)39;
    slot_config.cmd = (gpio_num_t)38;
    slot_config.d0 = (gpio_num_t)40;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "Mounting filesystem");
    err = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s)", esp_err_to_name(err));
        }

        return err;
    }

    ESP_LOGI(TAG, "Filesystem mounted");
    AxoMotor::event_group.set_flags(SD_LOADED_BIT);

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    return err;
}

esp_err_t AxoMotorService::read_trip_id()
{
    esp_err_t err;
    nvs_handle_t handle;
    size_t length = sizeof(s_current_trip_id);

    // abre una instancia de NVS
    err = nvs_open("axomotor", NVS_READONLY, &handle);
    if (err == ESP_OK) {
        err = nvs_get_str(handle, "trip_id", s_current_trip_id, &length);
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Current trip_id: %s", s_current_trip_id);
        nvs_get_u32(handle, "count", &s_trip_count);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No trip_id value is set");
    } else {
        ESP_LOGI(
            TAG, 
            "Failed to read trip_id (%s)",
            esp_err_to_name(err)
        );
    }
    
    // cierra la instancia de NVS
    nvs_close(handle);

    return err;
}

esp_err_t AxoMotorService::write_trip_id()
{
    esp_err_t err;
    nvs_handle_t handle;

    // abre una instancia de NVS
    err = nvs_open("axomotor", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        // escribe la cadena
        err = nvs_set_str(handle, "trip_id", s_current_trip_id);
    }

    if (err == ESP_OK) {
        err = nvs_get_u32(handle, "count", &s_trip_count);
        s_trip_count++;
    }
    
    if (err == ESP_OK) {
        err = nvs_set_u32(handle, "count", s_trip_count);
    }

    if (err == ESP_OK) {
        // confirma los cambios
        err = nvs_commit(handle);
    }

    if (err == ESP_OK) {
        event_group.set_flags(TRIP_ACTIVE_BIT);
    } else {
        ESP_LOGI(
            TAG, 
            "Failed to write trip_id (%s)",
            esp_err_to_name(err)
        );
    }

    // cierra la instancia de NVS
    nvs_close(handle);

    return err;
}

esp_err_t AxoMotorService::delete_trip_id()
{
    esp_err_t err;
    nvs_handle_t handle;

    // abre una instancia de NVS
    err = nvs_open("axomotor", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_erase_key(handle, "trip_id");
    }

    if (err == ESP_OK) {
        // confirma los cambios
        err = nvs_commit(handle);
    }

    if (err == ESP_OK) {
        AxoMotor::event_group.clear_flags(TRIP_ACTIVE_BIT);
    } else {
        ESP_LOGI(
            TAG, 
            "Failed to erase trip_id (%s)",
            esp_err_to_name(err)
        );
    }

    // cierra la instancia de NVS
    nvs_close(handle);
    // borra el identificador de viaje
    memset(s_current_trip_id, 0, sizeof(s_current_trip_id));

    return err;
}

esp_err_t AxoMotorService::send_response(httpd_req_t *req, bool success, const char *message) 
{
    // crea un objeto json para colocar valores
    JsonDocument doc;
    // longitud del archivo json a enviar
    size_t json_length;
    esp_err_t err;
    
    // establece el resultado de la operación
    doc["success"] = success;
    doc["timestamp"] = events::get_timestamp();
    // verifica si el resultado de la operación no fue exitoso
    if (!success && message != NULL) {
        // establece el mensaje de error
        doc["message"] = message;
    }

    // convierte el objeto json a texto
    json_length = serializeJson(doc, s_buffer, constants::general::MAX_JSON_LENGTH);
    
    // establece el tipo de respuesta
    err = httpd_resp_set_type(req, "application/json");
    if (err == ESP_OK) {
        // envía el json
        err = httpd_resp_send(req, s_buffer, json_length);
    }

    return err;
}

int64_t AxoMotorService::get_query_param_i64(httpd_req_t *req, const char *key, int64_t def)
{
    char param[32];
    
    if (httpd_req_get_url_query_len(req) == 0) return def;
    if (httpd_req_get_url_query_str(req, param, sizeof(param)) != ESP_OK) return def;

    char val_str[32];
    if (httpd_query_key_value(param, key, val_str, sizeof(val_str)) == ESP_OK) {
        return atoll(val_str);
    }

    return def;
}

void AxoMotorService::enable_log_to_file()
{
    s_default_writer = esp_log_set_vprintf(on_write_log);
}

void AxoMotorService::disable_log_to_file()
{
    esp_log_set_vprintf(s_default_writer);
}

void AxoMotorService::restart()
{
    uint8_t seconds = 3;

    do {
        ESP_LOGW(TAG, "Restarting in %u seconds...", seconds);
        seconds--;
    } while (seconds != 0);
    
    esp_restart();
}

esp_err_t AxoMotorService::on_index_req(httpd_req_t *req)
{
    return send_response(req, true);
}

esp_err_t AxoMotorService::on_start_trip_req(httpd_req_t *req)
{
    ESP_LOGI(TAG, "On start trip request handler");

    char query_str[33];
    char new_trip_id[sizeof(s_current_trip_id)];
    esp_err_t err;
    
    // verifica si no hay un viaje activo
    if (!event_group.is_trip_active()) {
        // obtiene la cadena de consulta
        err = httpd_req_get_url_query_str(req, query_str, sizeof(query_str));
        // verifica si se encontró una consulta
        if (err == ESP_OK) {
            // extrae el identificador de viaje
            err = httpd_query_key_value(
                query_str, 
                "tripId", 
                new_trip_id, 
                sizeof(new_trip_id)
            );
        }

        // verifica si se pudo extraer el identificador de viaje y si este tiene
        // la longitud esperada
        if (err == ESP_OK && strlen(new_trip_id) == constants::general::TRIP_ID_LENGTH) {
            // copia el identificador recibido
            strlcpy(s_current_trip_id, new_trip_id, sizeof(new_trip_id));
            // guarda el identificador
            write_trip_id();
            err = send_response(req, true, NULL);
        } else {
            err = send_response(req, false, "Invalid query parameters");
        }
    } else {
        err = send_response(req, false, "Another trip_id has been set");
    }

    return err;
}

esp_err_t AxoMotorService::on_stop_trip_req(httpd_req_t *req)
{
    ESP_LOGI(TAG, "On stop trip request handler");
    char query_str[33];
    char trip_id[sizeof(s_current_trip_id)];
    esp_err_t err;
    
    // verifica si hay un viaje activo
    if (event_group.is_trip_active()) {
        // obtiene la cadena de consulta
        err = httpd_req_get_url_query_str(req, query_str, sizeof(query_str));
        // verifica si se encontró una consulta
        if (err == ESP_OK) {
            
            // extrae el identificador de viaje
            err = httpd_query_key_value(
                query_str, 
                "tripId", 
                trip_id, 
                sizeof(trip_id)
            );
        }

        // verifica si se pudo extraer el identificador de viaje y si este coincide con
        // el establecido
        if (err == ESP_OK && strncmp(trip_id, s_current_trip_id, constants::general::TRIP_ID_LENGTH) == 0) {
            // borra el identificador actual            
            delete_trip_id();
            err = send_response(req, true, NULL);
        } else {
            err = send_response(req, false, "Invalid query parameters");
        }
    } else {
        err = send_response(req, false, "No trip_id has been set");
    }

    return err;
}

esp_err_t AxoMotorService::on_list_images_req(httpd_req_t *req)
{
    ESP_LOGI(TAG, "On list images request handler");

    time_t from = get_query_param_i64(req, "from", 0);
    time_t to = get_query_param_i64(req, "to", 0x7FFFFFFF);
    uint64_t offset = get_query_param_i64(req, "offset", 0);
    uint64_t limit = get_query_param_i64(req, "limit", 10);
    char dirpath[16];

    // establece la ruta del directorio
    snprintf(
        dirpath, 
        sizeof(dirpath), 
        SD_MOUNT_POINT "/%lu", 
        get_trip_count()
    );
    
    // abre el directorio
    DIR *dir = opendir(dirpath);
    if (!dir) {
        ESP_LOGE(TAG, "Could not open directory: '%s'", dirpath);
        return send_response(req, false, "Could not open directory");
    }

    JsonDocument doc;
    JsonArray files = doc.to<JsonArray>();
    
    char filepath[272];
    struct dirent *entry;
    struct stat st;
    int match_count = 0;

    // bucle que recorre todas las entradas en un directorio
    while ((entry = readdir(dir)) != NULL) 
    {
        // verifica si la entrada corresponde a un directorio
        if (entry->d_type != DT_REG) continue;

        // establece la ruta completa del archivo
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);
        // verifica si el archivo existe
        if (stat(filepath, &st) != 0) continue;

        // obtiene la fecha de creación del archivo
        time_t file_time = st.st_mtime;
        // verifica si la fecha se encuentra dentro del periodo indicado
        if (file_time >= from && file_time <= to) {
            // verifica si el archivo se encuentra dentro de los límites de paginación
            if (match_count >= offset && match_count < (offset + limit)) {
                // añade un objeto al arreglo de respuesta
                JsonObject f = files.add<JsonObject>();
                f["name"] = entry->d_name;
                f["size"] = (int)st.st_size;
                f["created"] = (long)file_time;  // Puedes dejarlo como timestamp Unix
            }

            match_count++;
        }
    }

    // cierra el directorio
    closedir(dir);

    size_t len = serializeJson(doc, s_buffer, constants::general::MAX_JSON_LENGTH);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, s_buffer, len);
    return ESP_OK;
}

esp_err_t AxoMotorService::on_get_image_req(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Get image");

    char filepath[272];
    char filename[128];

    // Extraer el nombre del archivo desde la URI
    const char *uri = req->uri;
    const char *base = "/image/";
    
    // compara la base de la uri con la predeterminada
    if (strncmp(uri, base, strlen(base)) != 0) {
        send_response(req, false, "Invalid URI format");
        return ESP_FAIL;
    }
    
    // obtiene un puntero al nombre del archivo
    const char *requested_file = uri + strlen(base);
    // verifica si se especificó un nombre de archivo demasiado largo
    if (strlen(requested_file) >= sizeof(filename)) {
        send_response(req, false, "File name too long");
        return ESP_FAIL;
    }

    // establece la ruta completa del archivo
    snprintf(
        filepath, 
        sizeof(filepath), 
        SD_MOUNT_POINT "/%lu/%s", 
        get_trip_count(),
        requested_file
    );

    // establece el nombre del archivo
    snprintf(filename, sizeof(filename), "%s", requested_file);

    // verifica si el archivo existe
    struct stat st;
    if (stat(filepath, &st) != 0) {
        ESP_LOGW(TAG, "File not found: '%s'", filepath);
        send_response(req, false, "File not found");
        return ESP_FAIL;
    }

    // abre el archivo
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Error opening file: '%s'", filepath);
        send_response(req, false, "Cannot open file");
        return ESP_FAIL;
    }

    // establece Content-Type
    httpd_resp_set_type(req, "image/jpeg");

    // lee y envia en chunks
    char chunk[constants::general::FILE_CHUNK_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (httpd_resp_send_chunk(req, chunk, bytes_read) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send file chunk");
            fclose(f);
            // finaliza la respuesta aunque haya fallado
            httpd_resp_sendstr_chunk(req, NULL);  
            
            return ESP_FAIL;
        }
    }

    fclose(f);
    // finaliza la respuesta correctamente
    httpd_resp_sendstr_chunk(req, NULL); 
    return ESP_OK;
}

void AxoMotorService::on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT)
    {
        switch (id)
        {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Connecting to AP...");
                ESP_ERROR_CHECK(esp_wifi_connect());
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Retrying to connect to the AP...");
                ESP_ERROR_CHECK(esp_wifi_connect());
                break;
            default:
                break;
        }
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "Connected to AP");
    }
}

int AxoMotorService::on_write_log(const char *str, va_list args)
{
    s_default_writer(str, args);

    // abre el archivo de log en modo append
    FILE *f = fopen(SD_MOUNT_POINT "/log.txt", "a");
    if (!f) {
        // si no se puede abrir, retornar sin escribir
        return 0;
    }

    // escribe el mensaje formateado
    int ret = vfprintf(f, str, args);

    // cierra el archivo
    fclose(f);
    return ret;
}

}

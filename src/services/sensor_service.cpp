#include "services/sensor_service.hpp"
#include "services/axomotor_service.hpp"
#include "constants/hw.hpp"
#include "constants/sensor.hpp"

#include <esp_log.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <math.h>

namespace axomotor::services {

using namespace axomotor::constants::hw::sensor;
using namespace axomotor::events;

constexpr static const char *TAG = "sensor_service";

SensorService::SensorService() :
    ServiceBase{TAG, 4 * 1024, 4, 1},
    m_bias_ax{0},
    m_bias_ay{0},
    m_bias_az{0},
    m_bias_gz{0},
    m_filtered_ax{0},
    m_filtered_ay{0},
    m_filtered_az{0},
    m_prev_ax{0},
    m_velocity{0},
    m_stationary_timer{0},
    m_accel_count{0},
    m_brake_count{0},
    m_curve_count{0},
    m_impact_count{0},
    m_delay{pdMS_TO_TICKS(1000 / FS)},
    m_last_event{event_code_t::NONE},
    m_last_event_ts{0}
{ 
    i2c_config_t config{};
    config.mode = I2C_MODE_MASTER;
    config.sda_io_num = PIN_SDA;
    config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    config.scl_io_num = PIN_SCL;
    config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    config.master.clk_speed = I2C_FREQ_HZ;
    config.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    // configura el controladr I2C (reinicia si falla)
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &config));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, config.mode, 0, 0, 0));

    // configura el pin del sensor de vibración
    gpio_reset_pin(PIN_VIBRATION_SENSOR);
    ESP_ERROR_CHECK(gpio_set_direction(PIN_VIBRATION_SENSOR, GPIO_MODE_INPUT));
}

esp_err_t SensorService::setup()
{
    esp_err_t err;

    // crea la instancia del sensor
    m_mpu6050 = mpu6050_create(I2C_PORT, MPU6050_I2C_ADDRESS);
    err = configure_sensor();

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "MPU6050 initialized");

        // calibra el sensor tomando 500 muestras
        calibrate_biases(500);
    } else {
        ESP_LOGE(TAG, "Failed to initialize MPU6050");
    }

    return err;
}

void SensorService::loop()
{
    esp_err_t err;
    mpu6050_acce_value_t acce{};
    mpu6050_gyro_value_t gyro{};

    err = mpu6050_get_acce(m_mpu6050, &acce);
    if (err == ESP_OK) {
        err = mpu6050_get_gyro(m_mpu6050, &gyro);
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read accelerometer measurements");
        vTaskDelay(pdMS_TO_TICKS(100));
        return;
    }

    bool vib_state = gpio_get_level(PIN_VIBRATION_SENSOR) == 1;

    // filtro y compensación de bias
    float ax = ALPHA * m_filtered_ax + (1 - ALPHA) * (acce.acce_x - m_bias_ax);
    float ay = ALPHA * m_filtered_ay + (1 - ALPHA) * (acce.acce_y - m_bias_ay);
    float az = ALPHA * m_filtered_az + (1 - ALPHA) * (acce.acce_z - m_bias_az);
    m_filtered_ax = ax; 
    m_filtered_ay = ay; 
    m_filtered_az = az;

    // calcula la magnitud total de aceleración
    float a_total = sqrt(ax * ax + ay * ay + az * az);
    float a_long = ax;
    float a_lat = ay;
    float yaw_rate = gyro.gyro_z - m_bias_gz;
    // calcula la derivada para distinguir entre una frenada progresiva de una
    // brutal
    float jerk = (ax - m_prev_ax) / DT;
    m_prev_ax = ax;

    // detección de impacto
    if (a_total > ACC_IMPACT_THR && vib_state) {
        m_impact_count++;
        if (m_impact_count > IMPACT_CONFIRM_COUNT) {
            ESP_LOGW(TAG, "Impact detected (a=%.6f)", a_total);
            report(event_code_t::IMPACT_DETECTED);
            m_impact_count = 0;
        }
    } else {
        m_impact_count = 0;
    }

    // detección de frenado fuerte
    if (a_long < BRAKE_THR) {
        m_brake_count++;
        if (m_brake_count > BRAKE_CONFIRM_COUNT)  {
            if (jerk < -JERK_THR) {
                ESP_LOGW(TAG, "Harsh braking (a_long=%.6f, jerk=%.6f)", a_long, jerk);
                report(event_code_t::HARSH_BRAKING);
            } else {
                ESP_LOGW(TAG, "Hard braking (a_long=%.6f)", a_long);
            }
            
            m_brake_count = 0;
        }
    }

    // detección de aceleración brusca
    if (a_long > ACCELERATION_THR) {
        m_accel_count++;
        if (m_accel_count > IMPACT_CONFIRM_COUNT) {
            if (jerk > JERK_THR) {
                ESP_LOGW(TAG, "Harsh acceleration (a_long=%.6f, jerk=%.6f)", a_long, jerk);
                report(event_code_t::HARSH_ACCELERATION);
            } else {
                ESP_LOGW(TAG, "Hard acceleration (a_long=%.6f)", a_long);
            }

            m_accel_count = 0;
        }
    } else {
        m_accel_count = 0;
    }

    // detección de giro brusco
    if (fabsf(a_lat) > CURVE_THR || fabsf(yaw_rate) > YAW_THR) {
        m_curve_count++;
        if (m_curve_count > ACCEL_CONFIRM_COUNT) {
            ESP_LOGW(TAG, "Harsh cornering detected (a_lat=%.6f, yaw_rate=%.6f)", a_lat, yaw_rate);
            report(event_code_t::HARSH_CORNERING);
            m_curve_count = 0;
        }
    } else {
        m_curve_count = 0;
    }

    vTaskDelay(m_delay);
}

void SensorService::finish()
{
    mpu6050_delete(m_mpu6050);
    i2c_driver_delete(I2C_PORT);
    m_mpu6050 = nullptr;
}

esp_err_t SensorService::configure_sensor()
{
    esp_err_t err;
    int attempt_num = 5;

    do {
        // configura el MPU6050
        err = mpu6050_config(m_mpu6050, ACCE_FS_8G, GYRO_FS_500DPS);
        if (err == ESP_OK) {
            // despierta el MPU6050
            err = mpu6050_wake_up(m_mpu6050);
            if (err == ESP_OK) {
                // termina el bucle
                break;
            }
        } 
        
        ESP_LOGW(TAG, "Attempting to configure MPU6050...");
        vTaskDelay(pdMS_TO_TICKS(250));
    } while (attempt_num != 0);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure MPU6050");
    }

    return err;
}

void SensorService::calibrate_biases(int samples_num)
{
    if (samples_num == 0) return;
    ESP_LOGI(TAG, "Calibrating sensors...");

    mpu6050_acce_value_t acce{};
    mpu6050_gyro_value_t gyro{};
    float sum_ax = 0, sum_ay = 0, sum_az = 0;
    float sum_gz = 0;
    int n = samples_num;

    do
    {
        mpu6050_get_acce(m_mpu6050, &acce);
        mpu6050_get_gyro(m_mpu6050, &gyro);
        
        sum_ax += acce.acce_x;
        sum_ay += acce.acce_y;
        sum_az += acce.acce_z;
        sum_gz += gyro.gyro_z;

        vTaskDelay(pdMS_TO_TICKS(10));
        n--;
    } 
    while (n != 0);

    m_bias_ax = sum_ax / samples_num;
    m_bias_ay = sum_ay / samples_num;
    m_bias_az = sum_az / samples_num;
    m_bias_gz = sum_gz / samples_num;

    ESP_LOGI(
        TAG, 
        "Bias values: ax=%.3f, ay=%.3f, az=%.3f, gz=%.3f",
        m_bias_ax, m_bias_ay, m_bias_az, m_bias_gz);

    ESP_LOGI(TAG, "Calibration completed");
}

void SensorService::report(events::event_code_t code)
{
    // verifica si el sistema no está listo
    if (!AxoMotor::event_group.is_system_ready()) return;

    TickType_t timestamp = xTaskGetTickCount();
    bool send_event = (code != m_last_event) || 
        (timestamp - m_last_event_ts >= pdMS_TO_TICKS(LAST_EVENT_DELAY));
    
    if (send_event) {
        AxoMotor::queue_set.device.send_to_back(code, 0);
        m_last_event = code;
        m_last_event_ts = timestamp;
    }
}

} // namespace axomotor::services

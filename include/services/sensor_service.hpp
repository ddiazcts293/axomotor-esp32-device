#pragma once

#include <service_base.hpp>
#include <mpu6050.h>

namespace axomotor::services {

class SensorService : public threading::ServiceBase
{
public:    
    SensorService();

private:
    mpu6050_handle_t m_mpu6050;
    float m_bias_ax;
    float m_bias_ay;
    float m_bias_az;
    float m_bias_gz;
    float m_filtered_ax;
    float m_filtered_ay;
    float m_filtered_az;
    float m_prev_ax;
    float m_velocity;
    float m_stationary_timer;
    int m_accel_count;
    int m_brake_count;
    int m_curve_count;
    int m_impact_count;
    TickType_t m_delay;

    esp_err_t setup() override;
    void loop() override;
    void finish() override;

    esp_err_t configure_sensor();
    void calibrate_biases(int samples_num);
};

} // namespace axomotor::services

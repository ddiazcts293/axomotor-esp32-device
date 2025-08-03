#pragma once
    
#define G                   9.80665 // Aceleración de gravedad (m/s^2)
#define ACC_IMPACT_THR      2.0f    // Umbral de límite inferior de impacto (g)
#define ACCELERATION_THR    0.4f    // Umbral de aceleración (g)
#define BRAKE_THR           -0.4f   // Umbral de frenado fuerte (g)
#define CURVE_THR           0.35f   // Umbral de giro lateral fuerte (g)
#define YAW_THR             25.0f   // Umbral de velocidad angular agresiva (°/s)
#define JERK_THR            0.8f    // Umbral de salto abrupto en aceleración (g/s)
#define ALPHA               0.95f   // Filtro pasabajas para suaviar la señal
#define FS                  100.0f  // Frecuencia mínima de muestreo (Hz)
#define DT                  (1.0f / FS)
#define CURVE_CONFIRM_COUNT 10
#define BRAKE_CONFIRM_COUNT 5
#define IMPACT_CONFIRM_COUNT 5
#define ACCEL_CONFIRM_COUNT 5

#define LAST_EVENT_DELAY    2000

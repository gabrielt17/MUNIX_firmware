#include "Controller.h"
#include "Arduino.h"

// Class constructor
Controller::Controller(float KP, float KD, float KI) 
: kp(KP), kd(KD), ki(KI) {
}

// Calculates the control signal to the given parameters
float Controller::getControlSignal(float TARGET, float RPMMEASUREMENT) {

    // Time in which the error was registered
    unsigned long currentMicros = micros();

    // Error and time intervals calculations
    float deltaMicros = static_cast<float>((currentMicros-previousMicros))/(1e6);
    float error = TARGET - RPMMEASUREMENT;
    float deltaError = error-previousError;
    float deltaRpmMeasurement = RPMMEASUREMENT - previousRpmMeasurement;
    if (fabs(error) < 5) {
        error = 0;
    } 

    // Time in which the previous error was registered
    previousMicros = currentMicros;

    // Proporcional instance
    float r = error*Controller::kp;

    // Derivative instance
    float d = (-deltaRpmMeasurement / deltaMicros) * Controller::kd;

    // Integrative instance
    this->i += error*Controller::ki*deltaMicros;

    // Limita o termo integral (anti-windup)
    const float integral_limit = 400.0; // Valor experimental, ajuste conforme necessário
    if (this->i > integral_limit) {
        this->i = integral_limit;
    } else if (this->i < -integral_limit) {
        this->i = -integral_limit;
    }

    Serial.printf("Error: %f, RPM: %f, Control Signal: %f\n", error, RPMMEASUREMENT, r + d + i);
    // Control signal equation
    float u = r + d + i;

    Controller::previousError = error;
    Controller::previousRpmMeasurement = RPMMEASUREMENT;
    
    return u;
}

// Converts the given control signal into a 10-bit PWM value
float Controller::convertToPWM(float VALUE) {
    
    float pwm = 0.145*VALUE+76.843;
    if (pwm > 1023) {
        pwm = 1023;
    }
    else if (pwm < -1023)
    {
        pwm = -1023;
    }

    return pwm;
}

/** @brief
 * Combines the previous methods into one. Accumulates the control singal.
 *  
 */ 
float Controller::controlMotor(float TARGET, float RPMMEASUREMENT) {
    
    float controlSignal = getControlSignal(TARGET, RPMMEASUREMENT);

    float pwm_value = convertToPWM(controlSignal);

    return pwm_value;
}

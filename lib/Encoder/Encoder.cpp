#include "Encoder.h"
#include <Arduino.h>

/** @brief Construtor que leva em consideração a existência de um pino
 *de alimentação.
 */
Encoder::Encoder(const uint8_t &ENCODERPIN, const uint8_t &PULSESPEROTATION, const uint8_t &POWERPIN)
    : encoderPin(ENCODERPIN), pulsesPerRotation(PULSESPEROTATION), powerPin(POWERPIN), pulses(0), rpm(0), previousMicros(0)
{
    Encoder::setupArduino(true, Encoder::powerPin);
}

// Construtor sem pino de alimentação
Encoder::Encoder(const uint8_t &ENCODERPIN, const uint8_t &PULSESPEROTATION)
    : encoderPin(ENCODERPIN), powerPin(0), pulses(0), rpm(0), previousMicros(0), pulsesPerRotation(PULSESPEROTATION)
{
    Encoder::setupArduino(false, 0);
}

// Configuração do Arduino
void Encoder::setupArduino(bool POWER, uint8_t POWERPIN)
{
    pinMode(Encoder::encoderPin, INPUT);
    if (POWER)
    {
        pinMode(POWERPIN, OUTPUT);
        digitalWrite(POWERPIN, HIGH);
    }
}

// Calcula o RPM
void Encoder::calculateRPM(long currentPulses, unsigned long sampleTime)
{  

    if (sampleTime == 0) { // Evitar divisão por zero
        this->rpm = 0.0f;
        Encoder::isCalculated = true;
        return;
    }

    this->rpm = (static_cast<float>(currentPulses) / pulsesPerRotation) * (1000000.0f / static_cast<float>(sampleTime)) * 60.0f;
    Encoder::isCalculated = true;
}

// Retorna o RPM
float Encoder::getRPM(bool ISCLOCKWISE)
{

    if (ISCLOCKWISE)
    {
        return -Encoder::rpm;
    } else {
        return Encoder::rpm;
    }
    Encoder::isCalculated = false;
}
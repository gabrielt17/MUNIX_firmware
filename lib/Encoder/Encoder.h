#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

/**
 * @param ENCODERPIN Encoder signal pin
 * @param PULSESPEROTATION Number of pulses on a single spin
 * @param POWERPIN Encoder power pin, in case there is one. Leave blank if none
 * @brief Disclaimer: You have to manually create the ISR functions and attach/detach them
 */
class Encoder {
    private:

        const uint8_t powerPin;
        uint pulsesPerRotation;
        float rpm;
        
        const uint8_t encoderPin;
        void setupArduino(bool POWER, uint8_t POWERPIN);       

    public:

        unsigned long previousMicros = 0;
        bool isCalculated;
        
        Encoder(const uint8_t& ENCODERPIN, const uint8_t& PULSESPEROTATION, const uint8_t& POWERPIN);
        Encoder(const uint8_t& ENCODERPIN, const uint8_t& PULSESPEROTATION);
        void calculateRPM(long currentPulses, unsigned long sampleTime);
        float getRPM(bool ISCLOCKWISE);
        volatile uint pulses;
};

#endif // ENCODER_H

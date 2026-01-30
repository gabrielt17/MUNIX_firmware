#ifndef KINEMATICS
#define KINEMATICS

#include "Kinematics.h"

// Funções auxiliares (podem ficar aqui se não forem usadas em outro lugar)
static float convertAngularToRPM(float VALUE) {
	return (30 * VALUE) / (M_PI);
}

// Função principal de conversão
motorRPM convertMessage(const geometry_msgs::Twist &MSG) {
	float u = MSG.linear.x;
	float w = MSG.angular.z;
	// Cinemática inversa para um robô diferencial
	float rRADS = 25.641 * u + 2.077 * w;
	float lRADS = 25.641 * u - 2.077 * w;
	float rRPM = convertAngularToRPM(-rRADS); // O sinal negativo pode ser para corrigir a montagem do motor
	float lRPM = convertAngularToRPM(lRADS);
	return {lRPM, rRPM};
}

float convertPulsesToRPM(float pulses, float PulsesPerRotation, float SampleTime) {
	
	if (SampleTime == 0) { // Evitar divisão por zero
        return 0;
    }

    float rpm = (static_cast<float>(pulses) / PulsesPerRotation) * (1000000.0f / static_cast<float>(SampleTime)) * 60.0f;
	
	return rpm;
}

#endif // KINEMATICS
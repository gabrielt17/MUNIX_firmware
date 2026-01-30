#ifndef KINEMATICS_H
#define KINEMATICS_H

#include <geometry_msgs/Twist.h>
#include "Arduino.h" // Necessário para M_PI
#include <ros.h>

// Estrutura para armazenar o RPM de cada motor
struct motorRPM {
	float lrpm;
	float rrpm;
};


// Declarações das funções
motorRPM convertMessage(const geometry_msgs::Twist &MSG);
float convertPulsesToRPM(float pulses, float PulsesPerRotation, float SampleTime);

#endif // KINEMATICS_H
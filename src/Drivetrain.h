#include <Arduino.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <stdlib.h>
#include <cstring>

using ll = long long;
using ld = long double;

using namespace std;

// ! Micros for time, millimeters for distance for accuracy


// define pins
const int button = 2;
const int en2 = 3;
const int step2 = 4;
const int dir2 = 5;
const int step1 = 6;
const int dir1 = 7;
const int en1 = 8;
const int ms1_1 = 9;
const int ms1_2 = 10;
const int ms2_2 = 11;
const int ms2_1 = 12;
const int analog_pin = A0;
const float R1 = 10000;
const float R2 = 10000;


// minimum gap between pulses that doesn't exceed torque limit
ll min_gap_between_pulses = 100; 

ld dist_between_wheels = 201.5; // distance between the two wheels in millimeters

ld micro_stepping = 16; // amount of microstepping

ld wheel_circumference = 84 * M_PI; // 84 mm diameter, circumference in millimeters

// Steps needed to complete a revolution with microstepping
ld steps_per_revolution = 200 * micro_stepping;


// delays between each step during the acceleration in micros
vector<ll> turning_speeds = {min_gap_between_pulses + 350, min_gap_between_pulses + 250, min_gap_between_pulses + 100, min_gap_between_pulses + 50, min_gap_between_pulses + 50, min_gap_between_pulses + 100, min_gap_between_pulses + 250, min_gap_between_pulses + 350}; 

// delay between each step during the acceleration/deceleration in micros
vector<ll> acc_speeds = {min_gap_between_pulses + 250, min_gap_between_pulses + 150, min_gap_between_pulses + 50, min_gap_between_pulses}; 

// Steps per phase in accel/decel for straight
ll straight_steps_per_acceleration_phase = 250; 

const ld steps_to_go_during_turn = dist_between_wheels * M_PI / 4 * steps_per_revolution / wheel_circumference + 17; 

// steps per acceleration/deceleration phase for turning
ll turning_steps_per_phase = round(steps_to_go_during_turn / 8); 

long double const_delay_time = 65000; // Extra time offset that I don't know where it comes from



// Struct that stores the information needed for each step of the run
struct Instruction
{
    bool is_straight;
    bool forward_or_left;
    ld steps;
};

// global pulses so I can adjust the motor turn ratio
long long pulses = 0;

// If I should move the left motor less or the right motor less, true if right motor should move less
bool adjust_right = true;

// Skip pulse every x pulses
long long rate = 700;

void set_direction(bool forward, bool turning, bool left)
{
    /*
    Sets the direction of the motors
    */
    if (turning)
    {
        if (left)
        {
            digitalWrite(dir1, HIGH);
            digitalWrite(dir2, LOW);
        }
        else
        {
            digitalWrite(dir1, LOW);
            digitalWrite(dir2, HIGH);
        }
    }
    else
    {
        if (forward)
        {
            digitalWrite(dir1, LOW);
            digitalWrite(dir2, LOW);
        }
        else
        {
            digitalWrite(dir1, HIGH);
            digitalWrite(dir2, HIGH);
        }
    }
}

ll get_time_per_turn()
{
    ll sum = 0;
    for (int i = 0; i < 8; i++)
    {
        sum += turning_speeds[i];
    }
    return sum * turning_steps_per_phase * 2 + const_delay_time;
}

void turn(bool left)
{
    Serial.println((unsigned long)steps_to_go_during_turn);
    set_direction(false, true, left);
    for (int i = 0; i < 8; i++)
    {      
        for (int j = 0; j < turning_steps_per_phase; j++)
        {
            digitalWrite(step1, HIGH);
            digitalWrite(step2, HIGH);
            delayMicroseconds(turning_speeds[i]);
            digitalWrite(step1, LOW);
            digitalWrite(step2, LOW);
            delayMicroseconds(turning_speeds[i]);
        }
    }
}




void accel_decel(bool reversed) // reversed is true if decelerating
{
    /*
    Function to accelerate/decelerate the motors when going straight, 4 steps of acceleration/deceleration
    */
    if (reversed) reverse(acc_speeds.begin(), acc_speeds.end());
    // Go through each acceleration phase and pulse the motors accordingly
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < straight_steps_per_acceleration_phase; j++)
        {

            if (pulses % rate == 0)
            {
                if (adjust_right)
                {
                    digitalWrite(step1, HIGH);
                    digitalWrite(step2, LOW);
                }
                else
                {
                    digitalWrite(step1, LOW);
                    digitalWrite(step2, HIGH);
                }
            }
            else
            {
                digitalWrite(step1, HIGH);
                digitalWrite(step2, HIGH);
            }
            delayMicroseconds(acc_speeds[i]);
            digitalWrite(step1, LOW);
            digitalWrite(step2, LOW);
            delayMicroseconds(acc_speeds[i]);
            pulses++;
        }
    }

    if (reversed) reverse(acc_speeds.begin(), acc_speeds.end());
}

ll get_accel_time_per_straight()
{
    // Return the average gap per pulse times the amount of steps per acceleration phase times 4 (for each phase) times 2 (for acceleration and deceleration)

    ll sum = 0;
    for (int i = 0; i < 4; i++)
    {
        sum += acc_speeds[i];
    }
    return sum * straight_steps_per_acceleration_phase * 2 * 2 + const_delay_time;
}

ll get_accel_distance_per_straight()
{
    // get the number of pulses for the motors in one straight
    return straight_steps_per_acceleration_phase * 4 * 2;
}


void straight(bool forward, ld steps, ld time) // Time is only the time at max speed, not accel/decel time
{
    // Set the direction of the motors
    set_direction(forward, false, false);
    // Accelerate
    unsigned long sum = 0;
    long cur = micros();
    accel_decel(false);
    sum += micros() - cur;
    // Calculate the delay between each pulse
    ll delay = time / steps / 2;
    // Make sure it doesn't go above the torque limit
    if (delay < min_gap_between_pulses)
    {
        delay = min_gap_between_pulses;
    }
    Serial.println((unsigned long)delay);

    // Go at constant speed

    for (int i = 0; i < steps; i++)
    {
        if (pulses % rate == 0)
        {
            if (adjust_right)
            {
                digitalWrite(step1, HIGH);
                digitalWrite(step2, LOW);
            }
            else
            {
                digitalWrite(step1, LOW);
                digitalWrite(step2, HIGH);
            }
        }
        else
        {
            digitalWrite(step1, HIGH);
            digitalWrite(step2, HIGH);
        }
        delayMicroseconds(delay);
        digitalWrite(step1, LOW);
        digitalWrite(step2, LOW);
        delayMicroseconds(delay);
        pulses++;
    }
    // Decelerate
    cur = micros();
    accel_decel(true);
    sum += micros() - cur;
    Serial.println(sum);
    Serial.println((unsigned long)get_accel_time_per_straight());
}








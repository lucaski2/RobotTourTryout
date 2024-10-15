#include <Arduino.h>
#include <math.h>
#include <vector>
#include <iostream>


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




ld dist_between_wheels = 204.5; // distance between the two wheels in millimeters

ld micro_stepping = 16; // amount of microstepping

// delays between each step during the acceleration in micros
vector<ll> turning_speeds = {500, 350, 200, 170, 170, 200, 350, 500}; 

// delay between each step during the acceleration/deceleration in micros
vector<ll> acc_speeds = {450, 350, 250, 170}; 

// Steps per phase in accel/decel for straight
ll straight_steps_per_acceleration_phase = 250; 

// distance between wheels * pi / 4 * steps per revolution / wheel circumference, adjusted slightly based on testing
const ld steps_to_go = 1935; 

// steps per acceleration/deceleration phase for turning
ll turning_steps_per_phase = round(steps_to_go / 8); 

ld wheel_circumference = 84 * M_PI; // 84 mm diameter, circumference in millimeters

// Steps needed to complete a revolution with microstepping
ld steps_per_revolution = 200 * micro_stepping;

// minimum gap that doesn't exceed torque limit
ll min_gap_between_pulses = 130; 

// Struct that stores the information needed for each step of the run
struct Instruction
{
    bool is_straight;
    bool forward_or_left;
    ld steps;
};




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

void turn(bool left)
{
    set_direction(false, true, left);
    // 
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

ll get_time_per_turn()
{
    ll sum = 0;
    for (int i = 0; i < 8; i++)
    {
        sum += turning_speeds[i];
    }
    return sum / turning_speeds.size() * turning_steps_per_phase * 2;
}


ll accel_decel(bool reversed)
{
    if (reversed) reverse(acc_speeds.begin(), acc_speeds.end());
    // Go through each acceleration phase and pulse the motors accordingly
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < straight_steps_per_acceleration_phase; j++)
        {
            digitalWrite(step1, HIGH);
            digitalWrite(step2, HIGH);
            delayMicroseconds(acc_speeds[i]);
            digitalWrite(step1, LOW);
            digitalWrite(step2, LOW);
            delayMicroseconds(acc_speeds[i]);
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
    return sum / acc_speeds.size() * straight_steps_per_acceleration_phase * 4 * 2;
}

ll get_accel_distance_per_straight()
{
    // get distance for accel and decel
    return straight_steps_per_acceleration_phase * 4 * 2;
}


void straight(bool forward, ld steps, ld time) // Time is only the time at max speed, not accel/decel time
{
    // Set the direction of the motors
    set_direction(forward, false, false);
    // Subtract the distance for acceleration and deceleration
    steps -= get_accel_distance_per_straight();
    // Accelerate
    accel_decel(false);
    
    // Calculate the delay between each pulse
    ll delay = steps / time;
    // Make sure it doesn't go above the torque limit
    if (delay < min_gap_between_pulses)
    {
        delay = min_gap_between_pulses;
    }

    // Go at constant speed

    for (int i = 0; i < steps; i++)
    {
        digitalWrite(step1, HIGH);
        digitalWrite(step2, HIGH);
        delayMicroseconds(delay);
        digitalWrite(step1, LOW);
        digitalWrite(step2, LOW);
        delayMicroseconds(delay);
    }
    // Decelerate
    accel_decel(true);
}






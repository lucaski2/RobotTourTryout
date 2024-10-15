#include "Drivetrain.h"


// Time for total run in microseconds
long double total_time = 2 * pow(10, 6); // convert time to microseconds


//! Robot length: ~18 cm
//! distance to center of wheel from front of robot: ~4 cm

//! Struct format: {is_straight, forward_or_left, steps}

//! Starting with L means left, F means forward, R means right, B means backwards, SF means forward a specific amount and SB means backwards a specific amount
// For forward and backward, the number is the amount of 50 cm steps
// To start should be SF29, forward 25 to get to center of box, 4 to move center to the middle of two wheels
vector<String> coded_instructions = {
  "SF29",
  "R",
  "F3",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
};

vector<Instruction> instructions(coded_instructions.size());

void setup() {
  // setup stepper pins
  pinMode(step1, OUTPUT);
  pinMode(dir1, OUTPUT);
  pinMode(en1, OUTPUT);
  pinMode(step2, OUTPUT);
  pinMode(dir2, OUTPUT);
  pinMode(en2, OUTPUT);
  // set enable to low
  digitalWrite(en1, LOW);
  digitalWrite(en2, LOW);

  // set microstepping to 1/16
  digitalWrite(ms1_1, HIGH);
  digitalWrite(ms2_1, HIGH);

  digitalWrite(ms1_2, HIGH);
  digitalWrite(ms2_2, HIGH);

  // setup button
  pinMode(button, INPUT_PULLUP);
  Serial.begin(57600);
}




void loop()
{ 
  if (digitalRead(button) == LOW)
  { 
    long long start_time = micros();
    Serial.println("Button pressed"); 
    delay(500);

    for (int i = 0; i < instructions.size(); i++)
    {
      // steps in cm, convert to steps
      ld steps = instructions[i].steps * 10 * steps_per_revolution / wheel_circumference;
      instructions[i].steps = steps;
    }
    

    for (int i = 0; i < instructions.size(); i++)
    {
      //! For each instruction, if it is a straight, estimate the time it takes for the rest of the run and calculate the time for the straight accordingly
      long long current_time = micros();
      long long time = total_time - current_time + start_time; // time that we have left
      long long total_steps = 0;

      for (int j = i; j < instructions.size(); j++)
      {
        if (instructions[j].is_straight)
        {
          time -= get_accel_time_per_straight(); // estimate time for acceleration
          total_steps += instructions[j].steps - get_accel_distance_per_straight(); // Add steps during constant speed to total_steps, ignore accel/decel because we can't control it
        }
        else
        {
          time -= get_time_per_turn(); // estimate time for turning
        }
      }


      // After calculating, run the command based on the time we have left

      if (!instructions[i].is_straight)
      {
        turn(instructions[i].forward_or_left);
      }
      else
      {
        // Calculate time left for all straights only when they are at constant speed
        long long time_in_straight = time / total_steps * (instructions[i].steps - get_accel_distance_per_straight());
        straight(instructions[i].forward_or_left, instructions[i].steps, time_in_straight);
      }
    }
  }
}


#include "Drivetrain.h"

// Time for total run in microseconds
long double total_time = 5 * pow(10, 6);            // convert time to microseconds
long double delay_between_steps = 500 * pow(10, 3); // Delay between each step in microseconds

void convert_instructions();

//! Robot length: ~18 cm
//! distance to center of wheel from front of robot: 2.5 cm

//! Struct format: {is_straight, forward_or_left, steps}

//! Starting with L means left, R means right, and any other number means forward or backwards that many cm
// For forward and backward, the number is the amount of 50 cm steps

// To start should be 27.5, forward 25 to get to center of box, 2.5 to move center to the middle of two wheels
vector<String> coded_instructions = {
    // "27.5",
    // "R",
    // "50",
    // "L",
    // "50",
    // "L",
    // "100",
    // "L",
    // "50",
    // "R",
    // "50",
    // "R",
    // "100",
    // "R",
    // "50",
    // "L",
    // "50",
    // "R",
    // "100",
    // "R",
    // "50",
    // "L",
    // "50",
    // "L",
    // "50",
    // "-50",
    // "L",
    // "50",
    // "L",
    // "100",
    // "L",
    // "50",
    // "L",
    // "50",
    // "-50",
    // "L",
    // "50",
    // "R",
    // "150",
    // "L",
    // "97.5",

    // Check

    // "150",
    // "R",
    // "R",
    // "R",
    // "R",
    "150",
    // "",
    // "",
};

vector<Instruction> instructions;

void setup()
{
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
  // Convert coded instructions to instructions

  convert_instructions();
  // Convert cm to steps
  for (int i = 0; i < instructions.size(); i++)
  {
    // Straight is always short by 1 cm, so add 1 cm
    instructions[i].steps += 1;
    // steps in cm, convert to steps
    ld steps = instructions[i].steps * 10 * steps_per_revolution / wheel_circumference;
    instructions[i].steps = steps;
    Serial.println((unsigned long)instructions[i].steps);
  }

  // Print battery voltage
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1023.0);
  float battery_voltage = voltage * (R1 + R2) / R2;
  Serial.println(battery_voltage);
}

void convert_instructions()
{

  for (int i = 0; i < coded_instructions.size(); i++)
  {
    // Check if the instruction is empty
    if (coded_instructions[i].length() == 0)
    {
      break;
    }
    // Left turn
    else if (coded_instructions[i][0] == 'L')
    {
      Instruction instruction;
      instruction.is_straight = false;
      instruction.forward_or_left = true;
      instructions.push_back(instruction);
    }
    // Right turn
    else if (coded_instructions[i][0] == 'R')
    {
      Instruction instruction;
      instruction.is_straight = false;
      instruction.forward_or_left = false;
      instructions.push_back(instruction);
    }
    // If the instruction is negative, it means it is a backwards instruction
    else if (coded_instructions[i][0] == '-')
    {
      Instruction instruction;
      instruction.is_straight = true;
      instruction.forward_or_left = false;
      instruction.steps = atof(coded_instructions[i].substring(1).c_str());
      instructions.push_back(instruction);
    }

    // If the instruction is positive, it means it is a forward instruction
    else
    {
      Instruction instruction;
      instruction.is_straight = true;
      instruction.forward_or_left = true;
      instruction.steps = atof(coded_instructions[i].c_str());
      instructions.push_back(instruction);
    }
  }
}

void loop()
{
  if (millis() % 100 == 0)
  {
    // Print battery voltage
    int sensorValue = analogRead(A0);
    float voltage = sensorValue * (5.0 / 1023.0);
    float battery_voltage = voltage * (R1 + R2) / R2;
    Serial.println(battery_voltage);
  }

  if (digitalRead(button) == LOW)
  {
    Serial.println("Button pressed");
    delay(500);

    ll start_time = micros();

    for (int i = 0; i < instructions.size(); i++)
    {
      //! For each instruction, if it is a straight, estimate the time it takes for the rest of the run and calculate the time for the straight accordingly
      ll current_time = micros();
      ll time = total_time - (current_time - start_time); // time that we have left
      ll total_steps = 0;

      for (int j = i; j < instructions.size(); j++)
      {
        if (instructions[j].is_straight)
        {
          time -= get_accel_time_per_straight();                                    // estimate time for acceleration
          total_steps += instructions[j].steps - get_accel_distance_per_straight(); // Add steps during constant speed to total_steps, ignore accel/decel because we can't control it
        }
        else
        {
          time -= get_time_per_turn(); // estimate time for turning
        }
        if (j != instructions.size() - 1)
        {
          time -= delay_between_steps; // delay between each instruction
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
        ll time_in_straight = time / total_steps * (instructions[i].steps - get_accel_distance_per_straight());
        straight(instructions[i].forward_or_left, instructions[i].steps - get_accel_distance_per_straight(), time_in_straight);
      }

      delayMicroseconds(delay_between_steps);
    }
    Serial.println((unsigned long)(micros() - start_time));
  }
}

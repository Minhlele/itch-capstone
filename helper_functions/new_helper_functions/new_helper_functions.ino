// This file is a bunch of helper functions for querying sensor states and setting actuators.
// NOTE: Pins are not freely chosen. Analog 4 and 5 are used for the I2C protocol (see Wire library),
//    and the motor enable pins and servo pin need to be PWM pins, which the nano only has a handful of.
//    Everything else can be moved around.

/**************************************************************************************
 * Library Declarations
 **************************************************************************************/

#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <Servo.h>
/**************************************************************************************
 * Data Structures
 **************************************************************************************/
 
// Which Side of the robot you are referencing, oriented as looking from hook to pusher.
enum Side {
  RIGHT = 0b01,
  LEFT = 0b10,
  BOTH = 0b11,
};

/**************************************************************************************
 * Global Variables
 **************************************************************************************/

/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

/**************************************************************************************
 * Constants
 **************************************************************************************/
 //Initial Servo
Servo servo;
int spoint = 103;
char value;
// IR sensors for motor encoder, pins and constants
const int LEFT_IR_PIN = A1;
const int RIGHT_IR_PIN = A2;
// Ultrasonic Sensor pins and constants.
const int ECHO_PIN = 6;
const int TRIG_PIN = 7;
// Using the speed of sound at 50% humidity and 20 degrees C, cm/us.
const double SOUND_SPEED_CONSTANT = 0.0344;
const double WALL_THRESHOLD = 8.0; // TODO: Tweak and use in code
// Servo pin and hook constants.
const int SERVO_PIN = 9;
const int HOOK_MIN = 125;
const int HOOK_MAX = 255;
const bool HOOK_UP = true;
// Motor A connections
const int RIGHT_EN = 10;
const int RIGHT_DIR_1 = 2;
const int RIGHT_DIR_2 = 3;
// Motor B connections.
const int LEFT_EN = 11;
const int LEFT_DIR_1 = 4;
const int LEFT_DIR_2 = 5;
//Initial MOVE_DISTANCE

/**************************************************************************************
 * Various Helper Functions
 **************************************************************************************/

/**
 * Read the ultrasonic sensor, scaled roughly to centimeters. It may return -1 if out of range (> 4m or <2cm).
 */
 double readUltrasonicDistance() {
    float duration, distance;
    digitalWrite(TRIG_PIN, LOW); 
    delayMicroseconds(2);
   
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    duration = pulseIn(ECHO_PIN, HIGH); // Duration of pulse in microseconds
    distance = (duration / 2) * SOUND_SPEED_CONSTANT;
    
    if (distance >= 400 || distance <= 2){
      return -1; // Out of range
    }
    else {
      return distance;
    }
 }

/**
 * Lower level, sets the states for each passed pin according to given motorSpeed.
 *  Use speedControl or stopMotor instead.
 */
void setMotorPins(int dir1, int dir2, int en, int motorSpeed) {
  if (motorSpeed < 0) {
    digitalWrite(dir1, LOW);
    digitalWrite(dir2, HIGH);
  } else if (motorSpeed > 0) {
    digitalWrite(dir1, HIGH);
    digitalWrite(dir2, LOW);
  } else {
    digitalWrite(dir1, LOW);
    digitalWrite(dir2, LOW);
  }
  analogWrite(en, abs(motorSpeed));
}

/**
 * Control speed and direction of an individual motor.
 * @param motor Set to RIGHT, LEFT, or BOTH.
 * @param motorSpeed positive for forward, negative for backward, -255 to 255.
 */
void speedControl(Side motor, int motorSpeed) {
  int dir1, dir2, enable = 0;
  if (motor & RIGHT) {
    setMotorPins(RIGHT_DIR_1, RIGHT_DIR_2, RIGHT_EN, motorSpeed);
  }
  if (motor & LEFT){
    setMotorPins(LEFT_DIR_1, LEFT_DIR_2, LEFT_EN, motorSpeed);
  }
}



/**
 * Stop a motor.
 * @param motor Set to RIGHT, LEFT, or BOTH.
 */
void stopMotor(Side motor) {
  speedControl(motor, 0);
}

void forward(int move_distance) {
  speedControl(BOTH, 150);
  delay(move_distance);
  stopMotor(BOTH);
}

void left(int move_distance) {
  speedControl(RIGHT, 150);
  delay(move_distance);
  stopMotor(RIGHT);
}

void right(int move_distance) {
  speedControl(LEFT, 150);
  delay(move_distance);
  stopMotor(LEFT);
}

void backward(int move_distance) {
  speedControl(BOTH, -150);
  delay(move_distance);
  stopMotor(BOTH);
}

//Control the robot using command from board
//Right now, this is getting input from Serial not the actual board
//Current commands : move forward, backward, rotate,stop
//TODO: Fine tune and fix bugs,Add more command
void boardControl(){
  if (Serial.available() > 0) {
    value = Serial.read();
    Serial.println(value);
  }
  //Get move distance from user
  int move_distance = 500;
  int rotate_degree = 180;

  if (value == "forward") {
    forward(move_distance);
  }
  else if (value == "backward") {
    backward(move_distance);
  }

  else if (value == "rotate") {
    rotate(rotate_degree);
  }
  else if (value == "stop") {
    stopMotor(BOTH);

  }




}









int hookPos = 0;
void setHook(bool hookUp) {
  int dir = -1;
  if (hookUp)
    dir = 1;
  if (hookPos == 0) {
    hookPos++;
  } else if (hookPos == 255) {
    hookPos--;
  }
  for (; (hookPos < 255) && (hookPos > 0); hookPos += dir) {
    analogWrite(SERVO_PIN, hookPos); 
    delay(10);
  }
}

/**
 * Identify the block's color as RED,GREEN or BLUE
 * @param red Red value from sensor
 * @param green green value from sensor
 * @param blue blue value from sensor
 */
 //TODO: If we need to add more color, then have to incorporate the green and blue rgb. If not, then the red value should be sufficient
String color_identify(uint16_t red, uint16_t green, uint16_t blue){
  if(red >= 445 && red <= 468) {
    return String("Green");
  }
  else if(red >= 731 && red <= 810) {
    return String("Red");
  }
  else if (red >= 813 && red <= 860) {
    return String("Blue");
  }
  else{
    return String("No Color Found");
  }
}

int right_side() {
  servo.write(20);
  delay(800);
  int left = readUltrasonicDistance();
  return left;
}

int left_side() {
  servo.write(180);
  delay(800);
  int right = readUltrasonicDistance();
  return right;
}

//Rotate the robot to a given degree
void rotate(int degree) {
  servo.detach();
  delay(2000);
  servo.attach(7);
  servo.write(degree);
  delay(1000);
}


/**************************************************************************************
 * Arduino Setup and Loop functions
 **************************************************************************************/

void setup() {
  // Set all the motor control pins to outputs
  pinMode(RIGHT_EN, OUTPUT);
  pinMode(LEFT_EN, OUTPUT);
  pinMode(RIGHT_DIR_1, OUTPUT);
  pinMode(RIGHT_DIR_2, OUTPUT);
  pinMode(LEFT_DIR_1, OUTPUT);
  pinMode(LEFT_DIR_2, OUTPUT);

  // Set Servo pin to output
  pinMode(SERVO_PIN, OUTPUT);

  servo.attach(SERVO_PIN);

  // Set IR pin to input
  

  // Initialize pin values and global vars
  setHook(false); 
  stopMotor(BOTH);

  Wire.begin();
  Serial.begin(9600);

  while(!Serial) {}

   /* Configure TSL2550 with Extended Range */
   if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }
}



void loop() {
//    int irLeft = analogRead(LEFT_IR_PIN);
//    int irRight = analogRead(RIGHT_IR_PIN);
//    Serial.print("Left: ");
//    Serial.println((irLeft * (5.0/1023.0)));
//    Serial.print("Right: ");
//    Serial.println((irRight * (5.0/1023.0)));
//    delay(100);
  uint16_t r, g, b, c, colorTemp, lux;


  tcs.getRawData(&r, &g, &b, &c);
  // colorTemp = tcs.calculateColorTemperature(r, g, b);
  colorTemp = tcs.calculateColorTemperature_dn40(r, g, b, c);
  lux = tcs.calculateLux(r, g, b);
  String color_name = color_identify(r,g,b);

  Serial.print("Color Temp: "); Serial.print(colorTemp, DEC); Serial.print(" K - ");
  Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" - ");
  Serial.print("R: "); Serial.print(r); Serial.print(" ");
  Serial.print("G: "); Serial.print(g, DEC); Serial.print(" ");
  Serial.print("B: "); Serial.print(b, DEC); Serial.print(" ");
  Serial.print("C: "); Serial.print(c, DEC); Serial.print(" ");
  Serial.println(" ");
  Serial.println(color_name);
  //forward();
  left(500);
  stopMotor(BOTH);
  
}

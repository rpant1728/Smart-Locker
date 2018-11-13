// Including libraries required for the working of various sensors
#include <Servo.h>
#include <Wire.h>
#include <Time.h>
#include <Digital_Light_TSL2561.h>
#include <Keypad.h>
#include <stdint.h>


// Declaring variables which will be used for the inputs given by the Keypad Sensor
const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6}; 
byte colPins[COLS] = {5, 4, 3, 2}; 

// Defined pins for sensors using principles of 'Embedded Programming'
const uint8_t trigPin = 12;
const uint8_t echoPin = 10;
const uint8_t buzzerTrigger = 48;
const uint8_t lightOff = 50;

// Variables defined
String temp;
String password;
String initHour;
String initMinute;
String initDay;
String initMonth;
String initYear;
String dHour;
String dMinute;
String dDay;
String dMonth;
String dYear;
unsigned long initTime;
unsigned long dTime;
unsigned long duration;
uint16_t distance;
uint8_t counterIR;
uint8_t counterBuzzer;
uint8_t counterLight;
bool lockerState;
bool flapState;

// Creating instances of Keypad and Servo motor
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
Servo servoFlap, servoDoor;

// Waits for connection of Bluetooth
bool detectBleBaudRate(){
  Serial.println("Detecting BLE baud rate:");
  while(1){
    Serial.write("Checking 9600");
    Serial1.begin(9600);
    Serial1.write("AT");
    Serial1.flush();
    delay(50);
    String response = Serial1.readString();
    if(response == "OK"){
      Serial.println("Detected");
      return true;
    } 
    else{
      Serial1.end();
    }
  }
  return false;
}

// A constant array to store the number of days in each month
const uint8_t monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; 
  
// This function counts number of leap years before the given date 
uint16_t countLeapYears(uint8_t m, uint16_t y){ 
    uint16_t years = y; 
  // Check if the current year needs to be considered for the count of leap years or not 
  if (m <= 2) 
      years--; 
  // An year is a leap year if it is a multiple of 4, multiple of 400 and not a multiple of 100. 
  return years / 4 - years / 100 + years / 400; 
}

// Calculate the difference between any two dates in number of days
int getDifference(uint8_t dt1d, uint8_t dt1m, uint16_t dt1y, uint8_t dt2d, uint8_t dt2m, uint16_t dt2y){ 
  // Count total number of days till the first date   
  unsigned long n1 = dt1y*365 + dt1d; 
  // Add days for months in given date 
  for (int i=0; i<dt1m - 1; i++) 
      n1 += monthDays[i]; 
  // Since every leap year is of 366 days, add a day for every leap year 
  n1 += countLeapYears(dt1m, dt1y); 
  // Count total number of days till the second date 
  unsigned long int n2 = dt2y*365 + dt2d; 
  for (int i=0; i<dt2m - 1; i++) 
    n2 += monthDays[i]; 
  n2 += countLeapYears(dt2m, dt2y); 
  // Return difference between two dates in milliseconds
  return (n2 - n1)*24*60*60*1000; 
} 
 
// Standard setup function of Arduino, initializes various variables
void setup() {
  Wire.begin();
  TSL2561.init();
  Serial.begin(9600);                      // Starts the serial communication
  
  // Initializes instances of Servo motors
  servoFlap.attach(13);
  servoFlap.write(0);
  servoDoor.attach(11);
  servoDoor.write(120);
  
  // Sets the modes of various pins of the Arduino as Input or Output
  pinMode(trigPin, OUTPUT);              
  pinMode(echoPin, INPUT);                
  pinMode(buzzerTrigger, OUTPUT); 
  pinMode(lightOff, OUTPUT); 
  
  // Initialize the light sensor and ultrasonic buzzer
  digitalWrite(buzzerTrigger, LOW);
  digitalWrite(lightOff, HIGH);
  
  // Checks Bluetooth Connection
  if (detectBleBaudRate())
    Serial.write("Ready, type AT commands\n\n");
  else
    Serial.write("Not ready. Halt");
  
  // Various variables initialized
  counterIR = 0;
  counterBuzzer = 0;
  counterLight = 0;
  lockerState = false;
  flapState = true;
  password = "1111";
  temp = "";
  dTime = -1;
  initTime = 0;
}

void loop() {
  // In case of break in, activate alarm
  if(lockerState && digitalRead(42)==0){
    digitalWrite(buzzerTrigger, HIGH);
  }

  // Close door if lockerState is found to be false
  if(!lockerState){
    if(digitalRead(42) == 1){
      servoDoor.write(0);
      flapState = true;
      lockerState = true;
    }
  }

  // If deadline time reached, activate alarm and close flap
  if(dTime <= millis()){
    Serial.println("LOCKDOWN");
    digitalWrite(buzzerTrigger, HIGH);
    delay(500);
    digitalWrite(buzzerTrigger, LOW);
    servoFlap.write(120);
    flapState = false;
    dTime = -1;
  }

  // Code to deal with Keypad Input
  char customKey = customKeypad.getKey();
  if(customKey){
    // If enter key pressed
    if(customKey=='*'){
      Serial.println(temp);
      // If incorrect password entered, activate alarm
      if(password!=temp){
        digitalWrite(buzzerTrigger, HIGH);
        delay(500);
        digitalWrite(buzzerTrigger, LOW);
        Serial.println("Invalid Password");
      }
      // Else open both door and flap
      else{
        Serial.println(temp);
        servoDoor.write(120);
        servoFlap.write(0);
        delay(1000);
        flapState = true;
        lockerState = false;
        digitalWrite(buzzerTrigger, LOW);
      }
      temp = "";
    }
    else temp += customKey;
    Serial.println(customKey);
  }

  // If locker is open
  if(!lockerState){
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    // Calculating the distance
    distance= duration*0.034/2;  
    // If user is farther from the locker than the permissiblle limit, increment buzzer counter  
    if(distance > 29)
      counterBuzzer = counterBuzzer + 1;
    else
      counterBuzzer = 0;
  
    if(counterBuzzer > 150)
      counterBuzzer = 126;  
    // After the user has been far from locker for a sufficiently long time, activate buzzer  
    if(counterBuzzer > 125)          
      digitalWrite(buzzerTrigger, HIGH);
    else
      digitalWrite(buzzerTrigger, LOW);
    // Read input from light sensor
    int light = TSL2561.readVisibleLux();
    // If light is less than permissible value, increment light counter
    if(light < 200) 
      counterLight = counterLight + 1; 
    else
      counterLight = 0;
    if(counterLight > 100)
      counterLight = 51;
    // If light value less than the threshold value for a considerable amount of time, activate electric bulb
    if(counterLight > 20){
      digitalWrite(lightOff, LOW);
    }
    else
      digitalWrite(lightOff, HIGH);
  }
  
  // If locker is closed, variables are initialized 
  else{
    counterBuzzer = 0;
    counterLight = 0;
    if(digitalRead(42)==1)
      digitalWrite(buzzerTrigger, LOW);
    digitalWrite(lightOff, HIGH);
  }

  // If flap is open
  if(flapState){
    // Read input from proximity sensor
    int value = analogRead(A0);
    // If obstacle detected closer than permissible limit, increment counter
    if(value>300){
      counterIR += 1;
    }
    else{
      counterIR = 0;
    }
    if(counterIR > 100){
      counterIR = 51;
    }
    // If obstacle detected closer than permissible limit for a considerable amount of time, close flap as locker is full
    if(counterIR > 50){
      servoFlap.write(120);
    }
    else{
      servoFlap.write(0);
    }
  }
  
  // If Bluetooth connection eneabled
  if(Serial1.available()){
    Serial.write("ble: ");
    // Read serial input from device connected via Blurttoth
    String str = Serial1.readString();
    // Convert String to a character array
    char input[str.length()+2];
    str.toCharArray(input, str.length()+2);
    input[str.length()+1] = 0;
    // Split input until the first empty space
    char *command = strtok(input," ");
    Serial.println(command);
    String com(command);
    // If open command entered
    if(com == "open"){
      // Split string further to extract password from the command
      command = strtok(0, " ");
      Serial.println(command);
      String com1(command);
      // If password is correct, open door and flap
      if(com1 == password){
        servoDoor.write(120);
        servoFlap.write(0);
        delay(1000);
        lockerState = false;
        flapState = true;
      }
      // If password is incorrect, activate alarm
      else{
        digitalWrite(buzzerTrigger, HIGH);
        delay(500);
        digitalWrite(buzzerTrigger, LOW);
        Serial.println("Invalid Password");
      }
    }
    // If command to change password is entered
    else if(com == "change"){
      // Split string further to obtain current password
      command = strtok(0, " ");
      String com1(command);
      // If password is correct
      if(com1 == password){
        // Split string further to obtain the new password
        command = strtok(0, " ");
        String com2(command);
        // Set the password to new password
        password = com2;
        Serial.println("changed");
      }
      // If incorrect password is entered, activate alarm
      else{
        digitalWrite(buzzerTrigger, HIGH);
        delay(500);
        digitalWrite(buzzerTrigger, LOW);
        Serial.println("Invalid Password");  
      }
    }
    // If command to calibrate Arduino is entered
    else if(com == "calibrate"){
      // Split string further to obtain current password
      command = strtok(0, " ");
      String com1(command);
      Serial.println("Calibrating..");
      // If password is correct
      if(com1 == password){
        // Split the string further to parse the day, month, year, hour and minute values
        command = strtok(0, ":");
        String com2(command);
        initHour = com2;
        command = strtok(0, " ");
        com2 = String(command);
        initMinute = com2;
        command = strtok(0, "/");
        com2 = String(command);
        initDay = com2;
        command = strtok(0, "/");
        com2 = String(command);
        initMonth = com2;
        command = strtok(0, " ");
        com2 = String(command);
        initYear = com2;
        // Set variable to mark a reference starting time
        initTime = millis();
      }
      // If incorrect password entered, activate alarm
      else{
        digitalWrite(buzzerTrigger, HIGH);
        delay(500);
        digitalWrite(buzzerTrigger, LOW);
        Serial.println("Invalid Password");
      }
    }
    // If command to implement deadline entered
    else if(com == "deadline"){
      // Split string further to obtain current password
      command = strtok(0, " ");
      String com1(command);
      Serial.println("Calibrating..");
      // If password is correct
      if(com1 == password){
        // Split the string further to parse the day, month, year, hour and minute values
        command = strtok(0, ":");
        String com2(command);
        dHour = com2;
        command = strtok(0, " ");
        com2 = String(command);
        dMinute = com2;
        command = strtok(0, "/");
        com2 = String(command);
        dDay = com2;
        command = strtok(0, "/");
        com2 = String(command);
        dMonth = com2;
        command = strtok(0, " ");
        com2 = String(command);
        dYear = com2;
        // Set deadline time to initial reference time added to the difference between the deadline and the starting times
        dTime = initTime + getDifference(initDay.toInt(), initMonth.toInt(), initYear.toInt(), dDay.toInt(), dMonth.toInt(), dYear.toInt())
                + (dHour.toInt() - initHour.toInt())*60*60*1000 + (dMinute.toInt() - initMinute.toInt())*60*1000;
      }
      // If incorrect password entered, activate alarm
      else{
        digitalWrite(buzzerTrigger, HIGH);
        delay(500);
        digitalWrite(buzzerTrigger, LOW);
        Serial.println("Invalid Password");
      }
    }
    Serial.write('\n');
    Serial.print(str + "\n");
  }
}

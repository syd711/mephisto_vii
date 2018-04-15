//Libraries:
#include <TEA5767.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

//Constants:
TEA5767 Radio; //Pinout SLC and SDA - Arduino uno pins A5 and A4


//rotary encoder stuff
const int encoderPin1 = 2;
const int encoderPin2 = 3;
const int ENCODER_BUTTON_PIN = 4; //push button switch
volatile int lastEncoded = 0;
int encoderValue = 0;
long lastencoderValue = 0;
int lastMSB = 0;
int lastLSB = 0;
int encoderButtonState = 0;

//multiplexer
int SER_Pin = 7; //pin 14 on the 75HC595
int RCLK_Pin = 8; //pin 12 on the 75HC595
int SRCLK_Pin = 9; //pin 11 on the 75HC595
//How many of the shift registers – change this
const int number_of_74hc595s = 2;
const int numOfRegisterPins = number_of_74hc595s * 8;
boolean registers[numOfRegisterPins];
int lights[] = {10, 9, 8, 3, 7, 15, 14, 13, 11, 6, 5, 4};

//sd card
File dataFile;
String DEFAULT_FEQUENCIES  = "95.5,95.5,95.5,95.5,95.5,95.5,95.5,95.5,95.5,95.5,95.5,95.5";
String dataString = "";

//  87,5 MHz bis 108 MHz
void setup() {
  Serial.begin(9600);  

  //init SD card
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }

  dataFile = SD.open("data.txt");
  while (dataFile.available() != 0) {  
    //A inconsistent line length may lead to heap memory fragmentation        
    dataString = dataFile.readStringUntil('\n');        
    if (dataString == "") { //no blank lines are anticipated        
      break;      
    }
    dataString.trim();
  } 
  if(dataString.length() == 0) {
    dataString = DEFAULT_FEQUENCIES;
    dataFile.println(dataString);
  }
  
  //init radio
  Radio.init();
  Radio.set_frequency(87.5);
  
  //rotary encoder
  pinMode(ENCODER_BUTTON_PIN, INPUT);
  pinMode(encoderPin2, INPUT); 
  pinMode(encoderPin1, INPUT);
  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  attachInterrupt(digitalPinToInterrupt(encoderPin1), updateEncoder, CHANGE); 
  attachInterrupt(digitalPinToInterrupt(encoderPin2), updateEncoder, CHANGE);

  //shift register
  pinMode(SER_Pin, OUTPUT);
  pinMode(RCLK_Pin, OUTPUT);
  pinMode(SRCLK_Pin, OUTPUT);  
  //reset all register pins
  clearRegisters();
  for(int i = numOfRegisterPins - 1; i >= 0; i--) {
    registers[i] = HIGH;
  }  
  writeRegisters();
}


void loop() {
  Serial.println(getStation(5));
  delay(1000);
}

/**
 * The rotary encoder implementation
 */
void updateEncoder(){   
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  //Serial.print("update ");
  //Serial.println(String(sum));

  //if(sum == 13 || sum == 4 || sum == 2 || sum == 11) encoderValue ++;
  if(sum == 2){
    encoderValue --;//skip updates

    String value = String(encoderValue);
    Serial.println(value);
  }
  //if(sum == 14 || sum == 7 || sum == 1 || sum == 8 ) encoderValue --;
  if(sum == 1) {
    encoderValue ++;//skip updates

    String value = String(encoderValue);
    Serial.println(value);
  }

  lastEncoded = encoded; //store this value for next time  
}  

/**
 * set all register pins to LOW
 */
void clearRegisters(){
  for(int i = numOfRegisterPins - 1; i >= 0; i--) {
    registers[i] = LOW;
  }
}

/**
 * Set and display registers.
 * Only call AFTER all values are set how you would like (slow otherwise)
 */
void writeRegisters(){  
  digitalWrite(RCLK_Pin, LOW);
  
  for(int i = numOfRegisterPins - 1; i >= 0; i--) {
    digitalWrite(SRCLK_Pin, LOW);  
    int val = registers[i];  
    digitalWrite(SER_Pin, val);
    digitalWrite(SRCLK_Pin, HIGH);  
  }
  digitalWrite(RCLK_Pin, HIGH);
}

/**
 * set an individual pin HIGH or LOW
 */
void setRegisterPin(int index, int value){
  registers[index] = value;
}

/**
 * Gets a value from the stations string from the SD card
 */
String getStation(int index) {
  char separator = ',';
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = dataString.length() - 1;
  for (int i = 0; i <= maxIndex && found <= index; i++) {
      if (dataString.charAt(i) == separator || i == maxIndex) {
          found++;
          strIndex[0] = strIndex[1] + 1;
          strIndex[1] = (i == maxIndex) ? i+1 : i;
      }
  }
  return found > index ? dataString.substring(strIndex[0], strIndex[1]) : "";
}

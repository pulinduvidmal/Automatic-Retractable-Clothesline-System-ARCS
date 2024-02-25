// Include Libraries
#include <Arduino.h> // Include the Arduino core library
#include <DHT.h> // Include the DHT library for the humidity and temperature sensor
#include <Stepper.h> // Include the Stepper library
#include <EEPROM.h>

#include <SoftwareSerial.h>

// Pin Definitions
#define DHT_PIN_DATA  5 // Define the pin for DHT data

#define inside_ledPin A2// Define the LED pin
#define auto_ledPin A3
#define outside_ledPin A4
#define sensorPin A0 // Define the pin for the rain sensor
#define ldr_pin A1

//Step Motor
int motorSpeed = 10; // Set the motor speed
Stepper myStepper(2048, 8, 9, 10, 11); // Initialize the Stepper object with the specified parameters


float isClothsOutside; // Flag to track if the motor has rotated
float isAuto;
float previous_value;


//DHT Sensor
DHT dht(DHT_PIN_DATA); // Create a DHT object


//////////////Thingspeak Read and Write //////////////////////////////////

// Pin Definitions
#define WIFI_PIN_TX 6 // 3 // blue
#define WIFI_PIN_RX 12 //2 // purple

#define LED_PIN_off 1//4
#define LED_PIN_on 7

SoftwareSerial espSerial(WIFI_PIN_RX, WIFI_PIN_TX);

const char* SSID = "Dialog 4G FF2";// Enter your Wi-Fi name
const char* PASSWORD = "Tash@0927"; // Enter your Wi-Fi password

String HOST = "api.thingspeak.com";
String PORT = "80";
String API = "ZQRO45V38G5PA209"; // Write API KEY

String motorStatus = "/channels/2225714/fields/1/last.txt";
String isClothOutsideField = "/channels/2225714/fields/6/last.txt";
String isAutoField = "/channels/2225714/fields/7/last.txt";


// Sample Values for Sensor Data
float Temperature;
float Humidity;
float rainValue;
float lightIntensity;

int countTrueCommand;
int countTimeCommand;
boolean found = false;

void setup() {
  Serial.begin(9600); // Initialize the serial communication

  myStepper.setSpeed(motorSpeed); // Set the motor speed

  while (!Serial); // Wait for the serial port to connect (needed for native USB)
  Serial.println("start"); // Print "start" message

  pinMode(inside_ledPin, OUTPUT); // Set the LED pin as an output
  pinMode(auto_ledPin, OUTPUT);
  pinMode(outside_ledPin, OUTPUT);

  digitalWrite(inside_ledPin, LOW); // Turn off the LED
  digitalWrite(auto_ledPin, LOW);
  digitalWrite(outside_ledPin, LOW);

  dht.begin(); // Initialize the DHT sensor


  ///////////////// Thingspeak //////////////////////////
  // Initialize ESP8266 serial communication
  espSerial.begin(9600);

  connectToWiFi();

  //Define testing LED pin output
  pinMode(LED_PIN_off, OUTPUT);
  pinMode(LED_PIN_on, OUTPUT);

  //isClothsOutside = readThingSpeak(isClothOutsideField);

  //delay(30);

  //isAuto = readThingSpeak(isAutoField);

  //delay(30);
  isClothsOutside = EEPROM.read(0);
  isAuto = EEPROM.read(0);

}

void loop() {

  Serial.print("isClothsOutside = ");
  Serial.println(isClothsOutside);
  Serial.print("isAuto = ");
  Serial.println(isAuto);


  // LDR
  float lightIntensity = analogRead(ldr_pin); // Read the ldr_value intensity from analog pin A1

  delay(30);

  //DHT
  float Humidity = 50;//dht.readHumidity(); // Read the humidity value from the DHT sensor
  float Temperature = 29.3;//dht.readTempC(); // Read the temperature value from the DHT sensor in Celsius

  delay(30);

  //Rain Sensor
  float rainValue = readRainSensor(); // Read the rain sensor value

  delay(30);

  printToSerial(float(lightIntensity), float(Humidity), float(Temperature), float(rainValue));

  ////////////////////////////////////// From Here Onwards thingspeak ////////////

  float checkFieldStatus = readThingSpeak(motorStatus);
  Serial.println(checkFieldStatus);


if (checkFieldStatus) {
    manualModeon();
    }
  else if(!checkFieldStatus && !isAuto){
    ManualModeoff(lightIntensity, Humidity, rainValue); 
   
    }
  
  else  {
    autoMode(lightIntensity, Humidity, rainValue);
  }
}

void manualModeon() {
  Serial.println(F("Manual mode on"));
  digitalWrite(LED_PIN_on, HIGH);
  digitalWrite(LED_PIN_off, LOW);
  isAuto = false;
  digitalWrite(auto_ledPin, LOW);
  
  turnOnInsideClothesRedLED();
  
}
void ManualModeoff(int lightIntensity,int Humidity,int rainValue){
  isAuto=true;
  if (!(lightIntensity < 150 || Humidity > 100 || rainValue > 49) && !isClothsOutside) {
    turnOnOutsideClothesGreenLED();  
  }
  }

void autoMode(int lightIntensity, int Humidity, int rainValue) {
  Serial.println(F("Manual mode off"));
  digitalWrite(LED_PIN_on, LOW);
  digitalWrite(LED_PIN_off, LOW);
  
  isAuto = true;
  Serial.println(F("Auto mode on"));
  digitalWrite(auto_ledPin, HIGH);

  if ((lightIntensity < 150 || Humidity > 100 || rainValue > 49) && isClothsOutside) {
    turnOnInsideClothesRedLED();
  } else if (!(lightIntensity < 150 || Humidity > 100 || rainValue > 49) && !isClothsOutside) {
    turnOnOutsideClothesGreenLED();
  }
}

void turnOnInsideClothesRedLED() {
  digitalWrite(inside_ledPin, HIGH);
  Serial.println(F("Now inside clothes red LED on"));
  
  isClothsOutside = false;
  digitalWrite(outside_ledPin, LOW);
  if (isClothsOutside){
    myStepper.step(1024);
    delay(1000);
    }
}

void turnOnOutsideClothesGreenLED() {
  digitalWrite(inside_ledPin, LOW);
  Serial.println(F("Now outside clothes green LED on"));
  myStepper.step(-1024);
  isClothsOutside = true;
  digitalWrite(outside_ledPin, HIGH);
  delay(1000);
}

// This function returns the analog data of Rain Sensor
int readRainSensor() {
  int sensorValue = analogRead(sensorPin);  // Read the analog value from the rain sensor
  int outputValue = map(sensorValue, 0, 1023, 255, 0); // Map the 10-bit data to 8-bit data
  //analogWrite(ledPin, outputValue); // Generate PWM signal
  return outputValue; // Return the analog rain value
}


void printToSerial(float(lightIntensity_), float(Humidity_), float(Temperature_), float(rainValue_)) {

  Serial.print(F("LDR Value ")); // Print the label
  Serial.println(lightIntensity_); // Print the ldr_value intensity value

  Serial.print(F("Humidity: ")); // Print the label
  Serial.print(Humidity_); // Print the humidity value
  Serial.println(F(" [%]\t")); // Print the unit for humidity

  Serial.print("Temperature: "); // Print the label
  Serial.print(Temperature_); // Print the temperature value
  Serial.println(" C"); // Print the unit for temperature


  Serial.print(F("RainValue ")); // Print the label
  Serial.println(rainValue_); // Print the rain sensor value
}



//////////////////////////// codes for read and write //////


void connectToWiFi() {
  // Reset ESP8266 module
  sendCommand("AT+RST", 5000, "ready");

  // Set ESP8266 to client mode
  sendCommand("AT+CWMODE=1", 2000, "OK");

  // Connect to Wi-Fi network
  Serial.println("Connecting to Wi-Fi...");
  sendCommand("AT+CWJAP=\"" + String(SSID) + "\",\"" + String(PASSWORD) + "\"", 10000, "OK");
}


bool isNumber(const String &str) {
  for (unsigned int i = 0; i < str.length(); i++) {
    if (!isdigit(str.charAt(i)) && str.charAt(i) != '.' && str.charAt(i) != '-') {
      return false;
    }
  }
  return true;
}


float readThingSpeak(String FIELD_PATH_) {
  // Make an HTTP GET request to Thingspeak for "FIELD_PATH_"
  String getRequest = "GET " + FIELD_PATH_;
  sendCommand("AT+CIPMUX=1", 5000, "OK");
  sendCommand("AT+CIPSTART=0,\"TCP\",\"" + HOST + "\"," + PORT, 10000, "OK");
  sendCommand("AT+CIPSEND=0," + String(getRequest.length() + 2), 5000, ">");
  espSerial.println(getRequest);
  delay(1000); // Give some time for the response

  // Read and parse the value from the response
  String response;
  while (espSerial.available())
  {
    char c = espSerial.read();
    response += c;
  }
  Serial.println(response);

  // Find the start and end indices of the numeric value
  int valueStartIndex = response.indexOf(":") + 1;
  int valueEndIndex = response.indexOf("0,CLOSED");
  String value_ = response.substring(valueStartIndex, valueEndIndex);

  // Remove any additional characters (e.g., newlines) from the value
  value_.trim();
  //String value;
  //if (isNumber(value_)) {
  //  float value = value_.toFloat(); // Convert the string to a float
  //} else {
  // String value = value_;
  //Serial.println("Error: Invalid input. The string is not a valid number.");

  //}
  // Serial.print("Field 2 Value: ");
  // Serial.println(value);
  float value = value_.toFloat();
  sendCommand("AT+CIPCLOSE=0", 5000, "OK");
  return value;
}


float writeThingSpeak(float Temperature_, float Humidity_, float rainValue_, float lightIntensity_) {

  String getData = "GET /update?api_key=" + API + "&field2=" + Temperature_ +
                   "&field3=" + Humidity_ + "&field4=" + rainValue_ + "&field5=" + lightIntensity_;

  sendCommand("AT+CIPMUX=1", 5000, "OK");
  sendCommand("AT+CIPSTART=0,\"TCP\",\"" + HOST + "\"," + PORT, 10000, "OK");
  sendCommand("AT+CIPSEND=0," + String(getData.length() + 4), 5000, ">");

  espSerial.println(getData);
  delay(1500);
  countTrueCommand++;
  sendCommand("AT+CIPCLOSE=0", 5000, "OK");
}


void sendCommand(String command, unsigned int timeout, const char* expected)
{
  espSerial.println(command);
  unsigned long startTime = millis();

  String response;

  while (millis() - startTime < timeout)
  {
    while (espSerial.available())
    {
      char c = espSerial.read();
      response += c;
    }
    if (response.indexOf(expected) != -1)
    {
      found = true;
      break;
    }
  }
  if (found)
  {
    Serial.println("OK");
    countTrueCommand++;
  }
  else
  {
    Serial.println("Fail");
    countTrueCommand = 0;
  }
  found = false;
}

//
//
// Sensorix
//
// Description of the project
// Developed with [embedXcode](http://embedXcode.weebly.com)
//
// Author 		DocSystems
// 				DocDevs
//
// Date			02/01/2018 21:28
// Version		0.0.3
//
// Copyright	© 2018
// Licence		licence
//
// See         ReadMe.txt for references
//


// Core library for code-sense - IDE-based
// !!! Help: http://bit.ly/2AdU7cu
#if defined(ENERGIA) // LaunchPad specific
#include "Energia.h"
#elif defined(TEENSYDUINO) // Teensy specific
#include "Arduino.h"
#elif defined(ESP8266) // ESP8266 specific
#include "Arduino.h"
#elif defined(ARDUINO) // Arduino 1.8 specific
#include "Arduino.h"
#else // error
#error Platform not defined
#endif // end IDE

#include "LiquidCrystal.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "GPRS_Shield_Arduino.h"
#include "SoftwareSerial.h"
#include "Wire.h"
#define VT_PIN A0 // voltage read-out

//GSM
#define PIN_TX    7
#define PIN_RX    8
#define BAUDRATE  9600
#define PHONE_NUMBER "+XXXXXXXXX"
#define MESSAGETEMP  "Temperatura za niska."
#define MESSAGEPOWER "Brak zasilania."
#define MESSAGEPOWERBACK "Zasilanie wrocilo."
#define MESSAGE_LENGTH 160

//flags:
bool tempMessageSent = false;
bool powerMessageSent = false;
bool hasPower = true;

//main configuration:
float tolerance1 = 13.0; //set minimum temperature here
float tolerance2 = 20.0; //not used
int delayTime = 5000; //delay of the main loop in miliseconds, 300k - 5 minutes
int powerDelay = 180000; // 180k = 3 minutes
int defaultCycles = 360; //number of cycles
int resetCycles = defaultCycles;
float tempLog[30];

//sms handling:
char message[MESSAGE_LENGTH];
int messageIndex = 0;
char phone[13];
char datetime[24];

//lcd
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//char msg = "";
// char messages[] = {"log", "status", "temp 25"};

GPRS gprsTest(PIN_TX,PIN_RX,BAUDRATE);//RX,TX,BaudRate

//Temperature sensor
#define ONE_WIRE_BUS 13

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

void setup() {
    
    //Temperature sensor
    //start serial port
    Serial.begin(9600);
    
    //locate devices on the bus
    sensors.begin();
    if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
    
    // show the addresses we found on the bus
    
    // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
    sensors.setResolution(insideThermometer, 9);
    
    //lcd
    Serial.println("Setting up lcd...");
    lcd.begin(16, 2);
}

// function to print the temperature for a device
float outputTemperature(DeviceAddress deviceAddress)
{
    float tempC = sensors.getTempC(deviceAddress);
    return tempC;
}

void printTemperature(float tempC) {
    Serial.println("Temperatura: ");
    Serial.print(tempC);
    lcd.setCursor(0, 0);
    lcd.print("Temperatura: ");
    lcd.print((char)223);
    lcd.print("C");
    lcd.print(tempC);
}

void printNoPower() {
    lcd.setCursor(0, 1);
    lcd.print("Brak zasilania.");
}

void printPowerOk(){
    lcd.setCursor(0, 1);
    lcd.print("Zasilanie OK.  ");
}

void powerUp()
// software equivalent of pressing the GSM shield "power" button
{
    if(!gprsTest.init() == true)
    {
        pinMode(9, OUTPUT);
        digitalWrite(9,LOW);
        delay(1000);
        digitalWrite(9,HIGH);
        delay(2000);
        digitalWrite(9,LOW);
        delay(3000);
    }
}

void sendMessageTemp()
{
    //GPS module
    while(!gprsTest.init()) {
        delay(1000);
        Serial.print("init error\r\n");
    }
    Serial.println("gprs init success");
    Serial.println("sending message ...");
    gprsTest.sendSMS(PHONE_NUMBER,MESSAGETEMP); //define phone number and text
}
void sendMessagePower()
{
    //GPS module
    while(!gprsTest.init()) {
        delay(1000);
        Serial.print("init error\r\n");
    }
    Serial.println("gprs init success");
    Serial.println("sending message ...");
    gprsTest.sendSMS(PHONE_NUMBER,MESSAGEPOWER); //define phone number and text
}

void sendMessagePowerBack()
{
    //GPS module
    while(!gprsTest.init()) {
        delay(1000);
        Serial.print("init error\r\n");
    }
    Serial.println("gprs init success");
    Serial.println("sending message ...");
    gprsTest.sendSMS(PHONE_NUMBER,MESSAGEPOWERBACK); //define phone number and text
}

/*
 * Main function. It requests the tempC from the sensors and display on Serial.
 */
void loop(void)
{
    Serial.println(resetCycles);
    if (resetCycles == 0){
        resetCycles = defaultCycles;
        tempMessageSent = false;
        powerMessageSent = false;
    } else {
        resetCycles--;
        
    }
    
    
    
    //turn on gps shield if not turned on
    powerUp();
    
    //check if it has power
    int vt_read = analogRead(VT_PIN);
    float voltage = vt_read * (5.0 / 1024.0) * 5.0;
    Serial.println("Checking voltage.");
    if (voltage == 0)
    {
        delay(180000);
        if (voltage == 0)
        {
            hasPower = false;
            Serial.println("Voltage is 0.");
            if(powerMessageSent == false){
                sendMessagePower();
                printNoPower();
                powerMessageSent = true;
            }
        }
        else
        {
            printPowerOk();
            hasPower = true;
        }
    }
    
    // Check received messages, delete messages
    messageIndex = gprsTest.isSMSunread();
    if (messageIndex > 0) { //At least, there is one UNREAD SMS
        gprsTest.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime);
        //In order not to full SIM Memory, is better to delete it
        gprsTest.deleteSMS(messageIndex);
        Serial.print("From number: ");
        Serial.println(phone);
        Serial.print("Recieved Message: ");
        Serial.println(message);
    }
    
    // Check temperature
    sensors.requestTemperatures();
    float tempC = outputTemperature(insideThermometer); // Use a simple function to print out the data
    
    Serial.println("Checking temperature.");
    Serial.println(tempC);
    Serial.println("Printing temperature...");
    printTemperature(tempC);
    
    if (tempC < tolerance1 && tempMessageSent == false)
    {
        sendMessageTemp()();
        tempMessageSent = true;
    }
    
    delay(delayTime);
}




void executeCommand(char message)
{
    
    
}



void getLog(void)
{
    
}
void getStatus(void)
{
    
}
void changeTemp1(float temp)
{
    tolerance1 = temp;
}
void changeTemp2(float temp)
{
    tolerance2 = temp;
}
void changeDelay(int dTime)
{
    delayTime = dTime;
}


/*
 // function to print a device address
 void printAddress(DeviceAddress deviceAddress)
 {
 for (uint8_t i = 0; i < 8; i++)
 {
 if (deviceAddress[i] < 16) Serial.print("0");
 Serial.print(deviceAddress[i], HEX);
 }
 }*/

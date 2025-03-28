#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <TinyGPSPlus.h>
#include <SPI.h>
// #include <SdFat.h>
#include <SD.h>
#include "wiring_private.h"
#include "sci_f.h"


#define lcd_msgs_count 10

// SENSIRION parameter requirement
SensirionI2CSen5x sen5x;
#define MAXBUF_REQUIREMENT 48
#if (defined(I2C_BUFFER_LENGTH) && (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
  #define USE_PRODUCT_INFO
#endif


// LCD  0x27 - uart adress 
LiquidCrystal_I2C lcd(0x27, 16, 2);
int lcd_position = 0;

// MICRO SD
const int chipSelect = SDCARD_SS_PIN;

// GPS
TinyGPSPlus gps;

 // Second serial port to cennecto to GPS
Uart GPS_Serial(&sercom3, 7, 6, SERCOM_RX_PAD_3, UART_TX_PAD_2);

void SERCOM3_Handler()
{
  GPS_Serial.IrqHandler();
}

// Button PINS
static const int but_up_pi = 10, but_dn_pi = 9;  


// Data wariables
float Pm1p0;
float Pm2p5;
float Pm4p0;
float Pm10p0;
float Humidity;
float Temperature;
float vocIndex;
float noxIndex;

float CO_ppm;
float CH4_ppm;

float Altitude;
float Longnitude;
float Latitude;

File dataFile;
String Filename;


// Whyle delay read GPS data
static void smartDelay(unsigned long ms, unsigned long ms_s=0) {
  unsigned long start = millis();
  if (ms_s != 0) {
    start = ms_s;
  }
  
  do 
  {
    while (GPS_Serial.available() > 0)
      gps.encode(GPS_Serial.read());
  } while (millis() - start < ms);
}


// Display Error on LCD + Serial
void display_error_msg(int level, String msg_1="", String msg_2="") {

  /*
    0 - DEBUG
    1 - INFO
    2 - WARN
    3 - ERROR
    4 - FATAL
  */
    
    lcd.clear();
    switch(level)
    {
      case 0:
        lcd.setCursor(3, 0);
        lcd.print("DEBUG");
        break;
      case 1:
        lcd.setCursor(3, 0);
        lcd.print("Info:");
        Serial.println("Info: ");
        break;
      case 2:
        lcd.setCursor(2, 0);
        lcd.print("Warning !!!");
        Serial.println("Warning !!: ");
        break;
      case 3:
        lcd.setCursor(3, 0);
        lcd.print("Error !!!");
        Serial.println("Error !!: ");
        break;
      case 4:
        lcd.setCursor(1, 0);
        lcd.print("Fatal Error !!");
        Serial.println("Fatal Error !!: ");
        break;
    };
    smartDelay(2000);
    lcd.clear();
    
    lcd.setCursor(0, 0);
    lcd.print(msg_1);
    lcd.setCursor(0, 1);
    lcd.print(msg_2);
    smartDelay(2000);

  if(level == 4) {
    while ((digitalRead(but_up_pi) == LOW) && (digitalRead(but_dn_pi) == LOW)) {
      delay(1);
    }
  }


  Serial.println(msg_1 + String(" ") + msg_2);
}



void setup() {
  Serial.begin(9600);
  GPS_Serial.begin(9600);

  pinMode(but_up_pi, INPUT);
  pinMode(but_dn_pi, INPUT);

  // New GPS Serial pin
  pinPeripheral(7, PIO_SERCOM_ALT); // RX
  pinPeripheral(6, PIO_SERCOM_ALT);  // TX

  // LCD init
  lcd.begin();
  lcd.backlight();

  lcd.setCursor(4, 0);
  lcd.print("Witam");

  // SENSIRION init
  Wire.begin();
  sen5x.begin(Wire);

  // SD init
  if (!SD.begin(chipSelect)) {
    display_error_msg(3, "SD card failed", "or not present !");
  }

  // Sensirion INIT 
  uint16_t error;
  char errorMessage[256];
  error = sen5x.deviceReset();
  if (error) {
    display_error_msg(3, "Trying to execute", "deviceReset() !");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  // SENSIRION ERRORS
  float tempOffset = 0.0;
  error = sen5x.setTemperatureOffsetSimple(tempOffset);
  if (error) {
    Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("Temperature Offset set to ");
    Serial.print(tempOffset);
    Serial.println(" deg. Celsius (SEN54/SEN55 only");
  }
  error = sen5x.startMeasurement();
  if (error) {
    Serial.print("Error trying to execute startMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  smartDelay(1000);
  creat_file();
}


String msg_ppm_str[] = { "Lat", "Lng", "CO_ppm", "CH4_ppm", "Pm1p0", "Pm2p5", "Pm4p0", "Pm10p0", "H", "T" };


void loop() {

  unsigned long Time_start = millis();

  // Read analog value from detectors
  // CO_ppm = analog_CO_to_R(analogRead(A0));    // K Omega
  // CH4_ppm = analog_CH4_to_R(analogRead(A2));  // K Omega
  CO_ppm = analogRead(A0);
  CH4_ppm = analogRead(A2);


  // Read Sensirion Measurement
  uint16_t error;
  char errorMessage[256];
  error = sen5x.readMeasuredValues(
    Pm1p0, Pm2p5, Pm4p0,
    Pm10p0, Humidity, Temperature, vocIndex,
    noxIndex);

  if (error) {
    display_error_msg(3, "Trying to execute", "readMeasuredValues");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    save_data_to_file();
  }

  if (digitalRead(but_up_pi) == HIGH) {
    lcd_position++;
  }

  if (digitalRead(but_dn_pi) == HIGH) {
    lcd_position--;
  }

  float msg_ppm_value[] = {Latitude, Longnitude, CO_ppm, CH4_ppm, Pm1p0, Pm2p5, Pm4p0, Pm10p0, Humidity, Temperature };

  // lcd_position %= lcd_msgs_count-1;
  if (lcd_position < 0) { lcd_position = lcd_msgs_count - 1; }
  if (lcd_position >= lcd_msgs_count - 1) { lcd_position = 0; }

  // Display LCD
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print(msg_ppm_str[lcd_position] + ": " + String(msg_ppm_value[lcd_position]));
  lcd.setCursor(2, 1);
  lcd.print(msg_ppm_str[lcd_position + 1] + ": " + String(msg_ppm_value[lcd_position + 1]));

  smartDelay(1000, Time_start);
  
}

float analog_CH4_to_R(int A_ch4) {
  float R_L_ch4 = 5.01; // K Omega
  return R_L_ch4 * (A_ch4/(1023 - A_ch4)); // K Omega
}


float analog_CO_to_R(int A_co) {
  float R_L_co = 5.06; // K Omega
  return R_L_co * (A_co/(1023 - A_co)); // K Omega
}



void creat_file() {
  String file_header = "GPS__Date_Time, Latitude, Longitude, [CH4]_ppm, [CO]_PPM, [NaOx]_Pm1p0, [NaOx]_Pm2p5, [NaOx]_Pm4p0, [NaOx]_Pm10p0, Humidity, Temperature\r\n";

  // short name due to SD library memory usage:   name < 13 byts
  Filename = gps.date.day() + String("-") + gps.time.hour() + String("_") + gps.time.minute();


  dataFile = SD.open(Filename,FILE_WRITE);

  if (dataFile) {

    dataFile.println(file_header);
    dataFile.close();
  }

  else {
    display_error_msg(3, "Opening data", "Fail failded !");
  }
},


void save_data_to_file() {

  dataFile = SD.open(Filename,FILE_WRITE);
  
  // Read location from GPS
  if(gps.location.isValid())
  {
    Latitude = gps.location.lat();
    Longnitude = gps.location.  ();
  }
  else
  {
    Latitude = 0;
    Longnitude = 0;
  }

  float data_values[] = {Longnitude, Latitude, CO_ppm, CH4_ppm, Pm1p0, Pm2p5, Pm4p0, Pm10p0, Humidity, Temperature };

  String Dataline = gps.date.day() + String("/") + gps.date.month() + String("/") + gps.date.year() + String(" ") + gps.time.hour() + String(":") + gps.time.minute() + String(":") +
  gps.time.second() + String(", ");

  for (int i = 0; i < 9; i++) {
    Dataline += sci(data_values[i], 5);
    Dataline += ", ";
  }

  Dataline += sci(data_values[9], 5);

  Serial.println(Dataline);

  if (dataFile) {
    dataFile.println(Dataline);
  }
  else {
    // Serial.println("No data File !!!");
    display_error_msg(3, "Fail write", "to file!");
  }

  dataFile.close();
}

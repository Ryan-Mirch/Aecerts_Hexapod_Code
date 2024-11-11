#include <SPI.h>
#include <RF24.h>
#define Current_Sensor_Pin 0
#define UNPRESSED 0x1
#define PRESSED  0x0

RF24 radio(49, 48); // CE, CSN
uint8_t address[6] = "HEX01";
bool radioNumber = 1;

unsigned long rc_last_received_time = 0;
unsigned long rc_timeout = 1000;

enum PackageType {
    CONTROL_DATA = 1,
    SETTINGS_DATA = 2
};

// Define the data packages
struct RC_Control_Data_Package {
    byte type; // 1 byte
    
    byte joy1_X; // 1 byte
    byte joy1_Y; // 1 byte
    
    byte joy2_X; // 1 byte
    byte joy2_Y; // 1 byte  
    byte slider1; // 1 byte
    byte slider2; // 1 byte

    byte joy1_Button:1; // 1 bit
    byte joy2_Button:1; // 1 bit
    byte pushButton1:1; // 1 bit
    byte pushButton2:1; // 1 bit
    byte idle:1;        // 1 bit
    byte sleep:1;        // 1 bit
    byte dynamic_stride_length:1; // 1 bit
    byte reserved : 1;  // 1 bits padding, 1 byte total

    byte gait;  // 1 byte
};

struct RC_Settings_Data_Package {
    byte type;
    
    byte calibrating:1; //1 bit
    byte reserved:7;              //7 bits padding, 1 byte total

    int8_t offsets[18];             //18 bytes
};

struct Hexapod_Data_Package {
  float current_sensor_value;
  int8_t offsets[18];
};

RC_Control_Data_Package rc_control_data;
RC_Settings_Data_Package rc_settings_data;
Hexapod_Data_Package hex_data;

void RC_Setup();
void RC_DisplayData();
void SendData();
bool GetData();
void initializeHexPayload();
void initializeControllerPayload();
void processControlData(const RC_Control_Data_Package& data);
void processSettingsData(const RC_Settings_Data_Package& data);

void RC_Setup(){
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {} // hold in infinite loop
  } else {
    Serial.println(F("radio hardware is ready!"));
  }

  radio.setAddressWidth(5);
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(32); // Set the payload size to the maximum of 32 bytes
  radio.setChannel(124);
  radio.openReadingPipe(1, address);
  radio.enableAckPayload();
  radio.startListening();

  initializeHexPayload();  
  initializeControllerPayload();

  radio.writeAckPayload(1, &hex_data, sizeof(hex_data)); 
}

void initializeHexPayload(){
  hex_data.current_sensor_value = 0;

  Serial.println("Filling hex_data.offsets with 0's.");
  for (int i = 0; i < 18; i++) {
      hex_data.offsets[i] = 0;
  }
}

void initializeControllerPayload(){

  //control package
  rc_control_data.joy1_X = 127;
  rc_control_data.joy1_Y = 180;
  rc_control_data.joy1_Button = UNPRESSED;

  rc_control_data.joy2_X = 127;
  rc_control_data.joy2_Y = 127;
  rc_control_data.joy2_Button = UNPRESSED;

  rc_control_data.slider1 = 40;
  rc_control_data.slider2 = 0;

  rc_control_data.pushButton1 = UNPRESSED;
  rc_control_data.pushButton2 = UNPRESSED;

  rc_control_data.idle = 1;
  rc_control_data.sleep = 1;

  rc_control_data.gait = 0;

  rc_control_data.dynamic_stride_length = 1;


  //settings package
  rc_settings_data.calibrating = 0;
  

  for (int i = 0; i < 18; i++) {
      rc_settings_data.offsets[i] = -128;
  }

}
byte currentType;
bool GetData(){  
  // This device is a RX node
  uint8_t pipe;
  if (radio.available(&pipe)) {
    uint8_t bytes = radio.getPayloadSize(); // get the size of the payload    
    byte incomingType;
    radio.read(&incomingType, sizeof(incomingType));
    if(currentType != incomingType && incomingType != NULL)currentType = incomingType;

    if (incomingType == CONTROL_DATA) {
        radio.read(&rc_control_data, sizeof(rc_control_data));
    } else if (incomingType == SETTINGS_DATA) {
        radio.read(&rc_settings_data, sizeof(rc_settings_data));
    }   

    hex_data.current_sensor_value = mapFloat(analogRead(Current_Sensor_Pin), 0, 1024, 0, 50);      
    radio.writeAckPayload(1, &hex_data, sizeof(hex_data)); // load the payload for the next time

    rc_last_received_time = millis();    
  }

  if (millis() - rc_last_received_time > rc_timeout) return false;

  return true;
}



void RC_DisplayData(){
  Serial.print("Joy1 X: ");
  Serial.print(rc_control_data.joy1_X);

  Serial.print(" | Joy1 Y: ");
  Serial.print(rc_control_data.joy1_Y);

  Serial.print(" | Joy1 Button: ");
  Serial.print(rc_control_data.joy1_Button);

  Serial.print(" | Joy2 X: ");
  Serial.print(rc_control_data.joy2_X);

  Serial.print(" | Joy2 Y: ");
  Serial.print(rc_control_data.joy2_Y);

  Serial.print(" | Joy2 Button: ");
  Serial.print(rc_control_data.joy2_Button);

  Serial.print(" | Pot 1: ");
  Serial.print(rc_control_data.slider1);

  Serial.print(" | Pot 2: ");
  Serial.print(rc_control_data.slider2);

  Serial.print(" | Button 1: ");
  Serial.print(rc_control_data.pushButton1);

  Serial.print(" | Button 2: ");
  Serial.println(rc_control_data.pushButton2);
}
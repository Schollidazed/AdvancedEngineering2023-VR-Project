//FINAL-TS4231.ino
//CODE WRITTEN BY SAM HOLLIDAY - CHICKENWINGJOHNNY - ARGS 22-23 Advanced Engineering
//Purpose - To send controller data about "Sweep times" and Buttons over serial.

#include "Header/ts4231.h"
#include "Header/ts4231.cpp"

#define SerialTX                Serial1

int sensor = 0;
int lighthouse = 0;
int axis = 0;
int sweep = 0;

void setup(){
    while(!SerialTX){}

    //Serial.begin(115200);
    SerialTX.begin(9600);
    Serial.begin(9600);

    //Serial.println("Hello, please wait...");

    delay(500);
    SerialTX.println("AT");
    delay(500);
    SerialTX.println("AT+NAMEADV-ENG2023"); 
    delay(500);
    //0 = Slave, 1 = Master
    SerialTX.println("AT+ROLE0");
    // 0: 9600, 1: 19200, 2: 38400, 3: 57600, 4: 115200, 5: 4800, 6: 2400, 7: 1200, 8: 230400
    SerialTX.println("AT+BAUD0");

    //Serial.println("READY");
}


void loop(){
    
    sweep++;
    if (sweep >= 8000){
        sweep = 0;
    }

    unsigned int data = 0;
    
    data |= sensor;
    data <<= 1;
    data |= lighthouse;
    data <<= 1;
    data |= axis;
    data <<= 24;
    data |= sweep;
    
    // SerialTX.write(data);

    String message = String(data);

    // for (int i = 31; i >= 0; i--) {
    //     int bit = (data >> i) & 1;
    //     message.concat(bit);
    // }
    message.concat(";");
    SerialTX.print(message);
    Serial.println(message);
    
   //SerialTX.write(0x12);
   //SerialTX.println("ARGGGHH");
   //BLE sends text through hex code.
   delay(8);
}   
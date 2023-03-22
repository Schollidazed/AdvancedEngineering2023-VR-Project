//FINAL-TS4231.ino
//CODE WRITTEN BY SAM HOLLIDAY - CHICKENWINGJOHNNY - ARGS 22-23 Advanced Engineering
//Purpose - To send controller data about "Sweep times" and Buttons over serial.
//NOTE: In order for this to properly work, both lighthouses must be active.
#include "Header/ts4231.h"
#include "Header/ts4231.cpp"

#define ts4231_LIGHT_TIMEOUT        3000 
#define MicrosecondsToTicks         48
#define LowestTickCount             2501
#define TickCountRange              500
#define MAX_LIGHTHOUSE_SWEEPTIME    8333

#define SerialTX                Serial1

///////////////////////////////////////////
///                                     ///
///             *SENSORS*               ///
///                                     ///
///////////////////////////////////////////

    //E pin then D pin.
    int EDPins[][2] = {{53, 23}/*, {}, {}, {}, {}, {}, {}*/};
    //When pin drops low, that's the start of an IR signal.
    TS4231 sensor0(EDPins[0][0], EDPins[0][1]);
    //TS4231 sensor1(EDPins[1][0], EDPins[1][1]);
    //TS4231 sensor2(EDPins[2][0], EDPins[2][1]);
    //TS4231 sensor3(EDPins[3][0], EDPins[3][1]);
    //TS4231 sensor4(EDPins[4][0], EDPins[4][1]);
    //TS4231 sensor5(EDPins[5][0], EDPins[5][1]);
    //TS4231 sensor6(EDPins[6][0], EDPins[6][1]);


    //Stores all the memory addresses of each sensor
    //Be sure to dereference when calling! Use the "->" operator. 
    TS4231* sensorList[] = {&sensor0/*, &sensor1, &sensor2, &sensor3, &sensor4, &sensor5, &sensor6*/};
    int sensorListLength = sizeof(sensorList)/sizeof(sensorList[0]);


///////////////////////////////////////////
///                                     ///
///             *OOTX Pulse*            ///
///                                     ///
///////////////////////////////////////////

    //OOTX = Omnidirectional Optical Transmitter --- the flashes and sweeps that happen. 
    //Stores data about the current pulse
    // - Variables: Defined, StartTime, EndTime, Axis, Data, Skip
    // - Methods: Clear()
    struct OotxPulseInfo {
        bool defined = false; //Has this pulse been fully fleshed out?
        int startTime = 0; //Time when pin is activated
        int endTime = 0; //Time when pin is deactivated
        bool axis = false; //X or Y Axis
        bool data = false; //Lighthouse encodes a message thru this.
        bool skip = false; //If this is true, then it disregards the object.

        //Resets everything in the object to the default value.
        void clear(){
            defined = false;
            startTime = 0;
            endTime = 0;
            axis = false;
            data = false;
            skip = false;
        }
    };

    //There's only 2 OOTX signals before a sweep, master will always flash first, and slave will follow.
    //These should be global
    OotxPulseInfo OOTX_MASTER;
    OotxPulseInfo OOTX_SLAVE;
    unsigned int sweepTime;


///////////////////////////////////////////
///                                     ///
///          *Circular Buffer*          ///
///                                     ///
///////////////////////////////////////////


    //This MUST be a power of two. The optimisation within the Ring buffer uses modulus, 
    //which when optimised, uses a BITWISE AND.
    #define DATA_CIRC_BUFFER_LENGTH    32
    #define CIRC_BUFFER_MODULUS_MASK (DATA_CIRC_BUFFER_LENGTH - 1)
    //unsigned int = 32 bit number 
    //first 6 bits -> Sensor 
    //next bit -> Lighthouse (0 = master, 1 = slave)
    //next bit -> Axis (0 = X, 1 = Y)
    //Rest of 24 = dtime from sweep. (This should be no more than 8333 microseconds)
    //if a value == 0, then no data is there.
    volatile unsigned int dataCircularBuffer[DATA_CIRC_BUFFER_LENGTH];
    int bufferFillLength = 0;
    int bufferWriteIndex = 0;
    int bufferReadIndex = 0;


void setup(){
    while(!SerialTX){}

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

    TS4231_configSensors();

    Serial.println();
    Serial.print("Number of Sensors ");
    Serial.println(sensorListLength);

    Serial.println("READY");

    delay(1000);

    TS4231_attachIntterupts();
}

//One cycle of Master/Slave loop should be about 30Hz, or a period of 0.033s
//Master Lighthouse gives its 2 updates in the first 0.0167s, then the Slave's 2 in the second 0.167s.
//Cycle is always: Master, Slave, Sweep. (Repeat)
//NOTE: The sweep time cannot be larger than 1/120th of a second (8333 microseconds), so discard those any bigger than that.
void loop(){
    //Find Sensor from sensorList that has Triggered Interrupt. Check if a change HAS occured. 
    //Check through every sensor, and find which one triggered the intterupt
    for(int i = 0; i < sensorListLength; i++){
        //Interrupt must be triggered, both lowToHighTime and highToLowTime must be nonzero, 
        //AND lowToHighTime must be more recent. 
        //Which change has occured?
        if(sensorList[i]->interruptTriggered && 
                sensorList[i]->highToLowTime && 
                sensorList[i]->lowToHighTime && 
                sensorList[i]->lowToHighTime > sensorList[i]->highToLowTime){
            //Only one of these conditions can be activated per sensorList
            //IF BUFFER IS FULL GET OUTTA THERE AND READDD PULEEEASEEEEUH
            if(bufferFillLength == DATA_CIRC_BUFFER_LENGTH){
                break;
            }
            
            //IF MASTER not defined, MASTER must be defined here
            else if (!OOTX_MASTER.defined){
                extractOOTXData(sensorList[i], &OOTX_MASTER);
                clearAllSensorData(); //Gets rid of all the sensor data, to prevent unintended calculations + reset the playing field
            }
            
            //IF SLAVE not defined, SLAVE must be defined here
            else if (OOTX_MASTER.defined && !OOTX_SLAVE.defined){
                extractOOTXData(sensorList[i], &OOTX_SLAVE);
                clearAllSensorData();
            }

            //Need to create conditions where...
            //  SLAVE cannot be found, and the current setting is a sweep
            //  MASTER cannot be found, and the current setting is a sweep
            //But we only know that it's master/slave from it going in order. 
            //If the object goes out of frame of one lighthouse, it's impossible to tell whether the incoming signal is slave or master...
            //Maybe by finding the time elapsed between signals, we can determine whether it's a slave or master signal...
            
            //IF Both SLAVE and MASTER are defined
            //Must be a sweep.
            else if (OOTX_MASTER.defined && OOTX_SLAVE.defined){
                if(!OOTX_MASTER.skip){
                    //this is the duration from the start of the sweep.
                    sweepTime = sensorList[i]->highToLowTime - OOTX_MASTER.startTime;
                    //SensorID, then LighthouseID, then AxisID, then the sweepTime
                    //Gets the latest write position, and adds this number to the buffer to be sent.
                    if(sweepTime <= MAX_LIGHTHOUSE_SWEEPTIME){
                        dataCircularBuffer[bufferWriteIndex & CIRC_BUFFER_MODULUS_MASK] = 
                            (i << 26) | (0 << 25) | (OOTX_MASTER.axis << 24) | sweepTime;
                        bufferWriteIndex++;
                        bufferFillLength++;
                    }
                    //SerialTX.print("Sweep Time - ");
                    //SerialTX.println(sweepTime);
                }
                else{
                    sweepTime = sensorList[i]->highToLowTime - OOTX_SLAVE.startTime;
                    if(sweepTime <= MAX_LIGHTHOUSE_SWEEPTIME) {
                        dataCircularBuffer[bufferWriteIndex & CIRC_BUFFER_MODULUS_MASK] = 
                            (i << 26) | (1 << 25) | (OOTX_SLAVE.axis << 24) | sweepTime;
                        bufferWriteIndex++;
                        bufferFillLength++;
                    }
                    //Serial.print("Sweep Time - ");
                    //Serial.println(sweepTime);
                }
                //This is the end, reset OOTX pulses, and restart the cycle.
                if(i == sensorListLength - 1){
                    OOTX_MASTER.clear();
                    OOTX_SLAVE.clear();
                    clearAllSensorData();
                }
            }

        }
        //All options have been exahusted, must be set back to false.
        sensorList[i]->interruptTriggered = false;
    }
    //Now defined, we send a byte of data over SerialTX.
    sendData();
}   

void TS4231_configSensors(){
    //Iterate through sensorList, configing them.
    for(int i = 0; i < sensorListLength; i++){
        uint8_t config_result;
        bool isSuccess = false;

        while (!isSuccess){
            if (sensorList[i]->waitForLight(ts4231_LIGHT_TIMEOUT)) {
                //Execute this code when light is detected
                Serial.println("Light DETECTED");
                config_result = sensorList[i]->configDevice();
                //user can determine how to handle each return value for the configuration function
                switch (config_result) {
                    case CONFIG_PASS:
                        Serial.println("Configuration SUCCESS");
                        isSuccess = true;
                        break;
                    case BUS_FAIL:  //unable to resolve state of TS4231 (3 samples of the bus signals resulted in 3 different states)
                        Serial.println("Configuration Unsuccessful - BUS_FAIL");
                        break;
                    case VERIFY_FAIL:  //configuration read value did not match configuration write value, run configuration again
                        Serial.println("Configuration Unsuccessful - VERIFY_FAIL");
                        break;
                    case WATCH_FAIL:  //verify succeeded but entry into WATCH mode failed, run configuration again
                        Serial.println("Configuration Unsuccessful - WATCH_FAIL");
                        break;
                    default:  //value returned was unknown
                        Serial.println("Program Execution ERROR");
                        break;
                    }
            } else {
            //insert code here for no light detection
            Serial.println("Light TIMEOUT");
            }
        Serial.println("");
        }
    }
}
void TS4231_attachIntterupts(){
    //Just copy and paste this, only change the first value in the list, and iterate the ISR up.
    attachInterrupt(digitalPinToInterrupt(EDPins[0][0]), ISR_sensor0, CHANGE);
    //attachInterrupt(digitalPinToInterrupt(EDPins[1][0]), ISR_sensor1, CHANGE);
}

//ISRs can't take parameters or serial communications :(
void ISR_sensor0(){
    unsigned int time = micros();
    sensor0.isLow = !digitalRead(sensor0.E_pin);
    sensor0.interruptTriggered = true;

    //True will be low.
    //If it is low, sets the highToLowTime to now.
    //If it is high, sets the lowToHighTime to now.
    if (sensor0.isLow)
        sensor0.highToLowTime = time;
    else
        sensor0.lowToHighTime = time;
}
/*void ISR_sensor1(){
    unsigned int time = micros();
    sensor1.isLow = !digitalRead(sensor1.E_pin);
    sensor1.interruptTriggered = true;

    //True will be low.
    //If it is low, sets the highToLowTime to now.
    //If it is high, sets the lowToHighTime to now.
    if (sensor1.isLow)
        sensor1.highToLowTime = time;
    else
        sensor1.lowToHighTime = time;
}*/

void sendData(){
    //Modulo operator, so that we can loop through everything.
    unsigned int data = dataCircularBuffer[bufferReadIndex & CIRC_BUFFER_MODULUS_MASK];
    //If the current data == 0 (Should only happen at beginning), or read has reached write
    if((data == 0) || (bufferReadIndex >= bufferWriteIndex)){
        return;
    }

    //Send data over chosen Serial port
    SerialTX.print(data);
    SerialTX.print(";");
    //This line's just for debug...
    Serial.println(data);
    
    //Example Breakdown:
    // int sensor = 0;
    // int lighthouse = 0;
    // int axis = 0;
    // int sweep = 0;
    // //Start from most significant bit, and go down. i controls the bitshift
    // for (int i = 31; i >= 0; i--) {
    //     int bit = (data >> i) & 1;
    //     //SerialTX.print(bit);
    //     if(i >= 26){
    //         sensor <<= 1;
    //         sensor |= bit;
    //     } else if(i == 25){
    //         lighthouse |= bit;
    //     } else if(i == 24){
    //         axis |= bit;
    //     } else{
    //         sweep <<= 1;
    //         sweep |= bit;
    //     }
    // }
    // Serial.print(" Sensor - ");
    // Serial.print(sensor);
    // Serial.print(" Lighthouse - ");
    // Serial.print(lighthouse);
    // Serial.print(" Axis - ");
    // Serial.print(axis);
    // Serial.print(" Sweep Time - ");
    // Serial.print(sweep);
    // Serial.println();
    
    bufferReadIndex++;
    bufferFillLength--;
}

//Takes the delta time, and exports the OOTX data to the OOTX at the pointed address. 
//This shouldn't happen if there's already OOTX data logged.
//lowToHighTime must be > highToLowTime
//Also includes a check such that sweeps are not counted as OOTX pulses, and if one is read within this, the entire OOTX dataset is cleared.
//Precondition: OOTX_MASTER or OOTX_SLAVE must not be defined.
void extractOOTXData(TS4231* sensor, OotxPulseInfo* OOTX){
    //Looking for the period of time where it's active. HighTime should be more recent.
    OOTX->startTime = sensor->highToLowTime;
    OOTX->endTime = sensor->lowToHighTime;
    int OOTXData = deltaTime_to_OOTXData(OOTX->endTime - OOTX->startTime);
    //SerialTX.print("OOTX DATA - ");
    //SerialTX.println(OOTXData);
    //We don't want to mistake a sweep with an OOTX var, or to have a long OOTX data value.
    //Only place this could happen is if it's just booted up.
    if(OOTXData < 0 || OOTXData > 7){
        //if OOTX data < 0, is a sweep. and if it's a sweep, we want to reset everything.
        //The only place where a sweep is valid is where both are defined, so if one is not defined, clear them all.
        OOTX_MASTER.clear();
        OOTX_SLAVE.clear();
        clearAllSensorData();
        return;
    }
    
    OOTX->axis = OOTXData & 0b0001;
    //OOTX->data = OOTXData & 0b0010;
    OOTX->skip = OOTXData & 0b0100;
    OOTX->defined = true;
}

//Converts dTime to OOTX number
int deltaTime_to_OOTXData(unsigned long dtime){
    int ticks = dtime * MicrosecondsToTicks;

    return((ticks-LowestTickCount)/TickCountRange);
}

//Clears all sensor data:
//InterruptTriggered, highToLowTime, and lowToHighTime.
//NOT the OOTX data. 
void clearAllSensorData(){
    for(int i = 0; i < sensorListLength; i++){
        sensorList[i]->interruptTriggered = false;
        sensorList[i]->highToLowTime = 0;
        sensorList[i]->lowToHighTime = 0;
    }
}
//FINAL-TS4231.ino
//CODE WRITTEN BY SAM HOLLIDAY - CHICKENWINGJOHNNY - ARGS 22-23 Advanced Engineering
//Purpose - To send controller data about "Sweep times" and Buttons over serial.

#include "Header/ts4231.h"
#include "Header/ts4231.cpp"

#define ts4231_LIGHT_TIMEOUT    3000 
#define MicrosecondsToTicks     48
#define LowestTickCount         2501
#define TickCountRange          500

#define SerialTX                Serial

///////////////////////////////////////////
///                                     ///
///             *SENSORS*               ///
///                                     ///
///////////////////////////////////////////

    //E pin then D pin.
    int EDPins[][2] = {{25, 23}, {53, 51}};
    //When pin drops low, that's the start of an IR signal.
    TS4231 sensor0(EDPins[0][0], EDPins[0][1]);
    TS4231 sensor1(EDPins[1][0], EDPins[1][1]);

    //Stores all the memory addresses of each sensor
    //Be sure to dereference when calling! Use the "->" operator. 
    TS4231* sensorList[] = {&sensor0, &sensor1};
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
    #define DATA_CIRC_BUFFER_LENGTH    16
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

    SerialTX.begin(115200);
    TS4231_configSensors();

    SerialTX.println();
    SerialTX.print("Number of Sensors ");
    SerialTX.println(sensorListLength);

    SerialTX.println("READY");

    delay(2000);

    TS4231_attachIntterupts();
}

//One cycle of Master/Slave loop should be about 30Hz, or a period of 0.033s
//Master Lighthouse gives its 2 updates in the first 0.0167s, then the Slave's 2 in the second 0.167s.
//Cycle is always: Master, Slave, Sweep. (Repeat)
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
            //IF BUFFER IS FULL GET OUTTA THERE AND READDD
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
            
            //IF Both SLAVE and MASTER are defined
            //Must be a sweep.
            else if (OOTX_MASTER.defined && OOTX_SLAVE.defined){
                if(!OOTX_MASTER.skip){
                    //this is the duration from the start of the sweep.
                    sweepTime = sensorList[i]->highToLowTime - OOTX_MASTER.startTime;
                    //SensorID, then LighthouseID, then AxisID, then the sweepTime
                    //Gets the latest write position, and adds this number to the buffer to be sent.
                    dataCircularBuffer[bufferWriteIndex & CIRC_BUFFER_MODULUS_MASK] = 
                            (i << 26) | (0 << 25) | (OOTX_MASTER.axis << 24) | sweepTime;
                }
                else{
                    sweepTime = sensorList[i]->highToLowTime - OOTX_SLAVE.startTime;
                    //SensorID, then LighthouseID, then AxisID, then the sweepTime
                    //Gets the latest write position, and adds this number to the buffer to be sent.
                    dataCircularBuffer[bufferWriteIndex & CIRC_BUFFER_MODULUS_MASK] = 
                            (i << 26) | (1 << 25) | (OOTX_SLAVE.axis << 24) | sweepTime;
                }
                bufferWriteIndex++;
                bufferFillLength++;
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
    attachInterrupt(digitalPinToInterrupt(EDPins[1][0]), ISR_sensor1, CHANGE);
}

//ISRs can't take parameters :(
void ISR_sensor0(){
    unsigned int time = micros();
    sensor0.isLow = !digitalRead(sensor0.E_pin);
    sensor0.interruptTriggered = true;
    //Not included to improve speed
    //Serial.println("Sensor ");
    //Serial.print(sensorID);
    //Serial.print(" just changed to ");
    //Serial.print(pinRead);

    //True will be low.
    //If it is low, sets the highToLowTime to now.
    //If it is high, sets the lowToHighTime to now.
    if (sensor0.isLow)
        sensor0.highToLowTime = time;
    else
        sensor0.lowToHighTime = time;
}
void ISR_sensor1(){
    unsigned int time = micros();
    sensor1.isLow = !digitalRead(sensor1.E_pin);
    sensor1.interruptTriggered = true;
    //Not included to improve speed
    //Serial.println("Sensor ");
    //Serial.print(sensorID);
    //Serial.print(" just changed to ");
    //Serial.print(pinRead);

    //True will be low.
    //If it is low, sets the highToLowTime to now.
    //If it is high, sets the lowToHighTime to now.
    if (sensor1.isLow)
        sensor1.highToLowTime = time;
    else
        sensor1.lowToHighTime = time;
}

void sendData(){
    //Modulo operator, so that we can loop through everything.
    unsigned int data = dataCircularBuffer[bufferReadIndex & CIRC_BUFFER_MODULUS_MASK];
    //If the current data == 0 (Should only happen at beginning), or read has reached write
    if((data == 0) || (bufferReadIndex >= bufferWriteIndex)){
        return;
    }

    //Send data over chosen Serial port
    SerialTX.print("Data sent: ");
    int sensor = 0;
    int lighthouse = 0;
    int axis = 0;
    int sweep = 0;
    //Start from most significant bit, and go down. i controls the bitshift
    for (int i = 31; i >= 0; i--) {
        int bit = (data >> i) & 1;
        //SerialTX.print(bit);
        
        if(i >= 26){
            sensor << 1;
            sensor | bit;
        } else if(i == 25){
            lighthouse | bit;
        } else if(i == 24){
            axis | bit;
        } else{
            sweep << 1;
            sweep | bit;
        }
    }
    /*
    SerialTX.print(" Sensor - ");
    SerialTX.print(sensor);
    SerialTX.print(" Lighthouse - ");
    SerialTX.print(lighthouse);
    SerialTX.print(" Axis - ");
    SerialTX.print(axis);
    SerialTX.print(" Sweep Time - ");
    SerialTX.print(sweep);
    */
    sprintf(new char[64], "Sensor - %hu; Lighthouse - %hu; Axis - %hu; Sweep Time - %hu", 
            sensor, lighthouse, axis, sweep);
    SerialTX.println();

    //SerialTX.println(dataCircularBuffer[bufferReadIndex & CIRC_BUFFER_MODULUS_MASK], BIN);
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

    //We don't want to mistake a sweep with an OOTX var, or to have a long OOTX data value.
    //Only place this could happen is if it's just booted up.
    if(OOTXData < 0 || OOTXData > 7){
        //if OOTX data < 0, is a sweep. and if it's a sweep, we want to reset everything.
        //The only place where a sweep is valid is where both are defined, so if one is not defined, clear them all.
        OOTX_MASTER.clear();
        OOTX_SLAVE.clear();
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
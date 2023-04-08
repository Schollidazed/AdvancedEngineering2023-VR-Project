//FINAL-TS4231.ino
//CODE WRITTEN BY SAM HOLLIDAY - CHICKENWINGJOHNNY - ARGS 22-23 Advanced Engineering
//Purpose - To send controller data about "Sweep times" and Buttons over serial.
//NOTE: In order for this to properly work, both lighthouses must be active.
#include "Header/ts4231.h"
#include "Header/ts4231.cpp"
#include <string.h>

#define SerialTX                Serial1

#define NUM_SENSORS                     2
#define TS4231_LIGHT_TIMEOUT            1000                            //in mS
#define MICROSECONDS_TO_TICKS           48.0                            //in Ticks/uS
#define LOWEST_TICK_COUNT               2501.0                          //In uS
#define TICK_COUNT_RANGE                500.0                           //In uS
#define MICROS_ADJUSTMENT               3.0                             //In uS
#define MIN_LIGHTHOUSE_SWEEPTIME        550                             //In uS (415 [TIME FROM MASTER BEGINNING TO SLAVE BEGINNING] + 135 [MAX OOTX SIGNAL]) 
#define MAX_LIGHTHOUSE_SWEEPTIME        8300                            //In uS (Meant to be 8333, but is slightly lower to prevent wacky hyjinks)
#define MAX_TIME_BTWN_OOTX              4.0                             //In uS
#define DELTA_THRESHOLD                 1000/MICROSECONDS_TO_TICKS       //In uS

//The lengths of both Ring Buffers MUST be a power of two. The optimisation within both use a BITWISE AND instead of a Modulus.

#define SCHEDULE_BUFFER_LENGTH          256
#define SCHEDULE_BUFFER_MODULUS_MASK    (SCHEDULE_BUFFER_LENGTH - 1)
#define DATA_CIRC_BUFFER_LENGTH         64
#define DATA_BUFFER_MODULUS_MASK        (DATA_CIRC_BUFFER_LENGTH - 1)

///////////////////////////////////////////
///                                     ///
///             *SENSORS*               ///
///                                     ///
///////////////////////////////////////////

    //E pin then D pin.
    int EDPins[][2] = {{53, 51}, {52, 50} /*,{}, {}, {}, {}, {}*/};
    //When pin drops low, that's the start of an IR signal.
    TS4231 sensor0(EDPins[0][0], EDPins[0][1]);
    TS4231 sensor1(EDPins[1][0], EDPins[1][1]);
    //TS4231 sensor2(EDPins[2][0], EDPins[2][1]);
    //TS4231 sensor3(EDPins[3][0], EDPins[3][1]);
    //TS4231 sensor4(EDPins[4][0], EDPins[4][1]);
    //TS4231 sensor5(EDPins[5][0], EDPins[5][1]);
    //TS4231 sensor6(EDPins[6][0], EDPins[6][1]);

    //Stores all the memory addresses of each sensor
    //Be sure to dereference when calling! Use the "->" operator. 
    TS4231* sensorList[] = {&sensor0, &sensor1/*, &sensor2, &sensor3, &sensor4, &sensor5, &sensor6*/};

    //Includes the Sensor, the highToLow time, the lowToHigh time, and the last pulse time.
    class SensorPackage {
        public: 
            volatile int sensor;
            volatile long highToLowTime;
            volatile long lowToHighTime;
            volatile long lastPulseTime;

        SensorPackage()
        {
            sensor = -1;
            highToLowTime = 0;
            lowToHighTime = 0;
            lastPulseTime = 0;
        }
    };

    //Ring Buffer! Like the Circular Buffer below :)
    SensorPackage sensorSchedule[SCHEDULE_BUFFER_LENGTH];
    volatile int scheduleWriteIndex = 0;
    volatile int scheduleReadIndex = 0;

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
    int sweepTime;
    int totTime;
    bool lighthouseID;

///////////////////////////////////////////
///                                     ///
///      *Circular DATA Buffer*         ///
///                                     ///
///////////////////////////////////////////

    //Unsigned int = 32 bit number 
    //First 6 bits -> Sensor 
    //Next bit -> Lighthouse (0 = master, 1 = slave)
    //Next bit -> Axis (0 = X, 1 = Y)
    //Rest of 24 = dtime from sweep. (This should be no more than 8333 microseconds)
    //if a value == 0, then no data is there.
    volatile unsigned int dataCircularBuffer[DATA_CIRC_BUFFER_LENGTH];
    volatile int bufferFillLength = 0;
    volatile int bufferWriteIndex = 0;
    volatile int bufferReadIndex = 0;

void setup(){
    while(!SerialTX){}

    SerialTX.begin(9600);
    Serial.begin(115200);

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
    // SerialTX.end();
    // SerialTX.begin(115200);
    
    //Fills up the schedule with blank schedules.
    for(int i = 0; i < SCHEDULE_BUFFER_LENGTH; i++){
        sensorSchedule[i] = SensorPackage();
    }

    TS4231_configSensors();

    Serial.println();
    Serial.print("Number of Sensors ");
    Serial.println(NUM_SENSORS);

    Serial.println("READY");

    delay(1000);

    TS4231_attachIntterupts();
}

//One cycle of an entire Master/Slave loop should be about 30Hz, or a period of 0.033s
//Master Lighthouse gives its 2 updates in the first 0.0167s, then the Slave's 2 in the second 0.167s.
//Cycle is always: Master, Slave, Sweep. (Repeat)
//NOTE: The sweep time cannot be larger than 1/120th of a second (8333 microseconds), so discard those any bigger than that.
void loop(){
    // unsigned int startLoop = micros();
    Serial.println("______"); 
    //DEBUG: debug();

    //Read index will only increment if this is true;
    if (sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor >= 0){
        pollOOTX();
    }
    //Now defined, we send a byte of data over SerialTX.
    sendData();

    //Serial.print("DELTA: ");
    //Serial.println(micros() - startLoop);
}   

void debug(){
    Serial.print("R: ");
    Serial.println(scheduleReadIndex);
    Serial.print("W: ");
    Serial.println(scheduleWriteIndex);

    Serial.print("S: ");
    Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor);
    Serial.print("ActTime: ");
    Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime);
    Serial.print("DActTime: ");
    Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime);
    Serial.print("PSActTime: ");
    Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lastPulseTime);
    Serial.print("dTime: ");
    Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime - sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime);
    Serial.print("Btwn: ");
    Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lastPulseTime);

    scheduleReadIndex++;
    cleanseSchedule();
}

void TS4231_configSensors(){
    //Iterate through sensorList, configing them.
    for(int i = 0; i < NUM_SENSORS; i++){
        uint8_t config_result;
        bool isSuccess = false;

        while (!isSuccess){
            if (sensorList[i]->waitForLight(TS4231_LIGHT_TIMEOUT)) {
                //Execute this code when light is detected
                Serial.print("Sensor ");
                Serial.print(i);
                Serial.println(" Light DETECTED");
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

//ISRs can't take parameters or serial communications :(
void ISR_sensor0(){
    unsigned int time = micros();
    //If it is low, sets the highToLowTime to now. and the last pulse time to highToLowTime
    //If it is high, sets the lowToHighTime to now.
    if (!digitalRead(sensor0.E_pin)){
        sensor0.lastPulseTime = sensor0.highToLowTime;
        sensor0.highToLowTime = time;
    }
    else{
        sensorSchedule[scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor = 0;
        sensorSchedule[scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime = sensor0.highToLowTime;
        sensorSchedule[scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime = time;
        sensorSchedule[scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK].lastPulseTime = sensor0.lastPulseTime;
        scheduleWriteIndex++;
    }
}
void ISR_sensor1(){
    unsigned int time = micros();
    if (!digitalRead(sensor1.E_pin)){
        sensor1.lastPulseTime = sensor1.highToLowTime;
        sensor1.highToLowTime = time;
    }
    else{
        sensorSchedule[scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor = 1;
        sensorSchedule[scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime = sensor1.highToLowTime;
        sensorSchedule[scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime = time;
        sensorSchedule[scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK].lastPulseTime = sensor1.lastPulseTime;
        scheduleWriteIndex++;
    }
}

//Sensor has Triggered Interrupt. Check if a change HAS occured. 
//Check through every sensor, and find which one triggered the intterupt
//Will automatically pop_back the sensor if its read or not.
//PRE-REQUISITE: sensor >= 0
void pollOOTX(){
    //DEBUG: Serial.print("INDEX DIFF: ");
    //DEBUG: Serial.println((scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK) - (scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK));

    //Buffer length must not be full. If it is, then it should send data instead.
    if(bufferFillLength <= DATA_CIRC_BUFFER_LENGTH){
        //If HighToLowTime is more recent, check the dtime
        if(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime < sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime
        && (sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lastPulseTime) > MAX_LIGHTHOUSE_SWEEPTIME){   
                OOTX_MASTER.defined = false; 
                OOTX_SLAVE.defined = false;
        }

        //lowToHighTime MUST be more recent to do anything useful
        else if (sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime > sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime){
            // 1) IF MASTER not defined, MASTER must be defined here
            if (!OOTX_MASTER.defined){
                //DEBUG: Serial.println("M");
                if(!extractOOTXData(&OOTX_MASTER))
                    return;
                cleanseSchedule();
            }
            // 2) IF SLAVE not defined, SLAVE must be defined here
            if (!OOTX_SLAVE.defined){
               //DEBUG: Serial.println("S");
                if(!extractOOTXData(&OOTX_SLAVE))
                    return;
                cleanseSchedule();
            }
            // 3) IF Both SLAVE and MASTER are defined, it must be a sweep.
            if (OOTX_MASTER.defined && OOTX_SLAVE.defined){
                //DEBUG: Serial.print("B");
                //IF both are skip, then try again lol.
                if(OOTX_MASTER.skip && OOTX_SLAVE.skip){
                    OOTX_MASTER.defined = false;
                    OOTX_SLAVE.defined = false;
                }
                else if(!OOTX_MASTER.skip){
                    //DEBUG: Serial.println("M");
                    checkSweep(&OOTX_MASTER);
                }
                else if(!OOTX_SLAVE.skip){
                    //DEBUG: Serial.println("S");
                    checkSweep(&OOTX_SLAVE);
                }
            }
            // 4) Needs to keep checking for sweeps until time is up. 
        }
    }
}


//Takes the delta time, and exports the OOTX data to the OOTX at the pointed address. 
//This shouldn't happen if all OOTX data has been logged.
//Sweeps are not counted as OOTX pulses. If one is read within this, the entire OOTX dataset is reset.
//Increments scheduleReadIndex once done.
//PRECONDITIONS: OOTX_MASTER or OOTX_SLAVE must not be defined & lowToHighTime must be > highToLowTime
bool extractOOTXData(OotxPulseInfo* OOTX){
    //Looking for the period of time where it's active. HighTime should be more recent.
    OOTX->startTime = sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime;
    OOTX->endTime = sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime; 
    int OOTXData = deltaTime_to_OOTXData(OOTX->endTime - OOTX->startTime);
    
    //DEBUG: Serial.print("Sensor: ");
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor);
    //DEBUG: Serial.print("dTime:  ");
    //DEBUG: Serial.println(OOTX->endTime - OOTX->startTime - MICROS_ADJUSTMENT);
    //DEBUG: Serial.print("OOTX: ");
    //DEBUG: Serial.println(OOTXData);

    scheduleReadIndex++;

    //if OOTX data < 0, is a sweep. and if it's a sweep, we want to reset everything.
    //The only place where a sweep is valid is where both are defined, so if one is not defined, clear them all.
    if(OOTXData < 0 || 7 < OOTXData){
        OOTX_MASTER.defined = false;
        OOTX_SLAVE.defined = false;
        return false;
    }
    
    OOTX->axis = OOTXData & 0b0001;
    //OOTX->data = OOTXData & 0b0010;
    OOTX->skip = OOTXData & 0b0100;
    OOTX->defined = true;
    return true;
}

//A cleansing sample is created. If the activation time is similar to that in the sample, the read index is incremented past.
//The read index is incremented once before incrementing more.
void cleanseSchedule(){
    long sample = sensorSchedule[(scheduleReadIndex-1) & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime;
    while(abs(sensorSchedule[(scheduleReadIndex) & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - sample) <= MAX_TIME_BTWN_OOTX){
        //DEBUG: Serial.println("Cleansed.");
        scheduleReadIndex++;
    }
}

//If the sweep time is less than the min time, move it forward, we accidentally read a sweep for a master.
//If the sweep time is more than the max time, remove the OOTX info.
//If the sweep time is in-between, then use it!
void checkSweep(OotxPulseInfo* OOTX){
    //DEBUG: Serial.println(OOTX->startTime);
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime);
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime);
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime - sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime);

    sweepTime = sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - OOTX->startTime;
    totTime = sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - OOTX_MASTER.startTime;
    lighthouseID = OOTX == &OOTX_MASTER ? 0 : 1;

    if(sensorList[sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor]->PrevSweepTime[lighthouseID][OOTX->axis] == 0){
        sensorList[sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor]->PrevSweepTime[lighthouseID][OOTX->axis] = sweepTime;
    }
    int PrevSweepTime = sensorList[sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor]->PrevSweepTime[lighthouseID][OOTX->axis];

    Serial.print("Sensor: ");
    Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor);
    Serial.print("lighthouseID: ");
    Serial.println(lighthouseID);
    Serial.print("Axis: ");
    Serial.println(OOTX->axis);

    Serial.print("SweepTime: ");
    Serial.println(sweepTime);
    Serial.print("PrevSweepTime: ");
    Serial.println(PrevSweepTime);
    //DEBUG: Serial.print("TotTime: ");
    //DEBUG: Serial.println(totTime);
    Serial.println(abs(sweepTime - PrevSweepTime));
    
    if(totTime < MIN_LIGHTHOUSE_SWEEPTIME){
        OOTX_MASTER.startTime = OOTX_SLAVE.startTime;
        OOTX_MASTER.endTime = OOTX_SLAVE.endTime;
        OOTX_MASTER.axis = OOTX_SLAVE.axis;
        OOTX_MASTER.skip = OOTX_SLAVE.skip;

        extractOOTXData(&OOTX_SLAVE);
        cleanseSchedule();

    } else if(MAX_LIGHTHOUSE_SWEEPTIME < totTime || MAX_LIGHTHOUSE_SWEEPTIME < sweepTime || abs(sweepTime - PrevSweepTime) > DELTA_THRESHOLD){
        OOTX_MASTER.defined = false;
        OOTX_SLAVE.defined = false;
    } else {
        encodeData(OOTX);
    }
}

//Takes the sweep time, the duration from the start of the sweep to the flash and encodes it into data.
//Also cludes the lighthouseID, the axis that was swept, and the sensor that detected the sweep.
//That data is sent to the circular buffer, to be sent over SerialTX
//Only proceed if the sweep time difference does not exceed that of the threshold.
//BufferWriteIndex++ bufferFillLength++ and scheduleReadIndex++
//PRECONDITIONS: Sweeptime must be defined.
void encodeData(OotxPulseInfo* OOTX){
    dataCircularBuffer[bufferWriteIndex & DATA_BUFFER_MODULUS_MASK] = 
        (sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor << 26) 
        | (lighthouseID << 25) | (OOTX->axis << 24) | sweepTime;
    bufferWriteIndex++; bufferFillLength++;
    sensorList[sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor]->PrevSweepTime[lighthouseID][OOTX->axis] = sweepTime;
    scheduleReadIndex++;
    
}

void sendData(){
    //Modulo operator, so that we can loop through everything.
    unsigned int data = dataCircularBuffer[bufferReadIndex & DATA_BUFFER_MODULUS_MASK];
    //If the current data == 0 (Should only happen at beginning), or read has reached write
    if((data == 0) || (bufferReadIndex >= bufferWriteIndex)){
        return;
    }

    //Send data over chosen Serial port
    SerialTX.print(data);
    SerialTX.print(';');

    bufferReadIndex++;
    bufferFillLength--;
    
    //Example Breakdown:
    int sensor = 0;
    int lighthouse = 0;
    int axis = 0;
    int sweep = 0;
    //Start from most significant bit, and go down. i controls the bitshift
    for (int i = 31; i >= 0; i--) {
        int bit = (data >> i) & 1;
        //SerialTX.print(bit);
        if(i >= 26){
            sensor <<= 1;
            sensor |= bit;
        } else if(i == 25){
            lighthouse |= bit;
        } else if(i == 24){
            axis |= bit;
        } else{
            sweep <<= 1;
            sweep |= bit;
        }
    }
    Serial.print(" Sensor - ");
    Serial.print(sensor);
    Serial.print(" Lighthouse - ");
    Serial.print(lighthouse);
    Serial.print(" Axis - ");
    Serial.print(axis);
    Serial.print(" Sweep Time - ");
    Serial.print(sweep);
    Serial.println();
}


//Converts dTime to OOTX number From Microseconds.
int deltaTime_to_OOTXData(unsigned long dtime){
    double ticks = (dtime-MICROS_ADJUSTMENT) * MICROSECONDS_TO_TICKS;
    double number = (ticks-LOWEST_TICK_COUNT)/TICK_COUNT_RANGE;

    return(number >= 0 ? number : -1);
}
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
#define MAX_TIME_BTWN_OOTX              20.0                            //In uS

//Be sure to allocate 14 bits for the sweeptime!!
#define DATA_LENGTH                     24                              //This is just for reference...
#define INPUT_LEFTSHIFT                 23                              //This is only meant for 1 button input.
#define SENSOR_LEFTSHIFT                18                              //With 1 input bit, this leaves 32 total sensors.
#define LIGHTHOUSE_LEFTSHIFT            17
#define AXIS_LEFTSHIFT                  16

//The lengths of both Ring Buffers MUST be a power of two. The optimisation within both use a BITWISE AND instead of a Modulus.
//1, 2, 4, 16, 32, 64, 128, 256, 512, 1024, 2048
#define SCHEDULE_BUFFER_LENGTH          64
#define SCHEDULE_BUFFER_MODULUS_MASK    (SCHEDULE_BUFFER_LENGTH - 1)

///////////////////////////////////////////
///                                     ///
///        *SENSORS AND INPUT*          ///
///                                     ///
///////////////////////////////////////////

    //E pin then D pin.
    int EDPins[][2] = {{53, 51}, {52, 50} /*,{}, {}, {}, {}, {}*/};
    //NOTE: When pin drops low, that's the start of an IR signal.

    TS4231 sensorList[NUM_SENSORS];

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

    //Ring Buffer!
    SensorPackage sensorSchedule[SCHEDULE_BUFFER_LENGTH];
    volatile int scheduleWriteIndex = 0;
    volatile int scheduleReadIndex = 0;

    //Connect to a switch that's normally low. Goes high once activated.
    //Set to -1 if you don't want it on.
    int inputPin = -1;
    bool inputVal = false;

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
    //The only time this is a previous sweep time is in checkSweep().
    //Sensor, Lighthouse, Axis
    int prevSweepTimeList[NUM_SENSORS][2][2];

///////////////////////////////////////////
///                                     ///
///          Data Structure             ///
///                                     ///
///////////////////////////////////////////

    //Fist bit = input (switch or a button)
    //Next 5 bits -> Sensor. Offset from Least Significant bit by SENSOR_LEFTSHIFT
    //Next bit -> Lighthouse (0 = master, 1 = slave). Offset from Least Significant bit by LUGHTHOUSE_LEFTSHIFT
    //Next bit -> Axis (0 = X, 1 = Y). Offset from Least Significant bit by AXIS_LEFTSHIFT
    //Rest of them = dtime from sweep. (This should be no more than 8333 microseconds)
    //if a value == 0, then no data is there.
    unsigned int data;

///////////////////////////////////////////
///                                     ///
///        *THE ACTUAL CODE LOL*        ///
///                                     ///
///////////////////////////////////////////

void setup(){
    while(!SerialTX){}

    SerialTX.begin(115200);
    Serial.begin(115200);

    //Serial.println("Hello, please wait...");

    //HM-10 only communicates AT commands when NL-CR are off.
    // delay(500);
    // SerialTX.println("AT");
    // delay(500);
    // SerialTX.println("AT+NAMEADV-ENG2023"); 
    // delay(500);
    // //0 = Slave, 1 = Master
    // SerialTX.println("AT+ROLE0");
    // // 0: 9600, 1: 19200, 2: 38400, 3: 57600, 4: 115200, 5: 4800, 6: 2400, 7: 1200, 8: 230400
    // SerialTX.println("AT+BAUD0");
    // SerialTX.end();
    // SerialTX.begin(115200);
    
    for(int i = 0; i < NUM_SENSORS; i++){
        sensorList[i].setPins(EDPins[i][0], EDPins[i][1]);
    }

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
    if(inputPin != -1) attachInterrupt(digitalPinToInterrupt(inputPin), ISR_input, CHANGE);
}

//One cycle of an entire Master/Slave loop should be about 30Hz, or a period of 0.033s
//Master Lighthouse gives its 2 updates in the first 0.0167s, then the Slave's 2 in the second 0.167s.
//Cycle is always: Master, Slave, Sweep. (Repeat)
//NOTE: The sweep time cannot be larger than 1/120th of a second (8333 microseconds), so discard those any bigger than that.
void loop(){
    unsigned int startLoop = micros();
    Serial.println("______"); 

    //The current issue is that the ISRs don't place the sensors properly inside of the buffer.

    //If a sensor has been placed in the schedule, it's done submitting. if it's >= 0, then it's able to be read.
    if (sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor >= 0 
    && sensorSchedule[(scheduleReadIndex+1) & SCHEDULE_BUFFER_MODULUS_MASK].sensor >= 0){
        pollOOTX();
    }

    Serial.print("DELTA: ");
    Serial.println(micros() - startLoop);
}   

void debug(){
    Serial.print("R: ");
    Serial.println(scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK);
    Serial.print("W: ");
    Serial.println(scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK);

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
            if (sensorList[i].waitForLight(TS4231_LIGHT_TIMEOUT)) {
                //Execute this code when light is detected
                Serial.print("Sensor ");
                Serial.print(i);
                Serial.println(" Light DETECTED");
                config_result = sensorList[i].configDevice();
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
//This would be so much easier if they could. 
void ISR_sensor0(){
    unsigned int time = micros();
    //If it is low, sets the highToLowTime to now. and the last pulse time to highToLowTime
    //If it is high, sets the lowToHighTime to now.
    if (!digitalRead(sensorList[0].E_pin)){
        scheduleWriteIndex++;
        sensorList[0].writeIndex = scheduleWriteIndex;
        sensorSchedule[sensorList[0].writeIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor = -1;
        sensorList[0].lastPulseTime = sensorList[0].highToLowTime;
        sensorList[0].highToLowTime = time;
    }
    else{
        sensorSchedule[sensorList[0].writeIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor = 0;
        sensorSchedule[sensorList[0].writeIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime = sensorList[0].highToLowTime;
        sensorSchedule[sensorList[0].writeIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime = time;
        sensorSchedule[sensorList[0].writeIndex & SCHEDULE_BUFFER_MODULUS_MASK].lastPulseTime = sensorList[0].lastPulseTime;
    }
}
void ISR_sensor1(){
    unsigned int time = micros();
    if (!digitalRead(sensorList[1].E_pin)){
        scheduleWriteIndex++;
        sensorList[1].writeIndex = scheduleWriteIndex;
        sensorSchedule[sensorList[1].writeIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor = -1;
        sensorList[1].lastPulseTime = sensorList[1].highToLowTime;
        sensorList[1].highToLowTime = time;
    }
    else{
        sensorSchedule[sensorList[1].writeIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor = 1;
        sensorSchedule[sensorList[1].writeIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime = sensorList[1].highToLowTime;
        sensorSchedule[sensorList[1].writeIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime = time;
        sensorSchedule[sensorList[1].writeIndex & SCHEDULE_BUFFER_MODULUS_MASK].lastPulseTime = sensorList[1].lastPulseTime;
    }
}
void ISR_input(){
    inputVal = digitalRead(inputPin);
}

//This is the main polling that the arduino uses to check for OOTX and Sweep signals.
//A sensor that has been triggered will send its complete data to the Sensor Schedule.
//This reads the most recent sensor reading, and interprets it.
void pollOOTX(){
    //DEBUG: Serial.print("INDEX DIFF: ");
    //DEBUG: Serial.println((scheduleWriteIndex & SCHEDULE_BUFFER_MODULUS_MASK) - (scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK));
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
            resetSensorRead();
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
                //DEBUG: Serial.println("N");
                OOTX_MASTER.defined = false;
                OOTX_SLAVE.defined = false;
            }
            else if(!OOTX_MASTER.skip){
                //DEBUG: Serial.println("M");
                if(checkSweep(&OOTX_MASTER)) encodeAndSendData(&OOTX_MASTER);
            }
            else if(!OOTX_SLAVE.skip){
                //DEBUG: Serial.println("S");
                if(checkSweep(&OOTX_SLAVE)) encodeAndSendData(&OOTX_SLAVE);
            }
        }
        // 4) Needs to keep checking for sweeps until time is up. 
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

    //DEBUG: Serial.print("ActTime: ");
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime);
    //DEBUG: Serial.print("DActTime: ");
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime);
    //DEBUG: Serial.print("PSActTime: ");
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lastPulseTime);
    //DEBUG: 
    //DEBUG: Serial.print("Sensor: ");
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor);
    //DEBUG: Serial.print("dTime:  ");
    //DEBUG: Serial.println(OOTX->endTime - OOTX->startTime - MICROS_ADJUSTMENT);
    //DEBUG: Serial.print("OOTX: ");
    //DEBUG: Serial.println(OOTXData);

    scheduleReadIndex++;

    //if OOTX data < 0, is a sweep. and if it's a sweep, we want to reset everything.
    //The only place where a sweep is valid is where both are defined, so if one is not defined, clear them all and try again.
    if(OOTXData < 0 || 7 < OOTXData){
        //DEBUG: Serial.println("OOTX was either too small or too big, oops.");
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

//This gets rid of similar, potentially duplicate sensor readings on the schedule.
//A cleansing sample is created. If the difference between activation times of the sample and current reading is under MAX_TIME_BTWN_OOTX... 
//the read index is incremented past.
void cleanseSchedule(){
    long sample = sensorSchedule[(scheduleReadIndex-1) & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime;
    while(abs(sensorSchedule[(scheduleReadIndex) & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - sample) <= MAX_TIME_BTWN_OOTX){
        //DEBUG: Serial.println("Cleansed.");
        scheduleReadIndex++;
    }
}

//Validates whether the current pulse is actually a sweep or not, returning a boolean.
//If the total time is less than the min time, adjust the OOTX, we accidentally declared a sweep a master OOTX.
//If the sweep time is more than the max time, remove the OOTX info.
//If the sensor has already been ready, then skip past it! We don't want any dupes...
//If the sweep time is in-between the min and max, and the sensor that sent it has not been read, then read it!
bool checkSweep(OotxPulseInfo* OOTX){
    //DEBUG: Serial.println(OOTX->startTime);
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime);
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime);
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].lowToHighTime - sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime);
    
    sweepTime = sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - OOTX->startTime;
    totTime = sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - OOTX_MASTER.startTime;
    lighthouseID = OOTX == &OOTX_MASTER ? 0 : 1;

    //DEBUG: Serial.print("SweepTime: ");
    //DEBUG: Serial.println(sweepTime);
    //DEBUG: Serial.print("TotTime: ");
    //DEBUG: Serial.println(totTime);
    //DEBUG: Serial.print("Sensor: ");
    //DEBUG: Serial.println(sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor);
    //DEBUG: Serial.print("hasBeenRead? ");
    //DEBUG: Serial.println(sensorList[sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor].hasBeenRead);
    
    if(totTime < MIN_LIGHTHOUSE_SWEEPTIME) {
        //DEBUG: Serial.println("Went Under - Adjusting");

        //this should be impossible. correct it by swapping them lol.
        if(totTime < sweepTime){
            OotxPulseInfo tempOOTX;
            tempOOTX.startTime = OOTX_MASTER.startTime;
            tempOOTX.endTime = OOTX_MASTER.endTime;
            tempOOTX.axis = OOTX_MASTER.axis;
            tempOOTX.skip = OOTX_MASTER.skip;

            OOTX_MASTER.startTime = OOTX_SLAVE.startTime;
            OOTX_MASTER.endTime = OOTX_SLAVE.endTime;
            OOTX_MASTER.axis = OOTX_SLAVE.axis;
            OOTX_MASTER.skip = OOTX_SLAVE.skip;

            OOTX_SLAVE.startTime = OOTX_MASTER.startTime;
            OOTX_SLAVE.endTime = OOTX_MASTER.endTime;
            OOTX_SLAVE.axis = OOTX_MASTER.axis;
            OOTX_SLAVE.skip = OOTX_MASTER.skip;

            scheduleReadIndex++;
            cleanseSchedule();
        }
        else{
            OOTX_MASTER.startTime = OOTX_SLAVE.startTime;
            OOTX_MASTER.endTime = OOTX_SLAVE.endTime;
            OOTX_MASTER.axis = OOTX_SLAVE.axis;
            OOTX_MASTER.skip = OOTX_SLAVE.skip;

            extractOOTXData(&OOTX_SLAVE);
            cleanseSchedule();
        }
        return false;

    } else if(MAX_LIGHTHOUSE_SWEEPTIME < totTime) {
        //DEBUG: Serial.println("Went Over - Resetting");
        if(totTime < 8400){ //8400 is arbitary. it's the cut off before we deem the system a failure.
            sweepTime = 0x4000;
            //If a sensor has not been swept, send an intentionally large value.
            for(int i = 0; i < NUM_SENSORS; i++){
                if(!sensorList[i].hasBeenRead) encodeAndSendData(OOTX, i);
            }
        }
        OOTX_MASTER.defined = false;
        OOTX_SLAVE.defined = false;
        return false;
    } else if (sensorList[sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor].hasBeenRead) {
        //DEBUG: Serial.println("Duplicate Scan");
        //TODO: FIGURE OUT WHY Lighthouse 1 Axis 1 always returns a dupe.
        scheduleReadIndex++;
        return false;
    }

    // int tempScheduleReadIndex = scheduleReadIndex + 1;

    //An attempt to average duplicate sweeps.
    // while(sensorSchedule[tempScheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - OOTX_MASTER.startTime < MAX_LIGHTHOUSE_SWEEPTIME){
    //     if(sensorSchedule[tempScheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor == sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor)
    //         sweepTime += sensorSchedule[tempScheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - OOTX->startTime;
    //         //sweepTime = sensorSchedule[tempScheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - OOTX->startTime;
    //     tempScheduleReadIndex++;
    // }
    // sweepTime /= tempScheduleReadIndex-scheduleReadIndex;

    
    //An attempt to use the previous sweep time to determine the current one.
    // int tempSweepTime;
    // int prevSweepTime = prevSweepTimeList[sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor][lighthouseID][OOTX->axis];
    // while(sensorSchedule[tempScheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - OOTX_MASTER.startTime < MAX_LIGHTHOUSE_SWEEPTIME){
    //     if(sensorSchedule[tempScheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor == sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor)
    //         tempSweepTime = sensorSchedule[tempScheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - OOTX->startTime;
    //         //sweepTime = sensorSchedule[tempScheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].highToLowTime - OOTX->startTime;
    //     tempScheduleReadIndex++;
    // }
    // sweepTime = ( abs(sweepTime-prevSweepTime) < abs(tempSweepTime-prevSweepTime) ) ? sweepTime : tempSweepTime;

    // prevSweepTimeList[sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor][lighthouseID][OOTX->axis] = sweepTime;

    return true;
}

//Takes the sweep time, the duration from the start of the sweep to the flash and encodes it into data.
//Also cludes the lighthouseID, the axis that was swept, and the sensor that detected the sweep.
//That data is sent over SerialTX
//scheduleReadIndex++
//PRECONDITIONS: Sweeptime must be defined.
void encodeAndSendData(OotxPulseInfo* OOTX){
    data = (inputVal << INPUT_LEFTSHIFT) 
        | (sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor << SENSOR_LEFTSHIFT) 
        | (lighthouseID << LIGHTHOUSE_LEFTSHIFT) | (OOTX->axis << AXIS_LEFTSHIFT) | sweepTime;
        
    SerialTX.print(data);
    SerialTX.print(';');
    Serial.print(data);
    Serial.println(';');

    debugData();

    sensorList[sensorSchedule[scheduleReadIndex & SCHEDULE_BUFFER_MODULUS_MASK].sensor].hasBeenRead = true;
    scheduleReadIndex++;   
}
//Alternate version of the original. Focuses solely on the OOTX and the Sensor.
//No iteration here,
void encodeAndSendData(OotxPulseInfo* OOTX, int sensor){
    data = (inputVal << INPUT_LEFTSHIFT) | (sensor << SENSOR_LEFTSHIFT) | (lighthouseID << LIGHTHOUSE_LEFTSHIFT) 
        | (OOTX->axis << AXIS_LEFTSHIFT) | sweepTime;
        
    SerialTX.print(data);
    SerialTX.print(';');

    debugData();

    sensorList[sensor].hasBeenRead = true;   
}

void debugData(){
    int sensor = 0;
    int lighthouse = 0;
    int axis = 0;
    int sweep = 0;
    //Start from most significant bit, and go down. i controls the bitshift
    for (int i = DATA_LENGTH; i >= 0; i--) {
        int bit = (data >> i) & 1;
        //SerialTX.print(bit);
        if(i >= SENSOR_LEFTSHIFT){
            sensor <<= 1;
            sensor |= bit;
        } else if(i == LIGHTHOUSE_LEFTSHIFT){
            lighthouse |= bit;
        } else if(i == AXIS_LEFTSHIFT){
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

void resetSensorRead(){
    for(int i = 0; i < NUM_SENSORS; i++){
        sensorList[i].hasBeenRead = false;
    }
}
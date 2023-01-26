#include "Header/ts4231.h"
#include "Header/ts4231.cpp"
#include "Header/DueLighthouseReader.h"
#include "Header/IrSensorClass.h"

static int counter = 0;

#define light_timeout   3000  //500ms is a placeholder as this number will be system dependent
uint8_t config_result;

static RingBuff ringBuff;
//D then E Pin
int DEPins[][2] = {{37, 39}};
IrSensor IRSensorList[MAX_RECEIVERS];
OotxPulseInfo OotxInfo;

void setup() 
{
  
  Serial.begin(115200);
  while (!Serial);  //wait for serial port to connect

  Serial.println("Serial Port Connected");
  Serial.println();
  Serial.println();
  
  //E THEN D
  TS4231 sensor1(DEPins[0][1], DEPins[0][0]);

  //Config all of the sensors
  configSensor(sensor1);
  
  //SysTick is a timer that counts down from a specific point to zero.
  Serial.print("SysTick->LOAD: ");
  Serial.println(SysTick->LOAD);

  // BEWARE!!!  The following line changes the working of the Arduino's inner clock. (NOT the CPU Clock)
  // Specifically, it will make it run 100X slower.  So, if you call "delay(10)" you
  // will instead delay by a full second instead of 10 milliseconds.
  // This is needed in order to do the high precision timing needed to capture
  // the pulses from the lighthouse.  

  //The due runs at 84MHz (84,000,000). this is that, but divided by 10... (8,400,000). 
  SysTick->LOAD = 8399999;
  Serial.print("SysTick->LOAD: ");
  Serial.println(SysTick->LOAD);

  attachInterrupts();
}


void loop() 
{
  //Not sure what this while loop is asking of?
  while ((((ringBuff.writerPos - 1 - ringBuff.readerPos) + RINGBUFF_LEN) & RINGBUFF_MODULO_MASK) != 0) {
    bool isFalling = false;
    
    ringBuff.readerPos = (ringBuff.readerPos+1) & RINGBUFF_MODULO_MASK;

    // assign a reference.  No need to copy to a temp value.
    volatile unsigned int &rawVal = ringBuff.buff[ringBuff.readerPos];
    
    //Determines if it's a falling edge from raw data. Remove 1st bit
    if (FALLING_EDGE & rawVal) {
      isFalling = true;
    }

    // this line limits us to 128 sensors. Remove 2nd through 8th bit
    int sensor = rawVal & 0x7F000000;

    //Plops in the value of the timer @ interrupt, filtering out the sensor. Remove rest of bits
    unsigned int val = rawVal & 0x00FFFFFF;

    //Special Precaution: this should NEVER happen
    if (sensor >= MAX_RECEIVERS) {
      Serial.print("Invalid Sensor Number!!!");
      continue;
    }
    
    //IF IS RISING
    if (!isFalling) {
      ifSignalIsRising(sensor, val);
    }
    //IF IS FALLLING
    else {
      ifSignalIsFalling(sensor, val);
    }

    counter++;
    if (counter % 100 == 0 & false) {
          Serial.print((ringBuff.writerPos + RINGBUFF_LEN - ringBuff.readerPos) & RINGBUFF_MODULO_MASK);
          Serial.print(" ");
          Serial.print(isFalling);
          Serial.print(" ");
          Serial.print(sensor);
          Serial.print(" ");
          Serial.println(val);
    }
  }
}

void ifSignalIsRising(int sensor, unsigned int val){
  IRSensorList[sensor].lastRiseTime = val;      

  int duration = (OotxInfo.startTime + MAX_COUNTER - val)%MAX_COUNTER;
  // if this looks like a pulse from a sweeping laser 
  if (duration < MAX_CLOCK_CYCLES_PER_SWEEP) {
    // hey, it looks like we see a laser sweeping.  Cool!

    //TODO: remove magic number.
    float angle = duration * 0.000257143;
    
    //Whether its X or Y axis.
    if (OotxInfo.rotor)
    {
        IRSensorList[sensor].Y = angle;    
    }
    else
    {
        IRSensorList[sensor].X = angle;      
    }

    // TODO: put a last update timestamp on as well.
    
  }
}
void ifSignalIsFalling(int sensor, unsigned int val){
  int duration = (IRSensorList[sensor].lastRiseTime + MAX_COUNTER - val) % MAX_COUNTER;  

      // if this looks like an OOTX pulse...
      if (duration >= BASE_OOTX_TICKS && duration < MAX_OOTX_TICKS)
      {
        OotxInfo.startTime = IRSensorList[sensor].lastRiseTime;
        
// we have an OOTX pulse.  
// for best accuracy, let's go figure out which sensor saw the beginning of the 
// pulse first.  It's effectively random which interrupt would have fired first
// so we want to find which one it was.  
        for (int i=0; i < MAX_RECEIVERS; i++)
        {
          int tempDuration = (IRSensorList[i].lastRiseTime + MAX_COUNTER - val) % MAX_COUNTER;  

          if (tempDuration > MAX_OOTX_TICKS){
            continue;
          }
          if (tempDuration < BASE_OOTX_TICKS){
/* 
that's odd, wouldn't expect to see this very often
only case I can think of would be if all of the sensors have been unable
to see the OOTX pulses for a while, then you come back. i.e. leaving room and coming back.
and even then, this would only only happen rarely because you'd have to come back
very close to a multiple of MAX_COUNTER ticks.  Specifically, if you left
and came back, you'd only see this 100*(MAX_OOTX_TICKS - BASE_OOTX_TICKS)/MAX_COUNTER percent of the time.  That's rare.
*/
            Serial.println("OOTX time underflow");
            continue;
          }
/*
the longest duration will indicate the earliest start time.
this should be from whichever sensor triggered the first interrupt
that's the one we care about because it will be the most accurate.
*/
          if (tempDuration > duration)
          {
            duration = tempDuration;
            OotxInfo.startTime = IRSensorList[sensor].lastRiseTime;           
          }
        }
/*
Okay, now we have a quality OOTX start time and duration.  
First, let's poison the start times. That way, we won't do the above check
again for the same OOTX pulse. We'll "poison" the start times by making sure they're
showing as old enough that the next sensor that registers a pulse will see it being
too long/old for the pulse to be an OOTX pulse, and it will throw it away.
since we only poison after the first falling edge is detected, we will ensure
that the first falling edge detected for the OOTX pulse from any sensor 
(which is going to be the most accurate time of the pulse) will be the only
falling edge we look at. Combined with logic in the code above, this means
that for an OOTX pulse, we'll always look at the most accurate start time
received from any sensor, and the most accurate pulse end time received
from any sensor, even if they're from different sensors.
*/
        poisonLastStartTime(val);
        
// Now we need to decode the OOTX pulse;
        unsigned int decodedPulseVal = (duration - BASE_OOTX_TICKS) / TICKS_PER_OOTX_CHANNEL;

        OotxInfo.rotor = decodedPulseVal & 0b0001;
        OotxInfo.data =  decodedPulseVal & 0b0010;
        OotxInfo.skip =  decodedPulseVal & 0b0100;

// This part does not compile on purpose, #if 0 is false. 
  #if 0
    Serial.print((ringBuff.writerPos + RINGBUFF_MAX - ringBuff.readerPos) % RINGBUFF_MAX);
    Serial.print(" ");
    Serial.print(OotxInfo.rotor);
    Serial.print(" ");
    Serial.print(OotxInfo.data);
    Serial.print(" ");
    Serial.println(OotxInfo.skip);
  #endif        

  #if 1
    static int jumpCounter = 0;
    jumpCounter++;
    if (jumpCounter %10 == 0)
    {
          Serial.print((ringBuff.writerPos + RINGBUFF_LEN - ringBuff.readerPos) & RINGBUFF_MODULO_MASK);
          //Serial.print((ringBuff.writerPos + RINGBUFF_MAX - ringBuff.readerPos) & RINGBUFF_MODULO_MASK);
          Serial.print(" ");

          for (int i=0; i < MAX_RECEIVERS - 1; i++)
          {
            Serial.print(i);
            Serial.print(":(");
            Serial.print(IRSensorList[i].X);
            Serial.print(",");
            Serial.print(IRSensorList[i].Y);
            Serial.print(") ");      
          }
          
          Serial.println("");
    }
  #endif

        // TODO: ProcessOotxBit(IRSensorList[sensor].data);        
      }
}

void poisonLastStartTime(unsigned int val){
// Let's poison. (Extra assurance to prevent any miscalculations)
        unsigned int poisonValue = (val + MAX_COUNTER - MAX_OOTX_TICKS - 10) % MAX_COUNTER; 
//I think doing a "- 1" above would be sufficient, but doing a "-10" just to avoid any possible off-by-1 errors.
        for (int i=0; i < MAX_RECEIVERS; i++)
        {
          IRSensorList[i].lastRiseTime = poisonValue;
        }
}


void RISING_INTERRUPT_BODY(int receiver){
  Serial.print("Sensor ");
  Serial.print(receiver);
  Serial.println(" rising interrupt triggered");
  if (ringBuff.readerPos != ringBuff.writerPos) {
    //Assigns the current buffer value a new number:
    //gets 24 bit number of the SysTick value with the receiver bitshifted to just before the last bit
    ringBuff.buff[ringBuff.writerPos] = SysTick->VAL | (receiver<<24);
    Serial.println(SysTick->VAL);
    //Increases the WriterPos by one, and applies the Modulo Mask
    ringBuff.writerPos = (ringBuff.writerPos + 1) & RINGBUFF_MODULO_MASK;
  }
}
void FALLING_INTERRUPT_BODY(int receiver){
  Serial.print("Sensor ");
  Serial.print(receiver);
  Serial.println(" falling interrupt triggered");
  if (ringBuff.readerPos != ringBuff.writerPos) {
    //Like rising, but inserts a falling edge into the first bit.
    ringBuff.buff[ringBuff.writerPos] = (SysTick->VAL) | FALLING_EDGE | (receiver<<24);
    Serial.println(SysTick->VAL);
    ringBuff.writerPos = (ringBuff.writerPos + 1) & RINGBUFF_MODULO_MASK;
  }
}
//Rework this mess...
void rising0()
{
  FALLING_INTERRUPT_BODY(0);
}
void falling0()
{
  RISING_INTERRUPT_BODY(0);
}

void attachInterrupts(){
  Serial.println("Attaching interrupts.");
  // flip the FALLING and RISING here because the input signal is active low
  //Format is: attachInterrupt(digitalPinToInterrupt(Pin#), Interupt Functions, RISING/FALLING);
  //sizeof(DEPins)/sizeof(DEPins[0]) = Number of Rows
  for(int i = 0; i < (sizeof(DEPins)/sizeof(DEPins[0])); i++){
    attachInterrupt(digitalPinToInterrupt(DEPins[i][1]), rising0, RISING);
    attachInterrupt(digitalPinToInterrupt(DEPins[i][1]), falling0, FALLING);
  }
}

void configSensor(TS4231 d){
        bool isSuccess = false;

        while (!isSuccess){
            if (d.waitForLight(light_timeout)) {
                //Execute this code when light is detected
                Serial.println("Light DETECTED");
                config_result = d.configDevice();
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
                }
        else {
            //insert code here for no light detection
            Serial.println("Light TIMEOUT");
            }
        Serial.println("");
    }
  }

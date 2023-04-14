//FINAL-TS4231.ino
//CODE WRITTEN BY SAM HOLLIDAY - CHICKENWINGJOHNNY - ARGS 22-23 Advanced Engineering
//Purpose - To send controller data about "Sweep times" and Buttons over serial.
//NOTE: In order for this to properly work, both lighthouses must be active.
#define SerialTX                Serial1

    //Connect to a switch that's normally low. Goes high once activated.
    //Set to -1 if you don't want it on.
    int inputPin = 53;
    bool inputVal = false;


void setup(){
    //SerialTX.begin(115200);
    Serial.begin(115200);

    //while(!SerialTX){}

    if(inputPin != -1) {
      pinMode(inputPin, INPUT);
      attachInterrupt(digitalPinToInterrupt(inputPin), ISR_input, CHANGE);
    }
    
}

//One cycle of an entire Master/Slave loop should be about 30Hz, or a period of 0.033s
//Master Lighthouse gives its 2 updates in the first 0.0167s, then the Slave's 2 in the second 0.167s.
//Cycle is always: Master, Slave, Sweep. (Repeat)
//NOTE: The sweep time cannot be larger than 1/120th of a second (8333 microseconds), so discard those any bigger than that.
void loop(){
    Serial.println(inputVal);
}   

void ISR_input(){
    inputVal = digitalRead(inputPin);
}

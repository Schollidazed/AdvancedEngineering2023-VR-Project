# AdvancedEngineering2023-VR-Project
This is the 2023 Advanced Engineering project for the Appomattox Reigonal Governor's School. 

***

# Overview

***There are two parts to this:***

**Arduino:** Samples IR sensors powered by TS4231 microchips. 
  - Determines Sensor that detected the signal, the Lighthouse that sent the signal, Whether the lighthouse was on the X or Y axis at that moment, and the time in-between the signal and the sweep. 
  - Plan on including a way to send extraneous signals, such as buttons and triggers through the data sent as well. Also plan on having the arduino recieve data through serial, to change the state of a cyclical motor.
  
**Unity:** Two Planned Projects.
  - Test Field: Polls data from the arduino (from a Bluetooth Low Energy connection), and makes the calculations neccessary to interpret it as movement in 3D space. Will show points in space, compared to a model of the object.
  - Sample Game: Uses the previous algorithm inside of a test game. This is the primary showcase of the tech.

## Required Materials
  - 2 HTC Vive Lighthouses (V1)
  - Arduino Due (Code can be changed to support other architectures)
  - IR Sensor(s) using TS4231 Microchip. (Again, code can be changed.)

***

## Instructions for ARDUINO:
  1. Open the ***FINAL-TS4231.ino*** file
  2. Scroll to **EDPins**. This is an array of the E pins and D pins that are connected. Inside the brackets, for each sensor, insert the E pin number that you have connected to your board, then your D pin (like the example shown in the file)
  3. Below the **EDPins** list ist a list of sensors **(sensor0, sensor1, sensor2, etc.)** Uncomment/add more objects according to the number of sensors that you have.
  4. **sensorList** stores the addresses of each sensor. add the necessary sensors by doing **&sensorX** (where X is the sensor number)
  5. Scroll to the function **TS4231_attachIntterupts().** Add more interrupts according to the template shown with sensor0. All you need to do is copy and paste, and adjusting the names. Don't forget about the different ISR's! when copying and pasting those, change the references from sensorX, to sensorY (the number that you change the title to.)
  6. Upload!

***

# Sources:
  - VRKitz-Board Library: AtillaTheBum    ---   https://github.com/AttilaTheBum/VRKitz---Board-Library
  - HomebrewLighthouse:   Trammel Hudson  ---   https://trmm.net/Lighthouse/
  - LighthouseRedox:      nairol          ---   https://github.com/nairol/LighthouseRedox
  - Tracking:             Omigamedev      ---   https://bitbucket.org/omigamedev/tracking/src/master/
  - DueLighthouse:        mwturvey        ---   https://github.com/mwturvey/DueLighthouse
  - BleWinrtDll:          ababru          ---   https://github.com/adabru/BleWinrtDll

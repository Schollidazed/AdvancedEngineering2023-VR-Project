# AdvancedEngineering2023-VR-Project
This is the 2023 Advanced Engineering project for the Appomattox Reigonal Governor's School. 

## There are two parts to this:
  - Arduino: Samples IR sensors powered by TS4231 microchips, determining the Sensor that detected the signal, the Lighthouse that sent the signal, which axis it was on, and the time in-between the signal and the sweep. Also will include a way to send extraneous signals, such as buttons and triggers through the data sent as well.   
  - Unity: polls data from a BLE lighthouse device, interprets that as movement in 3D space, and makes the calculations neccessary to make that possible.

***

## Instructions FOR ARDUINO:
  1. Be sure you are using IR sensors with the TS4231 Chip, as well as the Arduino Due. This program will not work otherwise without significant editing (Highly encouraged!)
  2. Open the ***FINAL-TS4231.ino*** file
  3. Scroll to **EDPins**. This is an array of the E pins and D pins that are connected. Inside the brackets, for each sensor, insert the E pin number that you have connected to your board, then your D pin (like the example shown in the file)
  4. Below the **EDPins** list ist a list of sensors **(sensor0, sensor1, sensor2, etc.)** Uncomment/add more objects according to the number of sensors that you have.
  5. **sensorList** stores the addresses of each sensor. add the necessary sensors by doing **&sensorX** (where X is the sensor number)
  6. Scroll to the function **TS4231_attachIntterupts().** Add more interrupts according to the template shown with sensor0. All you need to do is copy and paste, and adjusting the names. Don't forget about the different ISR's! when copying and pasting those, change the references from sensorX, to sensorY (the number that you change the title to.)

***

# Sources:
  - VRKitz-Board Library: AtillaTheBum    ---   https://github.com/AttilaTheBum/VRKitz---Board-Library
  - HomebrewLighthouse:   Trammel Hudson  ---   https://trmm.net/Lighthouse/
  - LighthouseRedox:      nairol          ---   https://github.com/nairol/LighthouseRedox
  - Tracking:             Omigamedev      ---   https://bitbucket.org/omigamedev/tracking/src/master/
  - DueLighthouse:        mwturvey        ---   https://github.com/mwturvey/DueLighthouse
  - BleWinrtDll:          ababru          ---   https://github.com/adabru/BleWinrtDll

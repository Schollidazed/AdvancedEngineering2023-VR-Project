# AdvancedEngineering2023-VR-Project
This is the 2023 Advanced Engineering project for the Appomattox Reigonal Governor's School. 

*There are two parts to this:*
  - Arduino: Samples IR sensors powered by TS4231 microchips, determining the Sensor that detected the signal, the Lighthouse that sent the signal, which axis it was on, and the time in-between the signal and the sweep. Also will include a way to send extraneous signals, such as buttons and triggers through the data sent as well.   
  - Unity: polls data from a BLE lighthouse device, interprets that as movement in 3D space, and makes the calculations neccessary to make that possible.

*Sources:*
  - VRKitz-Board Library: AtillaTheBum    ---   https://github.com/AttilaTheBum/VRKitz---Board-Library
  - HomebrewLighthouse:   Trammel Hudson  ---   https://trmm.net/Lighthouse/
  - LighthouseRedox:      nairol          ---   https://github.com/nairol/LighthouseRedox
  - Tracking:             Omigamedev      ---   https://bitbucket.org/omigamedev/tracking/src/master/
  - DueLighthouse:        mwturvey        ---   https://github.com/mwturvey/DueLighthouse
  - BleWinrtDll:          ababru          ---   https://github.com/adabru/BleWinrtDll

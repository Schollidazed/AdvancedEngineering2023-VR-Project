using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using UnityEngine;

public class BLEArduinoVR : MonoBehaviour
{
    BLE ble;
    BLE.BLEScan scan;

    public int NumberOfSensors;

    public string targetDeviceName = "ADV-ENG2023";
    public string serviceUuid = "{0000FFE0-0000-1000-8000-00805F9B34FB}";
    public string[] characteristicUuids = {
         "{0000FFE1-0000-1000-8000-00805F9B34FB}"      // CUUID 1 
    };

    public string deviceId = "";
    public string bleDeviceID = "BluetoothLE#BluetoothLE58:96:1d:5b:b7:00-a0:6c:65:cf:7e:67";
    public string bleCharacteristicName = "DSDTECH";

    IDictionary<string, string> discoveredDevices = new Dictionary<string, string>();
    volatile byte[] packageReceived = null;

    IRSensor sensor0;
    //public IRSensor[] sensorList;
    public double MaxThreshold = 100;

    //Must be to the power of 2.
    public volatile int[] ringBuffer = new int[256];

    public int ringBufferLength;
    int ringBufferModulusMask;

    volatile int bufferFillLength = 0;
    volatile int bufferReadIndex = 0;
    volatile int bufferWriteIndex = 0;

    public bool isScanning = false, isFinishedScanning = false, isConnecting = false, isReading = false;

    // BLE Threads 
    Thread scanningThread, connectionThread, readingThread;

    public void startBLE()
    {
        readingThread = new Thread(ReadBleData);
        readingThread.Priority = System.Threading.ThreadPriority.AboveNormal;

        ble = new BLE();
        ringBufferLength = ringBuffer.Length;
        ringBufferModulusMask = ringBufferLength - 1;

        sensor0 = new IRSensor();

        //for (int i = 0; i < NumberOfSensors; i++)
        //{
        //    sensorList[i] = new IRSensor();
        //}

        print("here we go!");
    }

    // Update is called once per frame
    //Inputting data is handled by readingThread, Outputting data in Update()
    public void updateBLE()
    {
        if (!ble.isConnected)
        {
            //Find our BLE Device
            if (!isScanning && !isFinishedScanning) ScanBleDevices();
            //Start a new thread, connect to it.
            if (isFinishedScanning && !isConnecting) StartConHandler();

        }
        else if (!isReading)
        {
            readingThread.Start();
            isReading = true;
        }
        else if (bufferReadIndex < bufferWriteIndex)
        {
            //Debug.Log("Read Index: " + (bufferReadIndex & ringBufferModulusMask));
            //Debug.Log("Write Index" + (bufferWriteIndex & ringBufferModulusMask));
            //Debug.Log("Arduino Buffer Fill: " + bufferFillLength)

            int data = ringBuffer[bufferReadIndex & ringBufferModulusMask];

            //Consider using this to send data? https://www.ascii-code.com/
            UInt32 sensor = (UInt32)data >> 26;

            //Debug.Log("Sensor " + sensor);
            switch (sensor)
            {
                case 0:
                    try
                    {
                        sensor0.updatePosition((UInt32)data);
                    }
                    catch (Exception e)
                    {
                        Debug.Log("Could not Update Position: " + e.ToString());
                    }
                    break;
                default:
                    break;
            }
            bufferReadIndex++;
            bufferFillLength--;
        }
        //NOTE: Canvas size is variable. We need to either lock the size, or scale the values to it.
    }

    public Vector2 getSensorData(int sensor, int lighthouse)
    {
        if (isReading) 
        {
            switch (lighthouse)
            {
                case 0:
                    return sensor0.lighthouse0xy;
                default:
                    return new Vector2(0,0);
                //case 1:
                //    return sensorList[1].lighthouse1xy;
            }

            throw new ArgumentOutOfRangeException("Sensor " + sensor + " is not valid");
        }
        else
        {
            throw new Exception("The client is not reading yet.");
        }
    }

    private void OnDisable()
    {
        ble.Close();
        readingThread.Abort();
    }

    private void OnApplicationQuit()
    {
        ble.Close();
        readingThread.Abort();
    }

    private void StartConHandler()
    {
        connectionThread = new Thread(ConnectBleDevice);
        connectionThread.Start();
        isConnecting = true;
    }

    private void ConnectBleDevice()
    {
        while (deviceId != null)
        {
            try
            {
                ble.Connect(deviceId, serviceUuid, characteristicUuids);
            }
            catch (Exception e)
            {
                Debug.Log("Could not establish connection to device with ID " + deviceId + "\n" + e);
            }
        }
        if (ble.isConnected)
            Debug.Log("Connected to: " + targetDeviceName);

    }

    private void ScanBleDevices()
    {
        try
        {
            scan = BLE.ScanDevices();
        }
        catch (Exception e)
        {
            Debug.Log("Continuing Forward!");
        }
        isScanning = true;
        Debug.Log("BLE.ScanDevices() started.");

        scan.Found = (_deviceId, deviceName) =>
        {
            if (deviceName != null && deviceName != "")
            {
                Debug.Log("found device with name: " + deviceName);
                if (!discoveredDevices.ContainsKey(_deviceId))
                    discoveredDevices.Add(_deviceId, deviceName);

                if (deviceId == "" && deviceName == targetDeviceName)
                {
                    Debug.Log("Found THE Device: " + deviceName + " with id: " + _deviceId);
                    deviceId = _deviceId;
                }
            }
        };
        scan.Finished = () =>
        {
            isScanning = false;
            isFinishedScanning = true;
            Debug.Log("scan finished");
            if (deviceId == null)
                deviceId = "-1";
        };

        //It eventually will come across the ID.
        while (deviceId == "")
            Thread.Sleep(500);

        scan.Cancel();

        scanningThread = null;
        isScanning = false;

        if (deviceId == "-1")
        {
            Debug.Log("no device found!");
            return;
        }
        else
        {
            Debug.Log("DEVICE ID FOUND: " + deviceId);
            return;
        }
    }

    //BEWARE: Uses an infinite loop! Should be threaded before running.
    //Uses isReading to check whether it 
    private void ReadBleData()
    {
        //This will contain leftover bytes after analysis.
        string buffer = "";
        while (isReading)
        {
            packageReceived = BLE.ReadBytes();

            byte[] toConcat = new byte[20];
            //Figure out a way to find when the transmission stops and the rando bits begin.
            //HM-10 Sends 20 Bytes before switching.
            for (int i = 0; i < 20; i++)
            {
                toConcat[i] = packageReceived[i];
                //Debug.Log("Byte " + i + ": " + packageReceived[i]);
            }
            buffer = String.Concat(buffer, Encoding.ASCII.GetString(toConcat));

            int endIndex = buffer.IndexOf(";");

            //Debug.Log("Buffer: " + buffer);
            //Debug.Log("endIndex: " + endIndex);

            //Runs through message, parsing the ints, and shipping them to the ringBuffer.
            //Don't write unless there is open space.
            while (endIndex >= 0)
            {
                if (bufferFillLength < ringBufferLength)
                {

                    if (!int.TryParse(buffer.Substring(0, endIndex), out ringBuffer[bufferWriteIndex & ringBufferModulusMask]))
                    {
                        print("Could not Parse Int.");
                        ringBuffer[bufferWriteIndex & ringBufferModulusMask] = ringBuffer[(bufferWriteIndex - 1) & ringBufferModulusMask];
                    }
                    else
                    {
                        //print("Parsed Int: " + ringBuffer[bufferWriteIndex & ringBufferModulusMask]);
                        bufferWriteIndex++;
                        bufferFillLength++;
                    }
                    try
                    {
                        string newBuffer = buffer.Substring(endIndex + 1); //Does not do CR and LF
                        buffer = newBuffer;
                        endIndex = buffer.IndexOf(";");

                        //if (endIndex == -1) Debug.Log("End Index less than 0, attempting concatination.");
                    }
                    catch (Exception e)
                    {
                        if (e is ArgumentOutOfRangeException)
                        {
                            //Debug.Log("Caught Exception... End Index less than 0, attempting concatination.");
                            buffer = "";
                            endIndex = buffer.IndexOf(";");
                        }
                        else Debug.Log("Huh... this is odd.");
                    }
                }
            }
            Thread.Sleep(1);
        }
    }

}

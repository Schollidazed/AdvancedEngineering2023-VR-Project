using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using UnityEngine;
using UnityEngine.Events;
//using MathNet.Numerics.LinearAlgebra;
//using MathNet.Numerics.LinearAlgebra.Double;

public class BLEArduinoVR : MonoBehaviour
{
    //////////////////////////////////BLE STUFF/////////////////////////////////
    BLE ble;
    BLE.BLEScan scan;

    // BLE Threads 
    Thread connectionThread, readingThread, mainThread;

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

    public bool isScanning = false, isFinishedScanning = false, isConnecting = false, isReading = false;
    /////////////////////////////////////////////////////////////////////////////


    ////////////////////////Sensor readings and Transformations//////////////////
    //public int NumberOfSensors;
    public bool InputVal;
    public UnityEvent TriggerPress;

    //public IRSensor[] sensorList;

    //Must be to the power of 2.
    volatile int[] ringBuffer;
    public int RING_BUFFER_LENGTH;
    int RING_BUFFER_MODULUS_MASK;

    public volatile int bufferFillLength = 0; //PUBLIC ONLY TO VIEW ON UNITY DASH
    volatile int bufferReadIndex = 0;
    volatile int bufferWriteIndex = 0;
    public int bufferSkipThreshold = 64; //difference before it skips to the most recent write.

    public static int DATA_LENGTH = 24; //bits long
    public static int INPUT_RIGHTSHIFT = 23;
    public static int SENSOR_RIGHTSHIFT = 18;
    public static int SENSOR_MASK = 31; //Sensor takes up 5 bits. 
    public static int LIGHTHOUSE_RIGHTSHIFT = 17;
    public static int LIGHTHOUSE_MASK;
    public static int AXIS_RIGHTSHIFT = 16;
    public static int AXIS_MASK;
    public static int SWEEP_MASK;

    //The Lighthouse Transform Matricies:
    /*public Matrix<double> LIGHTHOUSE_0_TRANSFORM_MATRIX = Matrix.Build.DenseOfColumnArrays(
         new double[][] {
            new double[] { 0, 0, 0, 0 }, //Rotation X basis
            new double[] { 0, 0, 0, 0 }, //Rotation Y basis
            new double[] { 0, 0, 0, 0 }, //Rotation Z Basis
            new double[] { 0, 0, 0, 1 }, //Position of Lighthouse
    });
    public Matrix<double> LIGHTHOUSE_1_TRANSFORM_MATRIX = Matrix.Build.DenseOfColumnArrays(
         new double[][] {
            new double[] { 0, 0, 0, 0 }, //Rotation X basis
            new double[] { 0, 0, 0, 0 }, //Rotation Y basis
            new double[] { 0, 0, 0, 0 }, //Rotation Z Basis
            new double[] { 0, 0, 0, 1 }, //Position of Lighthouse
    });*/
    /////////////////////////////////////////////////////////////////////////////
    

    //Let the code begin.

    void Start()
    {
        ringBuffer = new int[RING_BUFFER_LENGTH];
        RING_BUFFER_MODULUS_MASK = RING_BUFFER_LENGTH - 1;

        /*sensorList = new IRSensor[NumberOfSensors];
        for (int i = 0; i < NumberOfSensors; i++)
        {
            //sensorList[i] = new IRSensor(LIGHTHOUSE_0_TRANSFORM_MATRIX, LIGHTHOUSE_1_TRANSFORM_MATRIX);
            sensorList[i] = new IRSensor();
            Debug.Log("Sensor " + i + " Created.");
        }*/

        readingThread = new Thread(ReadBleData);
        readingThread.Priority = System.Threading.ThreadPriority.Normal;

        ble = new BLE();
        
        
        Debug.Log("here we go!");
        mainThread.Start();
    }

    // Update is called once per frame
    //Inputting data is handled by readingThread, Outputting data in Update()
    void Update()
    {
        if (!ble.isConnected)
        {
            //Find our BLE Device
            if (!isScanning && !isFinishedScanning) ScanBleDevices();
            //Start a new thread, connect to it.
            if (isFinishedScanning && !isConnecting) StartConHandler();

        }
        else if (!isConnecting && !isReading)
        {
            readingThread.Start();
            isReading = true;
        }
        else if (isReading && bufferReadIndex < bufferWriteIndex)
        {
            //Debug.Log("Read Index: " + (bufferReadIndex & RING_BUFFER_MODULUS_MASK));
            //Debug.Log("Write Index" + (bufferWriteIndex & RING_BUFFER_MODULUS_MASK));
            //Debug.Log("Arduino Buffer Fill: " + bufferFillLength);

            int data = ringBuffer[bufferReadIndex & RING_BUFFER_MODULUS_MASK];

            //Consider using this to send data? https://www.ascii-code.com/
            InputVal = (data == 0) ? true : false; //Can only be 0 or 1, so store as a bool :)
            if (InputVal)
            {
                Debug.Log("Triggered");
                TriggerPress.Invoke();
            }

            //int sensor = (data >> SENSOR_RIGHTSHIFT) & SENSOR_MASK; //This gets rid of the input val.
            //Debug.Log("Updating: Sensor " + sensor);
            try
            {
                //sensorList[sensor].updatePosition( (UInt32) data );
            }
            catch (Exception e)
            {
                Debug.Log("Could not Update Position: " + e.ToString());
            }
            bufferReadIndex++;
            bufferFillLength--;
        }
    }

 /*   public double[] getSensorData(int sensor, int lighthouse)
    {
        //Debug.Log("Called for sensor: " + sensor + " and Lighthouse: " + lighthouse);
        if (isReading) 
        {
            switch (lighthouse)
            {
                case 0:
                    return sensorList[sensor].lighthouse0xy;
                case 1:
                    return sensorList[sensor].lighthouse1xy;
            }

            throw new ArgumentOutOfRangeException("Sensor " + sensor + " is not valid");
        }
        else
        {
            throw new Exception("The client is not reading yet.");
        }
    }*/

    private void OnDisable()
    {
        killConnection();
    }

    private void OnApplicationQuit()
    {
        killConnection();
    }

    private void StartConHandler()
    {
        isConnecting = true;
        connectionThread = new Thread(ConnectBleDevice);
        connectionThread.Start();
    }

    private void ConnectBleDevice()
    {
        while (!ble.isConnected)
        {
            Debug.Log("Connecting...");
            try
            {
                ble.Connect(deviceId, serviceUuid, characteristicUuids);
            }
            catch (Exception e)
            {
                Debug.Log("Could not establish connection to device with ID " + deviceId + "\n" + e);
            }
        }
        Debug.Log("Connected to: " + targetDeviceName);
        isConnecting = false;
        connectionThread.Abort();
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

        while (isReading)
        {
            if (bufferFillLength >= bufferSkipThreshold)
            {
                Debug.Log("OVERFLOW!!");
                bufferReadIndex = bufferWriteIndex - 1;
                bufferFillLength = 1;
            }

            packageReceived = BLE.ReadBytes();

            byte[] toConcat = new byte[20];
            bool latch = true;
            //Figure out a way to find when the transmission stops and the rando bits begin.
            //HM-10 Sends 20 Bytes before switching.
            for (int i = 0; (i < 20) /*&& latch*/; i++)
            {
                toConcat[i] = packageReceived[i];
                //Debug.Log("Byte " + i + ": " + packageReceived[i]);
                //if (toConcat[i] == 59) latch = false;
            }
            //Debug.Log("HUAGGHHH: " + Encoding.ASCII.GetString(toConcat));

            string data = Encoding.ASCII.GetString(toConcat);

            int endIndex = data.IndexOf(";");

            Debug.Log("DATA: " + data);
            //Debug.Log("endIndex: " + endIndex);

            //Runs through message, parsing the ints, and shipping them to the ringBuffer.
            //Don't write unless there is open space.
            while (endIndex >= 0)
            {
                if (bufferFillLength < RING_BUFFER_LENGTH)
                {
                    if (!int.TryParse(data.Substring(0, endIndex), out ringBuffer[bufferWriteIndex & RING_BUFFER_MODULUS_MASK]))
                    {
                        Debug.Log("Could not Parse Int.");
                        ringBuffer[bufferWriteIndex & RING_BUFFER_MODULUS_MASK] = ringBuffer[(bufferWriteIndex - 1) & RING_BUFFER_MODULUS_MASK];
                    }
                    else
                    {
                        //Debug.Log("Parsed Int: " + ringBuffer[bufferWriteIndex & RING_BUFFER_MODULUS_MASK]);
                        bufferWriteIndex++;
                        bufferFillLength++;
                    }
                    try
                    {
                        string newBuffer = data.Substring(endIndex + 1); //Does not do CR and LF
                        data = newBuffer;
                        endIndex = data.IndexOf(";");

                        //if (endIndex == -1) Debug.Log("End Index less than 0, attempting concatination.");
                    }
                    catch (Exception e)
                    {
                        if (e is ArgumentOutOfRangeException)
                        {
                            //Debug.Log("Caught Exception... End Index less than 0, attempting concatination.");
                            data = "";
                            endIndex = data.IndexOf(";");
                        }
                        else Debug.Log("Huh... this is odd.");
                    }
                   }
            }
        }
    }
    public void killConnection()
    {
        Debug.Log("Boom. BLE Dead.");
        ble.Close();
        readingThread.Abort();
    }
}

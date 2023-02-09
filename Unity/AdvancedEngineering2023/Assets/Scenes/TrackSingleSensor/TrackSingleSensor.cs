using System;
using System.Collections;
using System.Collections.Generic;
using System.Security.Cryptography;
using UnityEngine;

public class TrackSingleSensor : MonoBehaviour
{
    public bool hasFoundBLE = false;
    public bool isSubscribed = false;

    IRSensor sensor1 = new IRSensor();

    public string bleDeviceId = "ADV-ENG2023";
    public string bleServiceId = "0000FFE0-0000-1000-8000-00805F9B34FB";
    public string bleCharacteristicId = "0000FFE1-0000-1000-8000-00805F9B34FB";

    Dictionary<string, string> characteristicNames = new Dictionary<string, string>();
    
    string lastError;


    // Start is called before the first frame update
    void Start()
    {
        //Find our BLE Device, and connect to it
        FindBLEDevice(bleDeviceId);
        BleApi.SubscribeCharacteristic(bleDeviceId, bleServiceId, bleCharacteristicId, false);
    }

    // Update is called once per frame
    void Update()
    {   
        //Parse data
        BleApi.BLEData data = new BleApi.BLEData();
        BleApi.PollData(out data, false);
        String ParsedData = BitConverter.ToString(data.buf, 0, data.size);

        //Consider using this to send data? https://www.ascii-code.com/
    }

    private void OnApplicationQuit()
    {
        BleApi.Quit();
    }

    //Find the device:
    //Scan devices until the right one is found.
    private void FindBLEDevice( String deviceID)
    {
        BleApi.StartDeviceScan();
        int count = 0;
        do
        {
            BleApi.DeviceUpdate device = new BleApi.DeviceUpdate();
            BleApi.PollDevice(ref device, false);
            // Find device with only name selected, and is connectable == true
            Debug.Log(device.name);
            Debug.Log(count);
            if (device.name == deviceID && device.isConnectable == true)
            {
                //Stop scanning. We found the device.
                BleApi.StopDeviceScan();
                hasFoundBLE = true;
                //status will now be == Finished.
            }
            count++;
        } while (!hasFoundBLE);
    }

    /*
    private UInt32 parseData(out BleApi.BLEData d)
    {
        
    }
    */
}

using System;
using System.Collections;
using System.Collections.Generic;
using System.Security.Cryptography;
using UnityEngine;

public class TrackSingleSensor : MonoBehaviour
{
    public bool hasFoundBLE = false;
    public bool isSubscribed = false;

    public Canvas lighthouse0;

    public RectTransform sensor0rep;
    IRSensor sensor0 = new IRSensor();
    

    public string bleDeviceId = "ADV-ENG2023";
    public string bleServiceId = "0000FFE0-0000-1000-8000-00805F9B34FB";
    public string bleCharacteristicId = "0000FFE1-0000-1000-8000-00805F9B34FB";

    Dictionary<string, string> characteristicNames = new Dictionary<string, string>();
    
    string lastError;


    // Start is called before the first frame update
    void Start()
    {
        /*
        //Find our BLE Device, and connect to it
        FindBLEDevice(bleDeviceId);
        BleApi.SubscribeCharacteristic(bleDeviceId, bleServiceId, bleCharacteristicId, false);
        */
    }

    // Update is called once per frame
    void Update()
    {
        int lighthouse0Width = (int) lighthouse0.pixelRect.width;
        int lighthouse0Height = (int) lighthouse0.pixelRect.height;

        //Parse data
        /*BleApi.BLEData data = new BleApi.BLEData();
        BleApi.PollData(out data, false);
        UInt32 ParsedData = BitConverter.ToUInt32(data.buf, 0);

        //Consider using this to send data? https://www.ascii-code.com/
        int sensor = (int) ParsedData >> 26;
        double x0, y0, x1, y1;

        switch (sensor)
        {
            case 0: sensor0.updatePosition(ParsedData, out x0, out y0, out x1, out y1); break;
            default: break;
        }*/

        sensor0rep.anchoredPosition = new Vector2(lighthouse0Width, lighthouse0Height);



        //NOTE: Cavas size is variable. We need to either lock the size, or scale the values to it.
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

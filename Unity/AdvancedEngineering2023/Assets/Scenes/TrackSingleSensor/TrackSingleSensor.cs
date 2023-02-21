using System;
using System.Collections;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using UnityEngine;

public class TrackSingleSensor : MonoBehaviour
{
    public bool hasFoundBLE = false;
    public bool hasFoundBLEService = false;
    public bool hasFoundBLECharacteristic = false;
    public bool isSubscribed = false;

    public Canvas lighthouse0;

    public RectTransform sensor0rep;
    IRSensor sensor0 = new IRSensor();

    BLE ble;
    BLE.BLEScan scan;

    public string bleDeviceID = "BluetoothLE#BluetoothLE58:96:1d:5b:b7:00-a0:6c:65:cf:7e:67";
    public string bleDeviceName = "ADV-ENG2023";

    public string bleServiceID = "0000FFE0-0000-1000-8000-00805F9B34FB";

    public string bleCharacteristicID = "0000FFE1-0000-1000-8000-00805F9B34FB";
    public string bleCharacteristicName = "DSDTECH";
    

    public bool hasSubscribed = false;

    Dictionary<string, Dictionary<string, string>> devices = new Dictionary<string, Dictionary<string, string>>();
    ArrayList services = new ArrayList();
    Dictionary<string, string> characteristicNames = new Dictionary<string, string>();
    int remoteAngle, lastRemoteAngle;
    
    string lastError;

    // BLE Threads 
    Thread scanningThread, connectionThread, readingThread;


    // Start is called before the first frame update
    void Start(){
        ble = new BLE();
        readingThread = new Thread(ReadBleData);
        print("here we go!");
    }

    // Update is called once per frame
    void Update()
    {
        int lighthouse0Width = (int)lighthouse0.pixelRect.width;
        int lighthouse0Height = (int)lighthouse0.pixelRect.height;

        if (!hasSubscribed){
            //Find our BLE Device, and connect to it
            BleApi.StartDeviceScan();
            FindBLEDevice(bleDeviceID, bleDeviceName);
            BleApi.StopDeviceScan();
            print("Found device, moving on to the services");

            print("Scanning Services...");
            BleApi.ScanServices(bleDeviceID);
            print("Scanned Services. Here's what we found:");
            findBLEService(bleServiceID);
            print("Found service, moving on to characteristics");

            print("Scanning Characteristics...");
            BleApi.ScanCharacteristics(bleDeviceName, bleServiceID);
            print("Scanned Characteristics. Here's what we found:");
            findBLECharacteristic(bleCharacteristicID, bleCharacteristicName);
            print("Found characteristic, attempting subscription");

            if (hasFoundBLE && hasFoundBLEService && hasFoundBLECharacteristic)
            {
                if (BleApi.SubscribeCharacteristic(bleDeviceID, bleServiceID, bleCharacteristicID, false))
                {
                    hasSubscribed = true;
                }
            }
            print("Hooray! Subscribed!");
        }
        else{ 
            //Parse data
            BleApi.BLEData data = new BleApi.BLEData();
            BleApi.PollData(out data, false);
            UInt32 ParsedData = BitConverter.ToUInt32(data.buf, 0);
            print(ParsedData);

            //Consider using this to send data? https://www.ascii-code.com/
            int sensor = (int)ParsedData >> 26;
            double x0, y0, x1, y1;

            switch (sensor)
            {
                case 0:
                    sensor0.updatePosition(ParsedData, out x0, out y0, out x1, out y1);
                    sensor0rep.anchoredPosition = new Vector2((float)x0, (float)y0);
                    break;
                default: break;
            }
        }

        //NOTE: Cavas size is variable. We need to either lock the size, or scale the values to it.
    }

    private void OnApplicationQuit()
    {
        ble.Close();
    }

    private void ReadBleData(object obj)
    {
        byte[] packageReceived = BLE.ReadBytes();
        // Convert little Endian.
        // In this example we're interested about an angle
        // value on the first field of our package.
        remoteAngle = packageReceived[0];
        Debug.Log("Angle: " + remoteAngle);
        //Thread.Sleep(100);
    }


    //Find the device:
    //Scan devices until the right one is found.
    private void FindBLEDevice(String deviceID, String deviceName)
    {
        print("Entered findBLEDevice Method");
        BleApi.ScanStatus status;
        BleApi.DeviceUpdate device = new BleApi.DeviceUpdate();
        do
        {
            status = BleApi.PollDevice(ref device, false);
            // Find device with only name selected, and is connectable == true
            if(status == BleApi.ScanStatus.AVAILABLE)
            {
                if (!devices.ContainsKey(device.id))
                    devices[device.id] = new Dictionary<string, string>() {
                            { "name", "" },
                            { "isConnectable", "False" }
                        };
                if (device.nameUpdated)
                    devices[device.id]["name"] = device.name;
                if (device.isConnectableUpdated)
                    devices[device.id]["isConnectable"] = device.isConnectable.ToString();
                if (devices[device.id]["name"] != "" && device.isConnectable == true)
                {
                    print("Name: " + devices[device.id]["name"] + " Device: " + device.id);
                    string message = "";
                    byte[] bytes = Encoding.Default.GetBytes(devices[device.id]["name"]);
                    foreach (byte b in bytes)
                    {
                        message += b.ToString() + " ";
                    }
                    print(message);

                    if (device.id == deviceID && deviceName == devices[device.id]["name"])
                    {
                        print("Found a match!");
                        //bleDeviceName = devices[device.id]["name"]; 
                        hasFoundBLE = true;
                        return;
                    }
                }
            }
        } while (!hasFoundBLE);
    }

    private void findBLEService(string serviceUUID)
    {
        print("Entered findBLEService Method");
        BleApi.ScanStatus status;

        do
        {
            print("Entered while loop 1");
            BleApi.Service service = new BleApi.Service();

            do
            {
                print("Entered while loop 2");
                status = BleApi.PollService(out service, false);
                if (status == BleApi.ScanStatus.PROCESSING)
                {
                    print("SHUDDUP I'M STILL SCANNINGUHHHH");
                }
                else if (status == BleApi.ScanStatus.AVAILABLE)
                {
                    print("Found a Serivce!");
                    print(service.uuid);
                    services.Add(service.uuid);
                }
                else if (status == BleApi.ScanStatus.FINISHED)
                {
                    print("Finished Scanning");
                    foreach (String s in services)
                    {
                        if (s == serviceUUID)
                            hasFoundBLEService = true;
                    }
                }
            } while (status == BleApi.ScanStatus.AVAILABLE);

        } while (!hasFoundBLEService);
    }

    private void findBLECharacteristic(string characteristicUUID, string characteristicName)
    {
        print("Entered findBLECharacteristic Method");
        BleApi.ScanStatus status;
        BleApi.Characteristic characteristic = new BleApi.Characteristic();
        do
        {
            status = BleApi.PollCharacteristic(out characteristic, false);
            if (status == BleApi.ScanStatus.AVAILABLE)
            {
                //if user description is != that, then goes with userdescriion. if not, then goes with UUID
                string name = characteristic.userDescription != "no description available" ?
                    characteristic.userDescription : characteristic.uuid;
                characteristicNames[name] = characteristic.uuid;
                print("Name: " + name + " UUID: " + characteristic.uuid);
            }
            else if (status == BleApi.ScanStatus.FINISHED)
            {
                if (characteristicNames.ContainsKey(characteristicName) &&
                    characteristicNames.ContainsValue(characteristicUUID))
                {
                    hasFoundBLECharacteristic = true;
                }
            }
        } while (!hasFoundBLECharacteristic);
    }
    /*
    private UInt32 parseData(out BleApi.BLEData d)
    {
        
    }
    */
}

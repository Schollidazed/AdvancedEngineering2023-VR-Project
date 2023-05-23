using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using System.IO.Ports;

public class SerialPortBlaster : MonoBehaviour
{
    public SerialPort ser = new SerialPort("\\\\.\\COM10", 9600);


    // Start is called before the first frame update
    void Start()
    {
        ser.Open();
    }

    // Update is called once per frame
    void Update()
    {
        Debug.Log(ser.ReadLine());
    }
}

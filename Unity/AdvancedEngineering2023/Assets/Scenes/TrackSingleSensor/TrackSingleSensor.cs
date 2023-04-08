using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using Unity.VisualScripting.Antlr3.Runtime;
using UnityEngine;
using UnityEngine.XR;

public class TrackSingleSensor : MonoBehaviour
{
    public RectTransform lighthouse0;

    double lighthouse0Width;
    double lighthouse0Height;

    public RectTransform sensor0rep;

    private BLEArduinoVR arduino;

    // Start is called before the first frame update
    void Start()
    {
        lighthouse0.sizeDelta.Set(lighthouse0.sizeDelta.y, lighthouse0.sizeDelta.y);
        lighthouse0Width = (double)lighthouse0.sizeDelta.x;
        lighthouse0Height = (double)lighthouse0.sizeDelta.y;

        arduino = GetComponent<BLEArduinoVR>();
        arduino.startBLE();
    }

    // Update is called once per frame
    // Inputting data is handled by readingThread, Outputting data in applyThread
    void Update()
    {
        arduino.updateBLE();

        //Debug.Log("Data: " + arduino.getSensorData(0, 0).x + " / " + arduino.getSensorData(0, 0).y);

        sensor0rep.anchoredPosition = new Vector2(
            (float)(arduino.getSensorData(0, 0).x * (lighthouse0Width / 180.0)),
            (float)(arduino.getSensorData(0, 0).y * (lighthouse0Width / 180.0))
        );
    }
}

    
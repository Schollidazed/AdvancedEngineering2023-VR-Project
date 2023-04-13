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
using UnityEngine.UI;
using UnityEngine.XR;

public class TrackSingleSensor : MonoBehaviour
{
    public RectTransform lighthouse0;
    public RectTransform lighthouse1;

    double lighthouse0Width;
    double lighthouse0Height;

    double lighthouse1Width;
    double lighthouse1Height;

    public RectTransform sensor0_0rep;
    public RectTransform sensor0_1rep;

    public RectTransform sensor1_0rep;
    public RectTransform sensor1_1rep;

    private BLEArduinoVR arduino;

    // Start is called before the first frame update
    void Start()
    {
        lighthouse0.sizeDelta.Set(lighthouse0.sizeDelta.y, lighthouse0.sizeDelta.y);
        lighthouse0Width = (double)lighthouse0.sizeDelta.x;
        lighthouse0Height = (double)lighthouse0.sizeDelta.y;

        lighthouse1.sizeDelta.Set(lighthouse1.sizeDelta.y, lighthouse1.sizeDelta.y);
        lighthouse1Width = (double)lighthouse1.sizeDelta.y;
        lighthouse1Height = (double)lighthouse1.sizeDelta.y;

        arduino = GetComponent<BLEArduinoVR>();
        arduino.startBLE();
    }

    // Update is called once per frame
    // Inputting data is handled by readingThread, Outputting data in applyThread
    void Update()
    {
        arduino.updateBLE();

        //Debug.Log("Data: " + arduino.getSensorData(1, 1)[0] + " / " + arduino.getSensorData(1, 1)[1]);
        double[] sensor0_0 = arduino.getSensorData(0, 0);
        double[] sensor1_0 = arduino.getSensorData(1, 0);
        double[] sensor0_1 = arduino.getSensorData(0, 1);
        double[] sensor1_1 = arduino.getSensorData(1, 1);  

        for(int i = 0; i < 2; i++)
        {
            sensor0_0rep.GetComponent<Image>().enabled = sensor0_0[i] == -1 ? false : true;
            sensor0_1rep.GetComponent<Image>().enabled = sensor0_1[i] == -1 ? false : true;
            sensor1_0rep.GetComponent<Image>().enabled = sensor1_0[i] == -1 ? false : true;
            sensor1_1rep.GetComponent<Image>().enabled = sensor1_1[i] == -1 ? false : true;
        }

        sensor0_0rep.anchoredPosition = new Vector2(
            (float)(sensor0_0[0]*(lighthouse0Width / 180.0) ),
            (float)(sensor0_0[1]*(lighthouse0Width / 180.0) )
        );
        sensor1_0rep.anchoredPosition = new Vector2(
            (float)(sensor1_0[0] * (lighthouse0Width / 180.0)),
            (float)(sensor1_0[1] * (lighthouse0Width / 180.0))
        );

        sensor0_1rep.anchoredPosition = new Vector2(
            (float)(sensor0_1[0] * (lighthouse0Width / 180.0)),
            (float)(sensor0_1[1] * (lighthouse0Width / 180.0))
        );
        sensor1_1rep.anchoredPosition = new Vector2(
            (float)(sensor1_1[0] * (lighthouse0Width / 180.0)),
            (float)(sensor1_1[1] * (lighthouse0Width / 180.0))
        );
    }
}

    
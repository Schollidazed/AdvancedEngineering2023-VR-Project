using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using TMPro;
using System;

public class Timer : MonoBehaviour
{
    public float timeValue;
    public TMP_Text timerText;

    public bool hasStarted = false;

    float totalTimeElapsed;

    public UnityEvent gameEnd = new UnityEvent();

    void Start(){}
    void Update()
    {
        if (hasStarted)
        {
            if ( (timeValue - totalTimeElapsed) > 0)
            {
                timerText.text = "Time Left: " + (timeValue - totalTimeElapsed);
            }
            else if ( (timeValue - totalTimeElapsed) <= 0)
            {
                timerText.text = "Time's Up!";
                hasStarted = false;
                gameEnd.Invoke();
            }
            totalTimeElapsed += Time.deltaTime;
        }
    }
    public void StartTimer()
    {
        hasStarted = true;
        totalTimeElapsed = 0;
    }
}

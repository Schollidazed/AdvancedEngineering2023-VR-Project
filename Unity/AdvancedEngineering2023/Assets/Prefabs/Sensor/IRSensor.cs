using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class IRSensor : MonoBehaviour
{
    private int lighthouse;
    private int axis;

    private int sweepTime;

    private double xAngle, yAngle;

    //Assume that runtime detected that it belongs to this sensor.

    void Start()
    {
        
    }

    void Update()
    {

    }
    public void updatePosition(UInt32 data)
    {
        processData(data);
        setAngle();
    }

    //parses sent data from runtime, gives lighthouse, axis, and sweeptime value.
    private void processData(UInt32 data)
    {
        for (int i = 25; i >= 0; i--)
        {
            int bit = (int) (data >> i) & 1;
            //SerialTX.print(bit);
            //if (i >= 26)
            //{
            //  sensor <<= 1;
            //  sensor |= bit;
            //}
            if (i == 25)
            {
                this.lighthouse |= bit;
            }
            else if (i == 24)
            {
                this.axis |= bit;
            }
            else
            {
                this.sweepTime <<= 1;
                this.sweepTime |= bit;
            }
        }
    }

    private void setAngle()
    {
        //Check Validity of Sweeptime
        if (sweepTime >= 8000){return;}

        //Check axis, and apply correct angle
        if (axis == 0) { xAngle = sweepTimeToDegrees(sweepTime); }
        else { yAngle = sweepTimeToDegrees(sweepTime); }
    }

    private double sweepTimeToDegrees(int time) { return ( (360 * (double) time) * (60 / (10 ^ 6))); }

    
}

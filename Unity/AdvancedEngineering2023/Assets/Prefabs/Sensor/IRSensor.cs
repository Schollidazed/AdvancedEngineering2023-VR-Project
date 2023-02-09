using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class IRSensor : MonoBehaviour
{
    private int lighthouse;
    private int axis;

    private int sweepTime;

    private double xAngleTmp, yAngleTmp;

    //Should we include a buffer for incoming data?

    //Assume that runtime detected that it belongs to this sensor.

    public void updatePosition(UInt32 data, out double x0, out double y0, out double x1, out double y1)
    {
        ProcessData(data);
        SetAngle(out x0, out y0, out x1, out y1);
    }

    //parses sent data from runtime, gives lighthouse, axis, and sweeptime value.
    private void ProcessData(UInt32 data)
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
    //Sets the angle using a predefined function. 
    private void SetAngle(out double x0, out double y0, out double x1, out double y1)
    {
        //Check Validity of Sweeptime
        if (sweepTime >= 8000){x0 = -1; x1 = -1; y0 = -1; y1 = -1; return;}

        //Check axis, and apply correct angle
        if (axis == 0) { xAngleTmp = sweepTimeToDegrees(sweepTime); }
        else { yAngleTmp = sweepTimeToDegrees(sweepTime); }

        if (lighthouse == 0) {x0 = xAngleTmp; y0 = yAngleTmp; x1 = -1; y1 = -1; return;}
        else { x0 = -1; y0 = -1; x1 = xAngleTmp; y1 = yAngleTmp; return;}
    }
    private double sweepTimeToDegrees(int time) { return ( (360 * (double) time) * (60 / (10 ^ 6))); }

    
}

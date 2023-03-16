using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using UnityEngine;
using UnityEngine.XR;

public class IRSensor
{
    private int lighthouse;
    private int axis;

    private uint sweepTime;

    private double xAngleTmp, yAngleTmp;

    public Vector2 lighthouse0xy;
    public Vector2 lighthouse1xy;

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
        uint tempTime = 0;
        for (int i = 25; i >= 0; i--)
        {
            int bit = (int) (data >> i) & 1;
            //Debug.Log("Num of Loops: " + i + "Bit: " + bit);
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
                tempTime <<= 1;
                tempTime |= (uint) bit;
            }
        }
        this.sweepTime = tempTime;

        //Debug.Log("Lighthouse: " + this.lighthouse);
        //Debug.Log("Axis: " + this.axis);
        //Debug.Log("Sweep Time: " + this.sweepTime);
    }
    //Sets the angle using a predefined function. 
    private void SetAngle(out double x0, out double y0, out double x1, out double y1)
    {
        //Check Validity of Sweeptime
        if (sweepTime >= 8000){x0 = -1; x1 = -1; y0 = -1; y1 = -1; return;}

        //Check axis, and apply correct angle
        if (axis == 0) { 
            xAngleTmp = sweepTimeToDegrees(sweepTime);
            //Debug.Log("X: " + xAngleTmp);
        }
        else { 
            yAngleTmp = sweepTimeToDegrees(sweepTime);
            //Debug.Log("Y: " + yAngleTmp);
        }

        if (lighthouse == 0) {x0 = xAngleTmp; y0 = yAngleTmp; x1 = -1; y1 = -1; return;}
        else { x0 = -1; y0 = -1; x1 = xAngleTmp; y1 = yAngleTmp; return;}
    }
    private double sweepTimeToDegrees(uint time) { 
        return ( (360.0 * (double) time) * (60.0 / Math.Pow(10.0, 6.0) ) ); 
    }

    
}

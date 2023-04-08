using System;
using UnityEngine;
public class IRSensor
{
    private int lighthouse;
    private int axis;

    private uint sweepTime;

    private double xAngleTmp, yAngleTmp;

    public Vector2 lighthouse0xy;
    public Vector2 lighthouse1xy;

    //Assume that runtime detected that it belongs to this sensor.

    public IRSensor()
    {

    }

    public void updatePosition(UInt32 data)
    {
        ProcessData(data);
        SetAngle();
    }

    //parses sent data from runtime, gives lighthouse, axis, and sweeptime value.
    private void ProcessData(UInt32 data)
    {
        uint tempTime = 0;
        for (int i = 25; i >= 0; i--)
        {
            int bit = (int) (data >> i) & 1;
            //We should already have the Sensor data, no need to process that.
            if (i == 25)
            {
                this.lighthouse = bit;
            }
            else if (i == 24)
            {
                this.axis = bit;
            }
            else
            {
                tempTime <<= 1;
                tempTime |= (uint) bit;
            }
        }
        this.sweepTime = tempTime;

        Debug.Log("Processed: " + data + " Lighthouse: " + this.lighthouse + " Axis: " + this.axis + " Sweep Time: " + this.sweepTime);
    }
    //Sets the angle using a predefined function. 
    private void SetAngle()
    {
        //Check Validity of Sweeptime
        if (this.sweepTime >= 8333){Debug.Log("Sweep Time Was Too High."); return;}

        //Check axis, and apply correct angle
        //0 axis = x angle
        //1 axis = y angle
        if (axis == 0) {
            this.xAngleTmp = sweepTimeToDegrees(this.sweepTime);
            //Debug.Log("X: " + xAngleTmp);
        }
        else { 
            this.yAngleTmp = sweepTimeToDegrees(this.sweepTime);
            //Debug.Log("Y: " + yAngleTmp);
        }

        if (this.lighthouse == 0) 
        {
            this.lighthouse0xy.x = (float)(this.xAngleTmp);
            this.lighthouse0xy.y = (float)(this.yAngleTmp);
            //Debug.Log("Lighthouse is 0"); 
            return;
        }
        else if (this.lighthouse == 1)
        { 
            this.lighthouse1xy.x = (float)(this.xAngleTmp);
            this.lighthouse1xy.y = (float)(this.yAngleTmp);
            //Debug.Log("Lighthouse is 1"); 
            return;
        }
    }
    private double sweepTimeToDegrees(uint time) { 
        return ( (360.0 * (double) time) * (60.0 / Math.Pow(10.0, 6.0) ) ); 
    }

    
}

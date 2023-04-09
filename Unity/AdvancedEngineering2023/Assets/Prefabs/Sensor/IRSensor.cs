using System;
using UnityEngine;
public class IRSensor
{
    private int lighthouse;
    private int axis;

    private UInt16 sweepTime;

    //NOTE: If the value in these is < 0, then it shouldn't be treated as a real value.
    public double[] lighthouse0xy = new double[2];
    public double[] lighthouse1xy = new double[2];

    

    //Assume that runtime detected that it belongs to this sensor.

    public IRSensor(){}

    public void updatePosition(UInt32 data)
    {
        //Debug.Log("Proccessing " + data);
        ProcessData(data);
        SetAngle();
    }

    //parses sent data from runtime, gives lighthouse, axis, and sweeptime value.
    private void ProcessData(UInt32 data)
    {
        UInt16 tempTime = 0;
        for (int i = BLEArduinoVR.DATA_LENGTH; i >= 0; i--)
        {
            int bit = (int) (data >> i) & 1;
            //We should already have the Sensor data, no need to process that.
            if (i == BLEArduinoVR.LIGHTHOUSE_RIGHTSHIFT)
            {
                this.lighthouse = bit;
            }
            else if (i == BLEArduinoVR.AXIS_RIGHTSHIFT)
            {
                this.axis = bit;
            }
            else
            {
                tempTime <<= 1;
                tempTime |= (UInt16) bit;
            }
        }
        this.sweepTime = tempTime;

        //Debug.Log("Lighthouse: " + this.lighthouse + " Axis: " + this.axis + " Sweep Time: " + this.sweepTime);
    }
    //Sets the angle using a predefined function. 
    private void SetAngle()
    {
        //Check Validity of Sweeptime
        //if Sweep time == 0x4000, then it was not swept.
        if (this.sweepTime >= 8333)
        {
            //Debug.Log("Sweep Time Was Too High."); 
            if(this.sweepTime == 0x4000)
            {
                //Debug.Log("Sensor was not swept."); 
            }
            switch (this.lighthouse)
            {
                case 0:
                    this.lighthouse0xy[this.axis] = (-1);
                    break;
                case 1:
                    this.lighthouse1xy[this.axis] = (-1);
                    break;
            }
            return;
        }

        double newAngle = sweepTimeToDegrees(this.sweepTime); ;

        switch(this.lighthouse)
        {
            case 0:
                this.lighthouse0xy[this.axis] = (newAngle);
                break;
            case 1:
                this.lighthouse1xy[this.axis] = (newAngle);
                break;
        }
    }
    private double sweepTimeToDegrees(UInt16 time) { 
        return ( (360.0 * time) * (60.0 / Math.Pow(10.0, 6.0) ) ); 
    }

    
}

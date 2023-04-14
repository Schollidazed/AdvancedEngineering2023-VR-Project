using System;
using UnityEngine;
//using MathNet.Numerics.LinearAlgebra;
//using MathNet.Numerics.LinearAlgebra.Double;
using System.Runtime.InteropServices.WindowsRuntime;
//using MathNet.Numerics;
using System.Collections.Generic;
using System.Security.Cryptography;

public class IRSensor
{
    private int lighthouse;
    private int axis;

    private UInt16 sweepTime;

    //NOTE: If the value in these is < 0, then it shouldn't be treated as a real value.
    public double[] lighthouse0xy = new double[2];
    public double[] lighthouse1xy = new double[2];

    //This Matrix/Vector library comes from Math.net
    //https://numerics.mathdotnet.com/Matrix.html
    /*private Matrix<double> Lighthouse0TransformMatrix;
    private Matrix<double> Lighthouse1TransformMatrix;*/

    //Vector 3d is a library that enables double precision. but with vector calculations. It's VERY user friendly.
    //It can be found here.
    //https://github.com/sldsmkd/vector3d
    public Vector3d XYZ = new Vector3d();
    public double distFromIntersect;

    public IRSensor()
    {

    }

    //Assume that runtime detected that it belongs to this sensor.
    /*public IRSensor(Matrix<double> LH0Matrix, Matrix<double> LH1Matrix) 
    {
        Lighthouse0TransformMatrix = LH0Matrix;
        Lighthouse1TransformMatrix = LH1Matrix;
    }*/

    public void updatePosition(UInt32 data)
    {
        //Debug.Log("Proccessing " + data);
        ProcessData(data);
        SetAngle();
        //updateXYZ();
    }

    //parses sent data from runtime, gives lighthouse, axis, and sweeptime value.
    private void ProcessData(UInt32 data)
    {
        UInt16 tempTime = 0;
        for (int i = BLEArduinoVR.LIGHTHOUSE_RIGHTSHIFT; i >= 0; i--)
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

    /*private void updateXYZ()
    {
        //Get the ray lines of each lighthouse:
        Vector<double> LH0ray = calculateUnitRayVector(lighthouse0xy[0], lighthouse0xy[1], Lighthouse0TransformMatrix);
        Vector<double> LH1ray = calculateUnitRayVector(lighthouse1xy[0], lighthouse1xy[1], Lighthouse1TransformMatrix);

        //Find the intersection of the ray lines.
        intersectLines(
            Lighthouse0TransformMatrix.RemoveRow(3).Column(3), LH0ray,
            Lighthouse1TransformMatrix.RemoveRow(3).Column(3), LH1ray
        );

        //XYZ is updated inside of this function (intersect lines).
        //We've sucessfully located the XYZ of the Sensor relative to the origin!
    }
    private Vector<double> calculateUnitRayVector(double X, double Y, Matrix<double> TransformMatrix)
    {
        //Normal vector to X plane
        Vector3d x = new Vector3d( Math.Cos(X), 0, -Math.Sin(X) );
        //Normal vector to Y plane
        Vector3d y = new Vector3d( 0, Math.Cos(Y), Math.Sin(Y) );

        //Find where the planes intersect. Calculate the ray vector, and normalize it.
        Vector3d ray = Vector3d.Cross(y, x).normalized;
        double[] rayArray = { ray.x, ray.y, ray.z };
        Vector<double> rayVect = Vector<double>.Build.DenseOfArray(rayArray);
        Matrix<double> RotMatrix = TransformMatrix.RemoveRow(3).RemoveColumn(3);
        //Rotate the vector to return to world space.
        return RotMatrix * rayVect;
    }
    private void intersectLines(Vector<double> LH0pos, Vector<double> LH0Ray, Vector<double> LH1pos, Vector<double> LH1Ray)
    {
        //Distance between Positions
        Vector<double> w0 = LH0pos - LH1pos;

        //Dot Product time.
        double a, b, c, d, e;
        a = LH0Ray.DotProduct(LH0Ray);
        b = LH0Ray.DotProduct(LH1Ray);
        c = LH1Ray.DotProduct(LH1Ray);
        d = LH0Ray.DotProduct(w0);
        e = LH1Ray.DotProduct(w0);

        double denom = a * c - b * b;
        if (Math.Abs(denom) < Math.Pow(10, -5)) return;

        // Closest point to 2nd line on 1st line
        double t0 = (b * e - c * d) / denom;
        Vector<double> pt1 = LH0Ray.Multiply(t0) + LH0pos;

        // Closest point to 1st line on 2nd line
        double t1 = (a * e - b * d) / denom;
        Vector<double> pt2 = LH1Ray.Multiply(t1) + LH1pos;

        // Result is in the middle
        Vector<double> res = (pt1 + pt2).Multiply(0.5);
        this.XYZ.Set(res.ToArray()[0], res.ToArray()[1], res.ToArray()[2]);

        // distfromIntersect is distance between pt1 and pt2
        Vector<double> tmp = (pt1 - pt2);
        this.distFromIntersect = new Vector3d(tmp.ToArray()[0], tmp.ToArray()[1], tmp.ToArray()[1]).magnitude;
    }*/
}

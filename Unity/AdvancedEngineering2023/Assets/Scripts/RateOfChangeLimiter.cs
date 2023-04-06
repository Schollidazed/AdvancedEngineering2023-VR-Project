using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RateOfChangeLimiter : MonoBehaviour
{
    private double _maxRate; // maximum rate of change allowed
    private double _maxThreshold; //maximum change allowed.
    private double _prevValue; // previous value of the coordinate
    private DateTime _prevTime; // previous time when the value was updated

    public RateOfChangeLimiter(double maxRate, double maxThreshold)
    {
        _maxRate = maxRate;
        _maxThreshold = maxThreshold;
        _prevValue = 0;
        _prevTime = DateTime.Now;
    }

    public double calculate(double newValue)
    {
        var now = DateTime.Now;
        var elapsed = (now - _prevTime).TotalSeconds;
        var maxChange = elapsed * _maxRate;

        var delta = newValue - _prevValue;
        var limitedDelta = Math.Min(Math.Abs(delta), maxChange) * Math.Sign(delta);

        var limitedValue = _prevValue + limitedDelta;

        _prevValue = limitedValue;
        _prevTime = now;

        return limitedValue;
    }

    public double limit(double newValue)
    {
        if (_prevValue == 0) _prevValue = newValue;

        double delta = Math.Abs(newValue - _prevValue);
        Debug.Log("Delta: " + delta + " New Value: " + newValue + " Previous Value: " + _prevValue);
        
        double r = (delta >= _maxThreshold) ? _prevValue : newValue;
        _prevValue = r;
        return r;
    }
}

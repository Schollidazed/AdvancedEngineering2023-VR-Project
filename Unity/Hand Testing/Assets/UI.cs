using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TMPro;
using System;

public class UI : MonoBehaviour
{
    public TMP_Text scoreText;
    public int score;

    void Start()
    {
        score = 0;
    }
    void Update()
    {
        scoreText.text = "Score: " + score;
    }
    public void ResetScore()
    {
        score = 0;
    }
}

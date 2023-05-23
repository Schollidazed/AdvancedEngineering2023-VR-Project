using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TMPro;

public class Leaderboard : MonoBehaviour
{
    public GameObject Scoreboard;
    public TMP_Text text;

    public int HighScore = 0;

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        text.text = "High Score: " + HighScore;
    }

    public void UpdateScore()
    {
        if (Scoreboard.GetComponent<UI>().score > HighScore)
            HighScore = Scoreboard.GetComponent<UI>().score;
    }
}

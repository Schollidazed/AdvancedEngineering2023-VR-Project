using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class activateGameWhenShot : MonoBehaviour
{
    public GameObject target;

    public UnityEvent gameStart = new UnityEvent();

    // Start is called before the first frame update
    void Start()
    {

    }

    // Update is called once per frame
    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == ("Bullet(Clone)"))
        {
            Debug.Log("Start the Game!!!");
            target.SetActive(false);
            Destroy(col.gameObject);

            //Activate the random position of the targets.
            //Start the Timer.
            gameStart.Invoke();
        }
    }

    public void ResetGame()
    {
        target.SetActive(true);
    }
}

using System.Collections;
using System.Collections.Generic;
using UnityEngine;


public class disappearWhenShot : MonoBehaviour
{
    public GameObject UI;

    public bool hasBeenShot;
    public bool read;

    // Start is called before the first frame update
    void Start()
    {

    }

    void Update()
    {

    }

    // Update is called once per frame
    void OnCollisionEnter(Collision col)
    {
        Debug.Log(col.gameObject.name);
        if (col.gameObject.name == ("Bullet(Clone)"))
        {
            Debug.Log("DESTROY");
            Destroy(col.gameObject);
            hasBeenShot = true;
        }
    }

    public bool isShot()
    {
        return hasBeenShot;
    }
}

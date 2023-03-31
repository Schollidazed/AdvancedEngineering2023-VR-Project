using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class normalObject : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {

    }

    void OnCollisionEnter(Collision col)
    {
        Debug.Log(col.gameObject.name);
        if (col.gameObject.name == ("Bullet(Clone)"))
        {
            Destroy(col.gameObject);
        }
    }
}

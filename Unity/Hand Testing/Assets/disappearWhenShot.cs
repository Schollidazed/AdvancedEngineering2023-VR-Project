using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class disappearWhenShot : MonoBehaviour
{
    public GameObject target;
    // Start is called before the first frame update
    void Start(){}

    // Update is called once per frame
    void OnCollisionEnter(Collision col)
    {
        Debug.Log(col.gameObject.name);
        if (col.gameObject.name == ("Bullet(Clone)"))
        {
            Debug.Log("DESTROY");
            Destroy(target);
            Destroy(col.gameObject);
        }
    }
}

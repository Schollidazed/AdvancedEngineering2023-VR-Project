using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR.Interaction.Toolkit;
using UnityEngine.InputSystem;

public class Blaster : MonoBehaviour
{
    public GameObject reference;
    private Transform target;

    public GameObject bullet;
    public Transform spawnPoint;
    public float fireSpeed = 20;


    public InputActionProperty blasterActivate;

    private float timeElapsed = 0.0F;
    private bool hasFired = false;

    //private Vector3 position;
    //private Vector3 rotation;

    // Start is called before the first frame update
    void Start()
    {
        target = GetComponent<Transform>();
    }
    
    // Update is called once per frame
    void Update()
    {
        target.position = reference.transform.position;
        target.rotation = reference.transform.rotation;

        if ((blasterActivate.action.ReadValue<float>() >= 1F) && !hasFired)
        {
            //Debug.Log("Ding!"); 

            GameObject spawnedBullet = Instantiate(bullet);
            spawnedBullet.transform.position = spawnPoint.position;
            spawnedBullet.transform.rotation = spawnPoint.rotation;
            spawnedBullet.GetComponent<Rigidbody>().velocity = spawnPoint.forward * fireSpeed;
            Destroy(spawnedBullet, 3);

            hasFired = true;
        }

        if(hasFired && blasterActivate.action.ReadValue<float>() == 0){
            hasFired = false;
           // Debug.Log("AGHHHHHHHH");
        }
    }
}
    
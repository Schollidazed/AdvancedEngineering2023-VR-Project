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

    private float totTimeElapsed = 0;


    public InputActionProperty blasterActivate;
    private bool fire = false;
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

        //(blasterActivate.action.ReadValue<float>() >= 1F

        if (fire && !hasFired)
        {
            //Debug.Log("Ding!"); 
            totTimeElapsed = 0;

            GameObject spawnedBullet = Instantiate(bullet);
            spawnedBullet.transform.position = spawnPoint.position;
            spawnedBullet.transform.rotation = spawnPoint.rotation;
            spawnedBullet.GetComponent<Rigidbody>().velocity = spawnPoint.forward * fireSpeed;
            Destroy(spawnedBullet, 3);

            hasFired = true;
            fire = false;
        }

        if(hasFired){
            hasFired = false;
           // Debug.Log("AGHHHHHHHH");
        }
    }

    void OnMessageArrived(string message)
    {
        Debug.Log(message);
        if(message == "Fire!")
        {
            fire = true;
        }
    }
}
    
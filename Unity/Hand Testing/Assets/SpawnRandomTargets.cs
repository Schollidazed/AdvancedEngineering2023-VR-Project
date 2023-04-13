using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SpawnRandomTargets : MonoBehaviour
{
    public bool startedSpawning = false;
    public bool objectSpawned = false;
    public GameObject targetPrefab;
    GameObject target;

    public GameObject ScoreBoard;

    public Transform origin;
    public Vector3 newpos;

    public float xRange;
    public float yRange;
    public float zRange;

    // Start is called before the first frame update
    void Start()
    {
        origin = GetComponentInParent<Transform>();
    }

    // Update is called once per frame
    void Update()
    {
        if (startedSpawning && !objectSpawned)
        {
            newpos = new Vector3(Random.value * xRange, Random.value * yRange, Random.value * zRange);
            Instantiate(targetPrefab, origin);

            target = GameObject.Find("Target(Clone)");
            if(target != null) target.transform.localPosition = newpos;

            objectSpawned = true;
        }
        if (startedSpawning && target == null)
        {
            target = GameObject.Find("Target(Clone)");
            if (target != null) target.transform.localPosition = newpos;
        }
        else if (startedSpawning && target.GetComponent<disappearWhenShot>().hasBeenShot == true)
        {
            Destroy(target);
            objectSpawned = false;
            ScoreBoard.GetComponent<UI>().score += 1;
        }
    }

    public void startSpawing()
    {
        startedSpawning = true;
    }

    public void stopSpawing()
    {
        startedSpawning = false;
        if (target != null)
        {
            Destroy(target);
            objectSpawned = false;
        }
    }
}

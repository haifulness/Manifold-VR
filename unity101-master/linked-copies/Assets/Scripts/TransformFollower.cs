using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TransformFollower : MonoBehaviour {
    public Vector3 translation;
    public Vector3 scale;
    public Quaternion rotation;
    public GameObject leader;

    // Use this for initialization
    void Start () {
	}
	
	// Update is called once per frame
	void Update () {
        transform.position = rotation * Vector3.Scale(leader.transform.position,scale) + translation;
        transform.localScale = Vector3.Scale(leader.transform.lossyScale,scale);
        transform.rotation = rotation * leader.transform.rotation;
    }
}

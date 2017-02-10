using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TouchController : MonoBehaviour {

	public OVRInput.Controller Controller;
	
	// Update is called once per frame
	void Update () {
		transform.localPosition = OVRInput.GetLocalControllerPosition (Controller);
		transform.localRotation = OVRInput.GetLocalControllerRotation (Controller);
	}

	void OnTriggerEnter(Collider other)
	{
		if (other.gameObject.CompareTag("Player"))
		{
			other.gameObject.SetActive(false);
		}
	}
}

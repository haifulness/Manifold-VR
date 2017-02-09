using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class PlayerController : MonoBehaviour {
    private Rigidbody rb;
    public float speed;
    private int boxesCollected;
    public Text countText;
    public Text winText;

    private void Start()
    {
        rb = GetComponent<Rigidbody>();
        boxesCollected = 0;
        updateCountText();
        winText.text = "";
    }

    // Just before rendering a frame
    void Update()
    {
        
    }

    // Just before physics calcs
    void FixedUpdate()
    {
        float moveHorizontal = Input.GetAxis("Horizontal");
        float moveVertical = Input.GetAxis("Vertical");
        Vector3 movement = new Vector3(moveHorizontal, 0, moveVertical);
        rb.AddForce(movement*speed);
    }

    
    void OnTriggerEnter(Collider other)
    {
        if (other.gameObject.CompareTag("Collectible"))
        {
            other.gameObject.SetActive(false);
            boxesCollected++;
            updateCountText();
        }
    }

    void updateCountText()
    {
        countText.text = "Collected: " + boxesCollected.ToString();
        if (boxesCollected >= 15)
        {
            winText.text = "You win!";
        }
    }
}

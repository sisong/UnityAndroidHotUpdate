using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Move : MonoBehaviour {

	// Use this for initialization
	void Start () {
		
	}

    float TranslateSpeedTime =1.5f;
    // Update is called once per frame
    void Update(){
        TranslateSpeedTime += Time.deltaTime;
        transform.Translate(Vector3.forward * Time.deltaTime*5);
        if (TranslateSpeedTime > 3.0f){
            transform.Rotate(0, 180, 0);
            TranslateSpeedTime = 0f;
        }
    }
}

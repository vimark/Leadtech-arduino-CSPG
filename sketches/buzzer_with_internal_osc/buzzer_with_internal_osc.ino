const int buzzer = 4; //buzzer to arduino pin 4


void setup(){
 
  pinMode(buzzer, OUTPUT); // Set buzzer - pin 9 as an output

}

void loop(){
 
  digitalWrite(buzzer, HIGH);
  delay(1000);        // ...for 1 sec
  digitalWrite(buzzer, LOW);
  delay(1000);        // ...for 1sec
  
}



void setup(){
  pinMode(13,INPUT_PULLUP);
  Serial.begin(9600);
}

void loop(){
  Serial.println(digitalRead(13));
  delay(150);
}
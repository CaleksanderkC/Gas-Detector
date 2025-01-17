
void setup() {
  Serial.begin(9600);
}


void loop() {

  int sensorValue1 = analogRead(A1);
  int sensorValue2 = analogRead(A2);

  Serial.print(sensorValue1);
  Serial.print(" - ");
  Serial.print(sensorValue2);
  Serial.print("\n");
  delay(100);
}

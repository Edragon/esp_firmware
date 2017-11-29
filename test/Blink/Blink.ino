
void setup() {
  // IO14
  pinMode(6, OUTPUT);
}


void loop() {
  digitalWrite(6, HIGH);   
  delay(1000);                      
  digitalWrite(6, LOW);    
  delay(1000);                      
}

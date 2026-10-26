const int ledPins[] = {8, 9, 10, 11};

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "LED1_ON") digitalWrite(8, HIGH);
    if (cmd == "LED1_OFF") digitalWrite(8, LOW);

    if (cmd == "LED2_ON") digitalWrite(9, HIGH);
    if (cmd == "LED2_OFF") digitalWrite(9, LOW);

    if (cmd == "LED3_ON") digitalWrite(10, HIGH);
    if (cmd == "LED3_OFF") digitalWrite(10, LOW);

    if (cmd == "LED4_ON") digitalWrite(11, HIGH);
    if (cmd == "LED4_OFF") digitalWrite(11, LOW);
  }
}
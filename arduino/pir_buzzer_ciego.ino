/*
 * ============================================================
 *  Proyecto: Alerta para Personas Ciegas - Sensor PIR + Buzzer
 *  Hardware: Arduino Uno + Sensor PIR HC-SR501 + Buzzer Pasivo
 *  Descripción: Suena al instante al detectar calor y para
 *               al instante cuando deja de detectar.
 * ============================================================
 */

const int PIR_PIN    = 2;
const int BUZZER_PIN = 9;
const int LED_PIN    = 13;

void setup() {
  pinMode(PIR_PIN,    INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  Serial.begin(9600);
  Serial.println("Calentando sensor PIR...");

  for (int i = 0; i < 30; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }

  Serial.println("Listo.");
  tone(BUZZER_PIN, 1000, 150); delay(200);
  tone(BUZZER_PIN, 2000, 200); delay(250);
}

void loop() {
  if (digitalRead(PIR_PIN) == HIGH) {
    tone(BUZZER_PIN, 2000);
    digitalWrite(LED_PIN, HIGH);
  } else {
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, LOW);
  }
}

/*
 * ============================================================
 *  Proyecto: Alerta para Personas Ciegas
 *  Hardware: Arduino Nano + Sensor IR-08H + Buzzer Pasivo + LED
 *
 *  El sensor IR-08H detecta obstáculos y pone OUT en LOW.
 *  El rango se ajusta con el potenciómetro del módulo.
 * ============================================================
 */

const int IR_PIN     = 2;   // OUT del IR-08H
const int EN_PIN     = 3;   // EN  del IR-08H (LOW = activo)
const int BUZZER_PIN = 9;   // Buzzer pasivo (pin PWM)
const int LED_PIN    = 13;  // LED integrado del Nano

unsigned long ultimoPitido = 0;
unsigned long ultimoLED    = 0;
bool ledEstado = false;

// ─────────────────────────────────────────────────────────────
void setup() {
  pinMode(IR_PIN,     INPUT);
  pinMode(EN_PIN,     OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  digitalWrite(EN_PIN, LOW);  // activar sensor

  Serial.begin(9600);
  Serial.println("Sistema listo.");

  jingleEmpresa();
  digitalWrite(LED_PIN, LOW);
}

// ─────────────────────────────────────────────────────────────
void loop() {
  unsigned long ahora = millis();

  // IR-08H: LOW = obstáculo detectado
  bool detecta = (digitalRead(IR_PIN) == LOW);

  if (detecta) {
    Serial.println("Obstaculo detectado!");
    pitido(ahora);
  } else {
    noTone(BUZZER_PIN);
    // LED parpadea suave cuando no hay obstáculo (sistema activo)
    if (ahora - ultimoLED >= 600) {
      ultimoLED = ahora;
      ledEstado = !ledEstado;
      digitalWrite(LED_PIN, ledEstado ? HIGH : LOW);
    }
  }
}

// ─────────────────────────────────────────────────────────────
// Pitido din-don cada 400 ms mientras haya obstáculo
// ─────────────────────────────────────────────────────────────
void pitido(unsigned long ahora) {
  if (ahora - ultimoPitido < 400) return;
  ultimoPitido = ahora;

  tone(BUZZER_PIN, 520, 80);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  tone(BUZZER_PIN, 440, 80);
  delay(100);
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
}

// ─────────────────────────────────────────────────────────────
// Jingle de inicio al encender el Nano
// ─────────────────────────────────────────────────────────────
void jingleEmpresa() {
  tone(BUZZER_PIN, 880, 100); delay(150);
  tone(BUZZER_PIN, 784, 100); delay(150);
  tone(BUZZER_PIN, 698, 100); delay(150);
  tone(BUZZER_PIN, 523, 400); delay(450);
  noTone(BUZZER_PIN);
}

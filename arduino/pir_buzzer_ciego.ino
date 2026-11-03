/*
 * ============================================================
 *  Proyecto: Alerta para Personas Ciegas
 *  Hardware: Arduino Nano + HC-SR04 + Buzzer Pasivo + LED
 *
 *  Detecta objetos hasta 150 cm. Más cerca = pitidos más rápidos.
 * ============================================================
 */

const int TRIG_PIN   = 7;
const int ECHO_PIN   = 8;
const int BUZZER_PIN = 9;
const int LED_PIN    = 13;

const float DIST_MAX = 150.0;  // cm (1.5 m)

unsigned long ultimoPitido = 0;
unsigned long ultimoLED    = 0;
unsigned long ultimaMedicion = 0;
float distancia = 999.0;
bool  ledEstado = false;

// ─────────────────────────────────────────────────────────────
void setup() {
  pinMode(TRIG_PIN,   OUTPUT);
  pinMode(ECHO_PIN,   INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  Serial.begin(9600);
  Serial.println("Sistema listo.");

  jingleEmpresa();
  digitalWrite(LED_PIN, LOW);
}

// ─────────────────────────────────────────────────────────────
void loop() {
  unsigned long ahora = millis();

  // Medir cada 80 ms
  if (ahora - ultimaMedicion >= 80) {
    ultimaMedicion = ahora;
    distancia = medirDistancia();
    if (distancia <= DIST_MAX) {
      Serial.print("Distancia: ");
      Serial.print(distancia, 1);
      Serial.println(" cm");
    }
  }

  if (distancia <= DIST_MAX) {
    pitido(ahora, distancia);
  } else {
    noTone(BUZZER_PIN);
    // LED parpadea suave: sistema activo, sin obstáculo
    if (ahora - ultimoLED >= 600) {
      ultimoLED = ahora;
      ledEstado = !ledEstado;
      digitalWrite(LED_PIN, ledEstado ? HIGH : LOW);
    }
  }
}

// ─────────────────────────────────────────────────────────────
// Más cerca = pitidos más rápidos
//   0 –  50 cm → cada 300 ms
//  50 – 100 cm → cada 600 ms
// 100 – 150 cm → cada 1000 ms
// ─────────────────────────────────────────────────────────────
void pitido(unsigned long ahora, float dist) {
  unsigned long intervalo = (dist <= 50) ? 300 : (dist <= 100) ? 600 : 1000;
  if (ahora - ultimoPitido < intervalo) return;
  ultimoPitido = ahora;

  tone(BUZZER_PIN, 520, 80);
  digitalWrite(LED_PIN, HIGH); delay(100);
  tone(BUZZER_PIN, 440, 80);
  delay(100);
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
}

// ─────────────────────────────────────────────────────────────
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  return (dur == 0) ? 999.0 : dur / 58.2;
}

// ─────────────────────────────────────────────────────────────
void jingleEmpresa() {
  tone(BUZZER_PIN, 880, 100); delay(150);
  tone(BUZZER_PIN, 784, 100); delay(150);
  tone(BUZZER_PIN, 698, 100); delay(150);
  tone(BUZZER_PIN, 523, 400); delay(450);
  noTone(BUZZER_PIN);
}

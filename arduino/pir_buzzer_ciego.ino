/*
 * ============================================================
 *  Proyecto: Alerta para Personas Ciegas - Sensor PIR + Buzzer
 *  Hardware: Arduino Uno + Sensor PIR HC-SR501 + Buzzer Pasivo
 *  Descripción: Detecta personas y emite patrones sonoros
 *               diferenciados para avisar al usuario ciego.
 * ============================================================
 */

// ── Pines ────────────────────────────────────────────────────
const int PIR_PIN    = 2;   // Salida digital del HC-SR501
const int BUZZER_PIN = 9;   // Buzzer pasivo (PWM necesario)
const int LED_PIN    = 13;  // LED de estado integrado en el Uno

// ── Configuración de tiempos (ms) ────────────────────────────
const unsigned long COOLDOWN_MS      = 3000;  // Pausa entre alertas
const unsigned long DETECTION_HOLD   = 200;   // Anti-rebote del PIR

// ── Estado global ────────────────────────────────────────────
unsigned long lastAlertTime = 0;
bool          personaPresente = false;

// ── Prototipos ───────────────────────────────────────────────
void alertaPersona();
void beep(int frecuencia, int duracion, int pausa = 80);
void silencio();

// ─────────────────────────────────────────────────────────────
void setup() {
  pinMode(PIR_PIN,    INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  Serial.begin(9600);
  Serial.println(F("Sistema iniciado. Calentando sensor PIR..."));

  // El HC-SR501 necesita ~30 segundos de estabilización.
  // Durante ese tiempo parpadeamos el LED para indicar espera.
  for (int i = 0; i < 30; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(F("\nListo. Monitoreando..."));

  // Pitido de confirmación: sistema listo
  beep(1000, 150);
  beep(1500, 150);
  beep(2000, 200);
}

// ─────────────────────────────────────────────────────────────
void loop() {
  int lectura = digitalRead(PIR_PIN);
  unsigned long ahora = millis();

  if (lectura == HIGH) {
    // Confirmamos la detección con un pequeño retardo anti-ruido
    delay(DETECTION_HOLD);
    if (digitalRead(PIR_PIN) == HIGH) {

      if (!personaPresente) {
        personaPresente = true;
        Serial.println(F("¡Persona detectada!"));
      }

      // Solo alertar si pasó el cooldown
      if (ahora - lastAlertTime >= COOLDOWN_MS) {
        lastAlertTime = ahora;
        alertaPersona();
      }

    }
  } else {
    if (personaPresente) {
      personaPresente = false;
      Serial.println(F("Zona despejada."));
      digitalWrite(LED_PIN, LOW);
    }
  }
}

// ─────────────────────────────────────────────────────────────
// Patrón de alerta: tres pitidos cortos + uno largo
// Inspirado en señales táctiles/sonoras universales.
// ─────────────────────────────────────────────────────────────
void alertaPersona() {
  digitalWrite(LED_PIN, HIGH);

  // Tres pitidos cortos a 2 kHz
  beep(2000, 120, 100);
  beep(2000, 120, 100);
  beep(2000, 120, 100);

  delay(150);

  // Pitido largo a tono más grave (confirma presencia)
  beep(1200, 500, 0);

  digitalWrite(LED_PIN, LOW);
}

// ─────────────────────────────────────────────────────────────
// Genera un tono en el buzzer pasivo con tone() y luego pausa.
// frecuencia : Hz  |  duracion : ms  |  pausa : ms post-tono
// ─────────────────────────────────────────────────────────────
void beep(int frecuencia, int duracion, int pausa) {
  tone(BUZZER_PIN, frecuencia, duracion);
  delay(duracion);
  noTone(BUZZER_PIN);
  if (pausa > 0) delay(pausa);
}

void silencio() {
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
}

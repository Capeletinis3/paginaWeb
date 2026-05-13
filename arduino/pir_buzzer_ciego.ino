/*
 * ============================================================
 *  Proyecto: Alerta para Personas Ciegas - v3.0
 *  Hardware: Arduino Uno + PIR HC-SR501 + HC-SR04 + Buzzer
 *
 *  PIR HC-SR501 → detecta presencia: sirena que sube urgencia
 *  HC-SR04      → mide distancia: pitidos (más cerca = más rápido)
 *  Prioridad: HC-SR04 > PIR (lo cercano es más urgente)
 * ============================================================
 */

// ── Pines ────────────────────────────────────────────────────
const int PIR_PIN    = 2;
const int TRIG_PIN   = 7;
const int ECHO_PIN   = 8;
const int BUZZER_PIN = 9;
const int LED_PIN    = 13;

// ── Distancia máxima HC-SR04 ──────────────────────────────────
const float DIST_MAX = 150.0;  // cm (1.5 metros)

// ── Sirena PIR ────────────────────────────────────────────────
const int FREQ_MIN  = 1000;
const int FREQ_MAX  = 2800;
const int FREQ_PASO = 25;
int  freqActual     = FREQ_MIN;
int  freqDir        = 1;

// ── Tiempos (non-blocking) ────────────────────────────────────
unsigned long tiempoDeteccionPIR = 0;
unsigned long ultimoStepSirena   = 0;
unsigned long ultimaMedicion     = 0;
unsigned long ultimoPitido       = 0;

// ── Distancia cacheada ────────────────────────────────────────
float distancia = 999.0;

// ─────────────────────────────────────────────────────────────
void setup() {
  pinMode(PIR_PIN,    INPUT);
  pinMode(TRIG_PIN,   OUTPUT);
  pinMode(ECHO_PIN,   INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  Serial.begin(9600);
  Serial.println(F("=== Alerta PIR + Ultrasonico v3.0 ==="));
  Serial.println(F("Calentando sensor PIR (30s)..."));

  for (int i = 30; i > 0; i--) {
    Serial.print(i); Serial.print(F("s "));
    digitalWrite(LED_PIN, HIGH); delay(250);
    digitalWrite(LED_PIN, LOW);  delay(250);
    digitalWrite(LED_PIN, HIGH); delay(250);
    digitalWrite(LED_PIN, LOW);  delay(250);
  }

  Serial.println(F("\nSistema listo."));
  melodiaInicio();
}

// ─────────────────────────────────────────────────────────────
void loop() {
  unsigned long ahora = millis();

  // Leer PIR
  bool pirDetecta = (digitalRead(PIR_PIN) == HIGH);
  if (pirDetecta && tiempoDeteccionPIR == 0) tiempoDeteccionPIR = ahora;
  if (!pirDetecta) tiempoDeteccionPIR = 0;

  // Medir distancia cada 80ms (no medir continuamente para no saturar)
  if (ahora - ultimaMedicion >= 80) {
    ultimaMedicion = ahora;
    distancia = medirDistancia();

    if (distancia <= DIST_MAX) {
      Serial.print(F("Distancia: "));
      Serial.print(distancia, 1);
      Serial.println(F(" cm"));
    }
  }

  bool sonicDetecta = (distancia <= DIST_MAX);

  // ── Prioridad: HC-SR04 tiene prioridad sobre PIR ──────────
  if (sonicDetecta) {
    pitidoUltrasonico(ahora, distancia);
  } else if (pirDetecta) {
    sirena(ahora);
  } else {
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, LOW);
    freqActual = FREQ_MIN;
    freqDir    = 1;
  }
}

// ─────────────────────────────────────────────────────────────
// Pitidos para HC-SR04: más cerca = pitidos más rápidos
//   0 –  50 cm → pitido cada 100 ms (muy urgente)
//  50 – 100 cm → pitido cada 300 ms
// 100 – 150 cm → pitido cada 600 ms
// ─────────────────────────────────────────────────────────────
void pitidoUltrasonico(unsigned long ahora, float dist) {
  unsigned long intervalo;
  if      (dist <= 50)  intervalo = 100;
  else if (dist <= 100) intervalo = 300;
  else                  intervalo = 600;

  if (ahora - ultimoPitido >= intervalo) {
    ultimoPitido = ahora;
    tone(BUZZER_PIN, 2600, 60);
    digitalWrite(LED_PIN, HIGH);
    delay(60);
    digitalWrite(LED_PIN, LOW);
  }
}

// ─────────────────────────────────────────────────────────────
// Sirena para PIR: barre frecuencia. Más tiempo = más urgente.
// ─────────────────────────────────────────────────────────────
void sirena(unsigned long ahora) {
  unsigned long enAlerta = (tiempoDeteccionPIR > 0) ? ahora - tiempoDeteccionPIR : 0;

  unsigned long intervalo;
  if      (enAlerta < 3000) intervalo = 8;
  else if (enAlerta < 6000) intervalo = 4;
  else                      intervalo = 2;

  if (ahora - ultimoStepSirena < intervalo) return;
  ultimoStepSirena = ahora;

  freqActual += freqDir * FREQ_PASO;
  if (freqActual >= FREQ_MAX) { freqActual = FREQ_MAX; freqDir = -1; }
  if (freqActual <= FREQ_MIN) { freqActual = FREQ_MIN; freqDir =  1; }

  tone(BUZZER_PIN, freqActual);

  int centro = (FREQ_MIN + FREQ_MAX) / 2;
  digitalWrite(LED_PIN, abs(freqActual - centro) > 600 ? HIGH : LOW);
}

// ─────────────────────────────────────────────────────────────
// Mide distancia con HC-SR04. Devuelve cm. 999 = sin respuesta.
// ─────────────────────────────────────────────────────────────
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracion = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms ≈ 5m
  if (duracion == 0) return 999.0;
  return duracion / 58.2;
}

// ─────────────────────────────────────────────────────────────
void melodiaInicio() {
  int notas[]      = { 523, 659, 784, 1047 };
  int duraciones[] = { 120, 120, 120,  300 };
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, notas[i], duraciones[i]);
    delay(duraciones[i] + 60);
  }
  noTone(BUZZER_PIN);
}

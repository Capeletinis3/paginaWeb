/*
 * ============================================================
 *  Proyecto: Alerta para Personas Ciegas - v4.1
 *  Hardware: Arduino Uno + PIR HC-SR501 + HC-SR04 + Buzzer
 * ============================================================
 */

const int PIR_PIN    = 2;
const int TRIG_PIN   = 7;
const int ECHO_PIN   = 8;
const int BUZZER_PIN = 9;
const int LED_PIN    = 13;

const float         DIST_MAX    = 150.0; // cm (1.5 m)
const unsigned long PIR_CONFIRM = 500;   // ms en HIGH para validar señal
const unsigned long PIR_HOLD    = 3000;  // ms que se mantiene la alerta después de que el PIR baje

// Frecuencias (tonos graves, no molestos)
const int FREQ_MIN  = 350;
const int FREQ_MAX  = 650;
const int FREQ_PASO = 8;
int freqActual = FREQ_MIN;
int freqDir    = 1;

// Tiempos
unsigned long pirStartTime   = 0;  // cuando PIR subió a HIGH
unsigned long pirDropTime    = 0;  // cuando PIR bajó a LOW
unsigned long ultimoStep     = 0;
unsigned long ultimaMedicion = 0;
unsigned long ultimoPitido   = 0;
unsigned long ultimoLED      = 0;  // para parpadeo no bloqueante del LED

float distancia  = 999.0;
bool  personaLog = false;
bool  ledEstado  = false;

// ─────────────────────────────────────────────────────────────
void setup() {
  pinMode(PIR_PIN,    INPUT);
  pinMode(TRIG_PIN,   OUTPUT);
  pinMode(ECHO_PIN,   INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  Serial.begin(9600);
  Serial.println(F("=== Alerta PIR + Ultrasonico v4.1 ==="));
  Serial.println(F("Calentando PIR 30 segundos..."));

  // Parpadeo durante calentamiento: 4 blinks por segundo
  for (int i = 30; i > 0; i--) {
    Serial.print(i); Serial.print(F("s "));
    for (int b = 0; b < 4; b++) {
      digitalWrite(LED_PIN, HIGH);
      delay(125);
      digitalWrite(LED_PIN, LOW);
      delay(125);
    }
  }
  Serial.println(F("\nSistema listo."));

  melodiaInicio();
  digitalWrite(LED_PIN, LOW);
}

// ─────────────────────────────────────────────────────────────
void loop() {
  unsigned long ahora = millis();

  // ── 1. PIR con confirmación + hold time ──────────────────
  bool pirRaw = (digitalRead(PIR_PIN) == HIGH);

  if (pirRaw) {
    pirDropTime = 0;                              // sigue en HIGH, reiniciar bajada
    if (pirStartTime == 0) pirStartTime = ahora;
  } else {
    if (pirStartTime > 0 && pirDropTime == 0) {
      pirDropTime = ahora;                        // acaba de bajar
    }
    // Después del hold time, resetear todo
    if (pirDropTime > 0 && (ahora - pirDropTime >= PIR_HOLD)) {
      pirStartTime = 0;
      pirDropTime  = 0;
      personaLog   = false;
    }
  }

  // Confirmado: PIR estuvo HIGH ≥500ms Y todavía no expiró el hold
  bool pirValido = (pirStartTime > 0) && (
    (pirRaw  && (ahora - pirStartTime >= PIR_CONFIRM)) ||
    (!pirRaw && pirDropTime > 0)
  );

  // ── 2. HC-SR04: medir cada 80 ms ─────────────────────────
  if (ahora - ultimaMedicion >= 80) {
    ultimaMedicion = ahora;
    distancia = medirDistancia();
    if (distancia <= DIST_MAX) {
      Serial.print(F("HC-SR04: "));
      Serial.print(distancia, 1);
      Serial.println(F(" cm"));
    }
  }
  bool sonicValido = (distancia <= DIST_MAX);

  // ── 3. Fusión de sensores ─────────────────────────────────
  if (pirValido && sonicValido) {
    if (!personaLog) {
      personaLog = true;
      Serial.println(F(">>> PERSONA CONFIRMADA (PIR + HC-SR04) <<<"));
    }
    alertaMaxima(ahora);

  } else if (pirValido && !sonicValido) {
    sirena(ahora, pirStartTime);

  } else if (sonicValido && !pirValido) {
    pitidoProximidad(ahora, distancia);

  } else {
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, LOW);
    freqActual = FREQ_MIN;
    freqDir    = 1;
    ledEstado  = false;
  }
}

// ─────────────────────────────────────────────────────────────
// Alerta máxima: sirena rápida + LED parpadeando rápido
// ─────────────────────────────────────────────────────────────
void alertaMaxima(unsigned long ahora) {
  // Tono
  if (ahora - ultimoStep >= 6) {
    ultimoStep = ahora;
    freqActual += freqDir * FREQ_PASO;
    if (freqActual >= FREQ_MAX) { freqActual = FREQ_MAX; freqDir = -1; }
    if (freqActual <= FREQ_MIN) { freqActual = FREQ_MIN; freqDir =  1; }
    tone(BUZZER_PIN, freqActual);
  }

  // LED parpadea rápido (cada 120 ms)
  if (ahora - ultimoLED >= 120) {
    ultimoLED = ahora;
    ledEstado = !ledEstado;
    digitalWrite(LED_PIN, ledEstado ? HIGH : LOW);
  }
}

// ─────────────────────────────────────────────────────────────
// Sirena moderada: solo PIR activo
// ─────────────────────────────────────────────────────────────
void sirena(unsigned long ahora, unsigned long inicio) {
  unsigned long enAlerta = ahora - inicio;
  unsigned long intervalo = (enAlerta < 3000) ? 8 : (enAlerta < 6000) ? 4 : 2;

  // Tono
  if (ahora - ultimoStep >= intervalo) {
    ultimoStep = ahora;
    freqActual += freqDir * FREQ_PASO;
    if (freqActual >= FREQ_MAX) { freqActual = FREQ_MAX; freqDir = -1; }
    if (freqActual <= FREQ_MIN) { freqActual = FREQ_MIN; freqDir =  1; }
    tone(BUZZER_PIN, freqActual);
  }

  // LED parpadea lento (cada 300 ms) — bug corregido: umbral basado en rango real
  if (ahora - ultimoLED >= 300) {
    ultimoLED = ahora;
    ledEstado = !ledEstado;
    digitalWrite(LED_PIN, ledEstado ? HIGH : LOW);
  }
}

// ─────────────────────────────────────────────────────────────
// Pitidos din-don: solo HC-SR04, sin calor detectado
// ─────────────────────────────────────────────────────────────
void pitidoProximidad(unsigned long ahora, float dist) {
  unsigned long intervalo = (dist <= 50) ? 300 : (dist <= 100) ? 600 : 1000;

  if (ahora - ultimoPitido < intervalo) return;
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
void melodiaInicio() {
  int notas[]      = { 523, 659, 784, 1047 };
  int duraciones[] = { 120, 120, 120,  300 };
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, notas[i], duraciones[i]);
    delay(duraciones[i] + 60);
  }
  noTone(BUZZER_PIN);
}

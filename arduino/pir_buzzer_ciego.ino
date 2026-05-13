/*
 * ============================================================
 *  Proyecto: Alerta para Personas Ciegas - v4.0
 *  Hardware: Arduino Uno + PIR HC-SR501 + HC-SR04 + Buzzer
 *
 *  Lógica de fusión de sensores:
 *
 *  PIR solo         → calor detectado, sirena suave (puede ser
 *                     animal pequeño, corriente caliente, etc.)
 *  HC-SR04 solo     → objeto cercano sin calor (mueble, pared)
 *                     → pitidos simples, NO es persona
 *  PIR + HC-SR04    → calor Y objeto en rango al mismo tiempo
 *                     → PERSONA CONFIRMADA, alerta máxima
 *
 *  Filtro anti-falsas: el PIR debe mantenerse en HIGH durante
 *  500 ms continuos antes de considerarse válido. Así se
 *  descartan animales pequeños y picos de calor breves.
 * ============================================================
 */

const int PIR_PIN    = 2;
const int TRIG_PIN   = 7;
const int ECHO_PIN   = 8;
const int BUZZER_PIN = 9;
const int LED_PIN    = 13;

const float          DIST_MAX     = 150.0;  // cm (1.5 m)
const unsigned long  PIR_CONFIRM  = 500;    // ms continuos para validar PIR

// Tonos suaves (rango grave, menos molesto)
const int FREQ_MIN  = 350;
const int FREQ_MAX  = 650;
const int FREQ_PASO = 8;
int freqActual      = FREQ_MIN;
int freqDir         = 1;

// Tiempos
unsigned long pirStartTime    = 0;
unsigned long ultimoStep      = 0;
unsigned long ultimaMedicion  = 0;
unsigned long ultimoPitido    = 0;

float distancia   = 999.0;
bool  personaLog  = false;  // para imprimir el mensaje solo una vez

// ─────────────────────────────────────────────────────────────
void setup() {
  pinMode(PIR_PIN,    INPUT);
  pinMode(TRIG_PIN,   OUTPUT);
  pinMode(ECHO_PIN,   INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  Serial.begin(9600);
  Serial.println(F("=== Alerta PIR + Ultrasonico v4.0 ==="));
  Serial.println(F("Calentando sensor PIR (30s)..."));

  for (int i = 30; i > 0; i--) {
    Serial.print(i); Serial.print(F("s "));
    digitalWrite(LED_PIN, HIGH); delay(250);
    digitalWrite(LED_PIN, LOW);  delay(250);
    digitalWrite(LED_PIN, HIGH); delay(250);
    digitalWrite(LED_PIN, LOW);  delay(250);
  }

  Serial.println(F("\nSistema listo. Monitoreando..."));
  melodiaInicio();
}

// ─────────────────────────────────────────────────────────────
void loop() {
  unsigned long ahora = millis();

  // ── 1. Leer PIR con filtro de tiempo ─────────────────────
  bool pirRaw = (digitalRead(PIR_PIN) == HIGH);
  if (pirRaw) {
    if (pirStartTime == 0) pirStartTime = ahora;
  } else {
    pirStartTime = 0;
    personaLog   = false;
  }
  // Solo se considera válido si lleva ≥500 ms continuo en HIGH
  bool pirValido = pirRaw && ((ahora - pirStartTime) >= PIR_CONFIRM);

  // ── 2. Medir HC-SR04 cada 80 ms ──────────────────────────
  if (ahora - ultimaMedicion >= 80) {
    ultimaMedicion = ahora;
    distancia = medirDistancia();
    if (distancia <= DIST_MAX) {
      Serial.print(F("HC-SR04: ")); Serial.print(distancia, 1); Serial.println(F(" cm"));
    }
  }
  bool sonicValido = (distancia <= DIST_MAX);

  // ── 3. Fusión de sensores ─────────────────────────────────
  if (pirValido && sonicValido) {
    // Ambos confirman: PERSONA REAL → alerta máxima
    if (!personaLog) {
      personaLog = true;
      Serial.println(F(">>> PERSONA CONFIRMADA (PIR + HC-SR04) <<<"));
    }
    alertaMaxima(ahora);

  } else if (pirValido && !sonicValido) {
    // Solo calor, fuera del rango ultrasónico: sirena moderada
    sirena(ahora, pirStartTime);

  } else if (sonicValido && !pirRaw) {
    // Objeto sin calor: NO es persona, pitidos informativos
    pitidoProximidad(ahora, distancia);

  } else {
    // Nada → silencio
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, LOW);
    freqActual = FREQ_MIN;
    freqDir    = 1;
  }
}

// ─────────────────────────────────────────────────────────────
// Alerta máxima: sirena a máxima velocidad + LED fijo
// Se activa solo cuando PIR y HC-SR04 coinciden (persona real)
// ─────────────────────────────────────────────────────────────
void alertaMaxima(unsigned long ahora) {
  if (ahora - ultimoStep < 6) return;
  ultimoStep = ahora;

  freqActual += freqDir * FREQ_PASO;
  if (freqActual >= FREQ_MAX) { freqActual = FREQ_MAX; freqDir = -1; }
  if (freqActual <= FREQ_MIN) { freqActual = FREQ_MIN; freqDir =  1; }

  tone(BUZZER_PIN, freqActual);
  digitalWrite(LED_PIN, HIGH);  // LED fijo encendido: máxima urgencia
}

// ─────────────────────────────────────────────────────────────
// Sirena moderada: solo PIR activo (posible persona lejana)
// La velocidad aumenta cuanto más tiempo lleva detectando
// ─────────────────────────────────────────────────────────────
void sirena(unsigned long ahora, unsigned long inicio) {
  unsigned long enAlerta = ahora - inicio;

  unsigned long intervalo;
  if      (enAlerta < 3000) intervalo = 8;
  else if (enAlerta < 6000) intervalo = 4;
  else                      intervalo = 2;

  if (ahora - ultimoStep < intervalo) return;
  ultimoStep = ahora;

  freqActual += freqDir * FREQ_PASO;
  if (freqActual >= FREQ_MAX) { freqActual = FREQ_MAX; freqDir = -1; }
  if (freqActual <= FREQ_MIN) { freqActual = FREQ_MIN; freqDir =  1; }

  tone(BUZZER_PIN, freqActual);
  int centro = (FREQ_MIN + FREQ_MAX) / 2;
  digitalWrite(LED_PIN, abs(freqActual - centro) > 600 ? HIGH : LOW);
}

// ─────────────────────────────────────────────────────────────
// Pitidos de proximidad: solo HC-SR04, sin calor
// Más cerca = pitidos más rápidos
// ─────────────────────────────────────────────────────────────
void pitidoProximidad(unsigned long ahora, float dist) {
  unsigned long intervalo;
  if      (dist <= 50)  intervalo = 300;
  else if (dist <= 100) intervalo = 600;
  else                  intervalo = 1000;

  if (ahora - ultimoPitido < intervalo) return;
  ultimoPitido = ahora;

  // Dos tonos suaves tipo "din-don" en lugar de un pitido agudo
  tone(BUZZER_PIN, 520, 80);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  tone(BUZZER_PIN, 440, 80);
  delay(100);
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
}

// ─────────────────────────────────────────────────────────────
// Devuelve distancia en cm. 999 = sin respuesta (fuera de rango)
// ─────────────────────────────────────────────────────────────
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long dur = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms ≈ 5 m
  if (dur == 0) return 999.0;
  return dur / 58.2;
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

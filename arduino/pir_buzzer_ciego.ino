/*
 * ============================================================
 *  Proyecto: Alerta para Personas Ciegas - v2.0
 *  Hardware: Arduino Uno + Sensor PIR HC-SR501 + Buzzer Pasivo
 *  Descripción: Sirena que sube y baja de frecuencia al detectar
 *               una persona. Cuanto más tiempo pasa, más rápida.
 *               Sin cooldown. Respuesta instantánea.
 * ============================================================
 */

const int PIR_PIN    = 2;
const int BUZZER_PIN = 9;
const int LED_PIN    = 13;

// Estados
enum Estado { LISTO, ALERTA, DESPEJADO };
Estado estadoActual = LISTO;

// Sirena
const int FREQ_MIN  = 1000;
const int FREQ_MAX  = 2800;
const int FREQ_PASO = 25;
int  freqActual     = FREQ_MIN;
int  freqDir        = 1;

// Tiempos (non-blocking, sin delay() en el loop)
unsigned long tiempoDeteccion = 0;
unsigned long ultimoStep      = 0;
unsigned long ultimoDespejado = 0;

void setup() {
  pinMode(PIR_PIN,    INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  Serial.begin(9600);
  Serial.println(F("=== Alerta PIR v2.0 ==="));
  Serial.println(F("Calentando sensor (30s)..."));

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

void loop() {
  unsigned long ahora = millis();
  bool detecta = (digitalRead(PIR_PIN) == HIGH);

  switch (estadoActual) {

    case LISTO:
      if (detecta) {
        estadoActual    = ALERTA;
        tiempoDeteccion = ahora;
        freqActual      = FREQ_MIN;
        freqDir         = 1;
        Serial.print(F("["));
        Serial.print(ahora / 1000);
        Serial.println(F("s] PERSONA DETECTADA"));
      }
      break;

    case ALERTA:
      sirena(ahora);
      if (!detecta) {
        noTone(BUZZER_PIN);
        digitalWrite(LED_PIN, LOW);
        ultimoDespejado = ahora;
        estadoActual    = DESPEJADO;
        Serial.print(F("["));
        Serial.print(ahora / 1000);
        Serial.print(F("s] Despejado. Presencia: "));
        Serial.print((ahora - tiempoDeteccion) / 1000);
        Serial.println(F("s"));
      }
      break;

    case DESPEJADO:
      // Espera 300ms antes de volver a escuchar (evita falsos rebotes al salir)
      if (ahora - ultimoDespejado >= 300) {
        estadoActual = LISTO;
      }
      break;
  }
}

// ── Sirena no bloqueante ──────────────────────────────────────
// Barre entre FREQ_MIN y FREQ_MAX. Cuanto más tiempo hay alguien,
// más rápido sube y baja (máx urgencia tras 8 segundos).
void sirena(unsigned long ahora) {
  unsigned long enAlerta = ahora - tiempoDeteccion;

  // Velocidad del barrido: empieza lento, se acelera con el tiempo
  unsigned long intervalo;
  if      (enAlerta < 3000)  intervalo = 8;
  else if (enAlerta < 6000)  intervalo = 4;
  else                       intervalo = 2;

  if (ahora - ultimoStep < intervalo) return;
  ultimoStep = ahora;

  freqActual += freqDir * FREQ_PASO;

  if (freqActual >= FREQ_MAX) { freqActual = FREQ_MAX; freqDir = -1; }
  if (freqActual <= FREQ_MIN) { freqActual = FREQ_MIN; freqDir =  1; }

  tone(BUZZER_PIN, freqActual);

  // LED encendido en los extremos del barrido (efecto parpadeo)
  int centro = (FREQ_MIN + FREQ_MAX) / 2;
  digitalWrite(LED_PIN, abs(freqActual - centro) > 600 ? HIGH : LOW);
}

// ── Melodía de inicio ─────────────────────────────────────────
void melodiaInicio() {
  int notas[]     = { 523, 659, 784, 1047 };
  int duraciones[] = { 120, 120, 120,  300 };
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, notas[i], duraciones[i]);
    delay(duraciones[i] + 60);
  }
  noTone(BUZZER_PIN);
}

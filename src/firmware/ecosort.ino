/*
  EcoSort - Smart Automated Waste Sorting Bin
  Sensors:
    - Load Cell + HX711 (weight)
    - HC-SR04 Ultrasonic (presence / height proxy)
    - TCS34725 Color Sensor (RGB + Clear)
    - IR Reflective Sensor (digital threshold)
  Outputs:
    - MG996R Servo (seesaw)
    - SSD1306 OLED (status)
    - Optional buzzer

  Tested items:
    - Paper 10g, white, NOT reflective -> TRASH
    - Cardboard 12g, brown, NOT reflective -> TRASH
    - Mint container 22g, green, NOT reflective -> RECYCLE
    - Plastic bottle 27g, light color, REFLECTIVE -> RECYCLE
*/

#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include "HX711.h"
#include <Adafruit_TCS34725.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------------------- PINS --------------------
const int HX_DOUT_PIN = 3;
const int HX_SCK_PIN  = 2;

const int IR_REFLECT_PIN = 4;     // digital output from HW-870/TCRT5000 module

const int SERVO_PIN  = 5;
const int BUZZER_PIN = 6;         // optional

const int US_TRIG_PIN = 8;         // HC-SR04 TRIG
const int US_ECHO_PIN = 9;         // HC-SR04 ECHO

// I2C: SDA/SCL are on A4/A5 on UNO-style boards (UNO R4 uses same header pins)

// -------------------- OLED --------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -------------------- COLOR SENSOR --------------------
Adafruit_TCS34725 tcs(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

// -------------------- SCALE --------------------
HX711 scale;

// IMPORTANT: Set this from your calibration
// Run calibration with a known weight and adjust until grams look right.
float CALIBRATION_FACTOR = 2280.0f;

// -------------------- SERVO --------------------
Servo gateServo;

// Servo angles (adjust to match your mechanism)
const int SERVO_RECYCLE = 0;   // left
const int SERVO_CENTER  = 90;  // neutral
const int SERVO_TRASH   = 180; // right

// -------------------- TIMING --------------------
const unsigned long SETTLE_MS      = 700;   // wait after detecting item
const unsigned long ACTUATE_MS     = 900;   // time to keep tilted so item drops
const unsigned long RETURN_MS      = 300;   // time after returning to center
const unsigned long COOLDOWN_MS    = 800;   // avoid double-trigger

// -------------------- THRESHOLDS (tune if needed) --------------------
// Presence detection thresholds
const float WEIGHT_PRESENT_G   = 5.0f;      // if weight > this, assume item present
const float HEIGHT_PRESENT_CM  = 15.0f;     // if distance drops below this, item present (depends on mounting)
// You will calibrate this based on your empty distance.

// Classification thresholds
const float LIGHT_TRASH_G      = 15.0f;     // < 15g -> TRASH (paper/cardboard)
const float RECYCLE_MAX_G      = 60.0f;     // safe upper bound for these demos

// Green detection for mint container (ratio)
const float GREEN_RATIO_MIN    = 0.30f;

// Optional: “tall object” heuristic (plastic bottle)
const float TALL_OBJECT_CM     = 10.0f;     // if estimated height >= this -> likely bottle (depends on setup)

// -------------------- TYPES --------------------
enum ItemClass {
  CLASS_TRASH,
  CLASS_RECYCLE,
  CLASS_UNKNOWN
};

struct Measurement {
  float weight_g;
  float distance_cm; // ultrasonic distance to object (not true height unless you compute from empty distance)
  bool reflective;
  uint16_t r, g, b, c;
  float rn, gn, bn; // normalized ratios
};

// -------------------- STATE MACHINE --------------------
enum State {
  STATE_IDLE,
  STATE_DETECTED,
  STATE_SETTLE,
  STATE_MEASURE,
  STATE_DECIDE,
  STATE_ACTUATE,
  STATE_RETURN,
  STATE_COOLDOWN
};

State state = STATE_IDLE;
unsigned long stateStartMs = 0;

// Used to convert ultrasonic distance to "height proxy"
float emptyDistanceCm = 0.0f; // measured at startup

// -------------------- HELPERS --------------------
void beep(int ms, int times=1) {
  if (BUZZER_PIN < 0) return;
  for (int i=0; i<times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(ms);
    digitalWrite(BUZZER_PIN, LOW);
    delay(60);
  }
}

void oledPrint(const String &l1, const String &l2="", const String &l3="", const String &l4="") {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(l1);
  display.setCursor(0, 16);
  display.print(l2);
  display.setCursor(0, 32);
  display.print(l3);
  display.setCursor(0, 48);
  display.print(l4);
  display.display();
}

float readUltrasonicCm() {
  // HC-SR04: trigger 10us pulse, measure echo duration
  digitalWrite(US_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(US_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(US_TRIG_PIN, LOW);

  unsigned long duration = pulseIn(US_ECHO_PIN, HIGH, 30000UL); // 30ms timeout (~5m)
  if (duration == 0) return 999.0f; // no echo
  // distance cm ~ duration/58
  return (float)duration / 58.0f;
}

float readWeightGrams(int samples=15) {
  // Average multiple scale readings to reduce noise
  float sum = 0.0f;
  for (int i=0; i<samples; i++) {
    sum += scale.get_units(1);
  }
  return sum / (float)samples;
}

void readColor(Measurement &m, int samples=5) {
  uint32_t r_acc=0, g_acc=0, b_acc=0, c_acc=0;
  for (int i=0; i<samples; i++) {
    uint16_t r,g,b,c;
    tcs.getRawData(&r,&g,&b,&c);
    r_acc += r; g_acc += g; b_acc += b; c_acc += c;
    delay(10);
  }
  m.r = (uint16_t)(r_acc / samples);
  m.g = (uint16_t)(g_acc / samples);
  m.b = (uint16_t)(b_acc / samples);
  m.c = (uint16_t)(c_acc / samples);

  float C = (m.c == 0) ? 1.0f : (float)m.c;
  m.rn = (float)m.r / C;
  m.gn = (float)m.g / C;
  m.bn = (float)m.b / C;
}

bool readReflective() {
  // Most IR modules: LOW or HIGH depending on threshold.
  // If your module behavior is opposite, flip this logic.
  int v = digitalRead(IR_REFLECT_PIN);
  // assume HIGH = reflective detected (common on some boards), adjust if needed
  return (v == HIGH);
}

ItemClass classify(const Measurement &m) {
  // Convert distance to a "height proxy" using emptyDistanceCm if you want:
  // heightProxy = emptyDistance - currentDistance
  float heightProxy = (emptyDistanceCm > 0 && m.distance_cm < 900) ? (emptyDistanceCm - m.distance_cm) : 0.0f;

  // Rule 1: very light -> TRASH (paper/cardboard)
  if (m.weight_g < LIGHT_TRASH_G) {
    return CLASS_TRASH;
  }

  // Rule 2: demo recycle range
  if (m.weight_g >= LIGHT_TRASH_G && m.weight_g <= RECYCLE_MAX_G) {
    // Mint container: green dominant
    if (m.gn > GREEN_RATIO_MIN) {
      return CLASS_RECYCLE;
    }

    // Plastic bottle: reflective
    if (m.reflective) {
      return CLASS_RECYCLE;
    }

    // Optional: tall object -> likely bottle
    if (heightProxy >= TALL_OBJECT_CM) {
      return CLASS_RECYCLE;
    }

    // Otherwise unknown -> TRASH (safe)
    return CLASS_TRASH;
  }

  // Heavy unknown -> TRASH by default
  return CLASS_TRASH;
}

void go(State s) {
  state = s;
  stateStartMs = millis();
}

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);
  delay(400);

  pinMode(IR_REFLECT_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(US_TRIG_PIN, OUTPUT);
  pinMode(US_ECHO_PIN, INPUT);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    // If OLED not found, still continue without it
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    oledPrint("EcoSort Booting...");
  }

  // Servo
  gateServo.attach(SERVO_PIN);
  gateServo.write(SERVO_CENTER);

  // Color sensor
  bool tcs_ok = tcs.begin();
  if (!tcs_ok) {
    Serial.println("ERROR: TCS34725 not found.");
  } else {
    Serial.println("TCS34725 OK");
  }

  // HX711
  scale.begin(HX_DOUT_PIN, HX_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare(); // zero the scale
  Serial.println("Scale tared.");

  // Measure empty ultrasonic distance (baseline)
  // Take multiple samples to reduce noise
  float sum=0.0f;
  int n=10;
  for (int i=0; i<n; i++) {
    float d = readUltrasonicCm();
    sum += d;
    delay(50);
  }
  emptyDistanceCm = sum / n;
  Serial.print("Ultrasonic empty baseline (cm): ");
  Serial.println(emptyDistanceCm);

  oledPrint("EcoSort Ready", "Place Item...");
  beep(80, 2);

  go(STATE_IDLE);
}

// -------------------- LOOP --------------------
void loop() {
  unsigned long now = millis();

  // quick reads for detection
  float w_quick = readWeightGrams(5);
  float d_quick = readUltrasonicCm();

  bool presentByWeight = (w_quick > WEIGHT_PRESENT_G);
  bool presentByUS = (d_quick < HEIGHT_PRESENT_CM); // depends on your mounting

  switch (state) {

    case STATE_IDLE:
      gateServo.write(SERVO_CENTER);
      oledPrint("EcoSort Ready", "Place Item...");
      // detect item
      if (presentByWeight || presentByUS) {
        go(STATE_DETECTED);
      }
      break;

    case STATE_DETECTED:
      oledPrint("Item Detected", "Stabilizing...");
      beep(60, 1);
      go(STATE_SETTLE);
      break;

    case STATE_SETTLE:
      if (now - stateStartMs >= SETTLE_MS) {
        go(STATE_MEASURE);
      }
      break;

    case STATE_MEASURE: {
      Measurement m{};
      m.weight_g = readWeightGrams(15);
      m.distance_cm = readUltrasonicCm();
      m.reflective = readReflective();
      readColor(m);

      // show measurements
      String l2 = "W: " + String(m.weight_g, 1) + "g";
      String l3 = "d: " + String(m.distance_cm, 1) + "cm";
      String l4 = String("G%: ") + String(m.gn, 2) + (m.reflective ? " Rfl:Y" : " Rfl:N");
      oledPrint("Measuring...", l2, l3, l4);

      // log
      Serial.print("W(g)="); Serial.print(m.weight_g, 2);
      Serial.print("  d(cm)="); Serial.print(m.distance_cm, 2);
      Serial.print("  refl="); Serial.print(m.reflective ? "Y" : "N");
      Serial.print("  RGBc="); Serial.print(m.r); Serial.print(",");
      Serial.print(m.g); Serial.print(","); Serial.print(m.b);
      Serial.print(" c="); Serial.print(m.c);
      Serial.print("  rn/gn/bn="); Serial.print(m.rn, 3); Serial.print(",");
      Serial.print(m.gn, 3); Serial.print(","); Serial.println(m.bn, 3);

      // decide immediately
      ItemClass cls = classify(m);

      // decide target angle
      int targetAngle = SERVO_TRASH;
      String result = "TRASH";
      if (cls == CLASS_RECYCLE) {
        targetAngle = SERVO_RECYCLE;
        result = "RECYCLE";
      }

      oledPrint("RESULT:", result,
                String("W: ") + String(m.weight_g, 1) + "g",
                m.reflective ? "Reflective: YES" : "Reflective: NO");

      // actuate
      gateServo.write(targetAngle);
      beep(90, (cls == CLASS_RECYCLE) ? 2 : 1);

      go(STATE_ACTUATE);
      break;
    }

    case STATE_ACTUATE:
      if (now - stateStartMs >= ACTUATE_MS) {
        gateServo.write(SERVO_CENTER);
        go(STATE_RETURN);
      }
      break;

    case STATE_RETURN:
      if (now - stateStartMs >= RETURN_MS) {
        go(STATE_COOLDOWN);
      }
      break;

    case STATE_COOLDOWN:
      // Wait until item removed to avoid re-trigger
      oledPrint("Remove Item", "Resetting...");
      if ((now - stateStartMs >= COOLDOWN_MS) && !presentByWeight && !presentByUS) {
        go(STATE_IDLE);
      }
      break;
  }

  delay(20); // small loop delay to reduce flicker / noise
}


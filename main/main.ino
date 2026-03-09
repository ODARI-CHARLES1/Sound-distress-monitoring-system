#include <DHT.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

#define DHTPIN 15
#define DHTTYPE DHT11
#define SOUND_PIN 32
#define BUZZER 23
#define SIM_RX 8
#define SIM_TX 9

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial sim(SIM_RX, SIM_TX);
LiquidCrystal lcd(22, 21, 4, 5,18,19);

// Thresholds
int soundThreshold = 100;          // Adjust based on sensor
int durationThreshold = 3000;      // Sound duration in ms

// State variables
unsigned long soundStartTime = 0;
bool alertSent = false;
bool envAlertSent = false;

// Buzzer timing
unsigned long buzzerStartTime = 0;   // When unsafe condition occurs
unsigned long buzzerOnTime = 0;      // When buzzer was turned ON
bool buzzerActive = false;           // True when buzzer is ON
const unsigned long buzzerDelay = 5000;    // 10 seconds before buzzer
const unsigned long buzzerDuration = 2000;  // 4 seconds ON

// ------------------ BUZZER FUNCTIONS ------------------
void buzzerOn(int frequency = 1000) {
  tone(BUZZER, frequency);
}

void buzzerOff() {
  noTone(BUZZER);
}

// ------------------ SMS FUNCTION ------------------
void sendSMS(String message) {
  Serial.println("*** SMS SENDING ***");
  Serial.println(message);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sending SMS...");

  sim.println("AT");
  delay(1000);
  sim.println("AT+CMGF=1");
  delay(1000);
  sim.println("AT+CMGS=\"+254XXXXXXXXX\""); // Replace with your number
  delay(1000);
  sim.print(message);
  delay(500);
  sim.write(26); // Ctrl+Z

  lcd.setCursor(0, 1);
  lcd.print("SMS Sent");

  Serial.println("*** SMS SENT ***\n");
  delay(2000);
  lcd.clear();
}

// ------------------ SETUP ------------------
void setup() {
  pinMode(SOUND_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);

  dht.begin();
  sim.begin(9600);
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.print("System Starting");
  Serial.println("System Starting...");
  delay(2000);
  lcd.clear();
}

// ------------------ LOOP ------------------
void loop() {
  int soundLevel = analogRead(SOUND_PIN);
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  unsigned long currentTime = millis();

  // Check for unsafe condition
  bool unsafeCondition = (soundLevel >= soundThreshold) || (temp > 32 || hum < 25);

  // Start buzzer timer when unsafe condition occurs
  if (unsafeCondition && buzzerStartTime == 0) {
    buzzerStartTime = currentTime;
  }

  // Send alerts only once
  if ((soundLevel >= soundThreshold) && !alertSent) {
    sendSMS("High distress noise detected in classroom.");
    alertSent = true;
  }

  if ((temp > 32 || hum < 25) && !envAlertSent) {
    sendSMS("Environmental alert: Temp/Humidity unsafe.");
    envAlertSent = true;
  }

  // Turn buzzer ON after 10s, for 4s only
  if (unsafeCondition && !buzzerActive && (currentTime - buzzerStartTime >= buzzerDelay)) {
    if (soundLevel >= soundThreshold) buzzerOn(1200);
    else buzzerOn(800);
    buzzerActive = true;
    buzzerOnTime = currentTime;  // start 4s timer
  }

  // Turn buzzer OFF after buzzerDuration
  if (buzzerActive && (currentTime - buzzerOnTime >= buzzerDuration)) {
    buzzerOff();
    buzzerActive = false;
  }

  // Reset timers when safe
  if (!unsafeCondition) {
    buzzerOff();
    buzzerActive = false;
    buzzerStartTime = 0;
    alertSent = false;
    envAlertSent = false;
  }

  // -------- SERIAL OUTPUT --------
  Serial.println("----- SYSTEM STATUS -----");
  Serial.print("Temperature : "); Serial.print(temp); Serial.println(" C");
  Serial.print("Humidity    : "); Serial.print(hum); Serial.println(" %");
  Serial.print("Sound Level : "); Serial.println(soundLevel);
  Serial.print("Buzzer      : "); Serial.println(buzzerActive ? "ON" : "OFF");
  Serial.print("Sound Alert : "); Serial.println(alertSent ? "YES" : "NO");
  Serial.print("Env Alert   : "); Serial.println(envAlertSent ? "YES" : "NO");
  Serial.println("-------------------------\n");

  // -------- LCD DISPLAY --------
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp);
  lcd.print("C H:");
  lcd.print(hum);
  lcd.print("% ");

  lcd.setCursor(0, 1);
  lcd.print("S:");
  lcd.print(soundLevel);
  lcd.print(buzzerActive ? " BUZZ:ON " : " BUZZ:OFF");

  delay(500);
}

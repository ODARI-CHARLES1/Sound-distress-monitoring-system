#include <DHT.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>   // For PCF8563 RTC

#define DHTPIN 15
#define DHTTYPE DHT11
#define SOUND_PIN 32
#define BUZZER 23
#define SIM_RX 8
#define SIM_TX 9

DHT dht;
SoftwareSerial sim(SIM_RX, SIM_TX);

// LCD pins (avoiding I2C)
LiquidCrystal lcd(10, 11, 4, 5, 6, 7);

// ------------------ RTC ------------------
RTC_PCF8563 rtc;

// Thresholds
int soundThreshold = 100;          // Adjust based on sensor
int durationThreshold = 3000;      // Sound duration in ms

// State variables
unsigned long soundStartTime = 0;
bool alertSent = false;
bool envAlertSent = false;
bool callMade = false;

// Buzzer timing
unsigned long buzzerStartTime = 0;
unsigned long buzzerOnTime = 0;
bool buzzerActive = false;
const unsigned long buzzerDelay = 5000;    // 5 seconds
const unsigned long buzzerDuration = 2000; // 2 seconds

String caregiverNumber = "+254700000000"; // Replace with actual caregiver number

// ------------------ BUZZER FUNCTIONS ------------------
void buzzerOn(int frequency = 1000) { tone(BUZZER, frequency); }
void buzzerOff() { noTone(BUZZER); }

// ------------------ SIM800L FUNCTIONS ------------------
void sendSMS(String message) {
  sim.println("AT");
  delay(1000);
  sim.println("AT+CMGF=1");
  delay(1000);
  sim.print("AT+CMGS=\""); sim.print(caregiverNumber); sim.println("\"");
  delay(1000);
  sim.print(message);
  delay(500);
  sim.write(26); // Ctrl+Z
  delay(5000);
}

void makeCall() {
  sim.print("ATD"); 
  sim.print(caregiverNumber);
  sim.println(";");
  delay(30000); // Ring for 30 sec
  sim.println("ATH"); // Hang up
}

// ------------------ SETUP ------------------
void setup() {
  pinMode(SOUND_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);

  delay(dht.getMinimumSamplingPeriod());
  dht.setup(DHTPIN);
  sim.begin(9600);
  Serial.begin(9600);
  lcd.begin(16, 2);

  // Initialize RTC
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("RTC not found");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.print("System Starting");
  Serial.println("System Starting...");
  delay(2000);
  lcd.clear();
}

// ------------------ LOOP ------------------
void loop() {
  int soundLevel = analogRead(SOUND_PIN);
  float temp = dht.getTemperature();
  float hum = dht.getHumidity();
  unsigned long currentTime = millis();

  // Read RTC time
  DateTime now = rtc.now();

  // Check for unsafe conditions
  bool soundUnsafe = (soundLevel >= soundThreshold);
  bool envUnsafe = (temp > 32 || hum < 25);
  bool unsafeCondition = soundUnsafe || envUnsafe;

  // Start buzzer timer
  if (unsafeCondition && buzzerStartTime == 0) buzzerStartTime = currentTime;

  // Send SMS & make call for sound
  if (soundUnsafe && !alertSent) {
    String msg = "Alert: High distress noise detected.\n";
    msg += "Time: " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
    sendSMS(msg);
    makeCall();
    alertSent = true;
    callMade = true;
  }

  // Send SMS & make call for environmental unsafe
  if (envUnsafe && !envAlertSent) {
    String msg = "Alert: Temp/Humidity unsafe.\n";
    msg += "Temp: " + String(temp) + "C, Hum: " + String(hum) + "%\n";
    msg += "Time: " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
    sendSMS(msg);
    if (!callMade) { makeCall(); callMade = true; }
    envAlertSent = true;
  }

  // Turn buzzer ON after buzzerDelay
  if (unsafeCondition && !buzzerActive && (currentTime - buzzerStartTime >= buzzerDelay)) {
    if (soundUnsafe) buzzerOn(1200); else buzzerOn(800);
    buzzerActive = true;
    buzzerOnTime = currentTime;
  }

  // Turn buzzer OFF after buzzerDuration
  if (buzzerActive && (currentTime - buzzerOnTime >= buzzerDuration)) {
    buzzerOff();
    buzzerActive = false;
  }

  // Reset when safe
  if (!unsafeCondition) {
    buzzerOff();
    buzzerActive = false;
    buzzerStartTime = 0;
    alertSent = false;
    envAlertSent = false;
    callMade = false;
  }

  // -------- SERIAL OUTPUT --------
  Serial.println("----- STATUS -----");
  Serial.print("Time       : "); Serial.print(now.hour()); Serial.print(":"); Serial.print(now.minute()); Serial.print(":"); Serial.println(now.second());
  Serial.print("Temp       : "); Serial.print(temp); Serial.println("C");
  Serial.print("Humidity   : "); Serial.print(hum); Serial.println("%");
  Serial.print("Sound      : "); Serial.println(soundLevel);
  Serial.print("Buzzer     : "); Serial.println(buzzerActive ? "ON" : "OFF");
  Serial.println("Alert sent : " + String(alertSent || envAlertSent ? "YES" : "NO"));
  Serial.println("-----------------\n");

  // -------- LCD DISPLAY --------
  lcd.setCursor(0,0);
  lcd.print("T:"); lcd.print(temp); lcd.print("C H:"); lcd.print(hum); lcd.print("%");
  lcd.setCursor(0,1);
  lcd.print("S:"); lcd.print(soundLevel);
  lcd.print(buzzerActive ? " BUZZ:ON " : " BUZZ:OFF");

  delay(500);
}
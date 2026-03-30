#include <DHT.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

#define DHTPIN 15
#define DHTTYPE DHT11
#define SOUND_PIN 32
#define LED_PIN 25
#define BUZZER 23
#define SIM_RX 16
#define SIM_TX 17

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial sim(SIM_RX, SIM_TX);
LiquidCrystal lcd(22,21,4,5,18,19);

int soundThreshold = 100;

bool alertSent = false;
bool envAlertSent = false;

unsigned long buzzerStartTime = 0;
unsigned long buzzerOnTime = 0;
bool buzzerActive = false;

unsigned long buzzerDelay = 10000;
unsigned long buzzerDuration = 4000;

unsigned long previousBlink = 0;
bool ledState = false;

String phoneNumber = "+254708837750";


// ---------- BUZZER ----------
void buzzerOn(int freq=1000){
  tone(BUZZER,freq);
}

void buzzerOff(){
  noTone(BUZZER);
}


// ---------- SMS + CALL ----------
void sendSMS(String message){

  Serial.println("Sending SMS...");

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sending SMS...");

  sim.println("AT");
  delay(1000);

  sim.println("AT+CMGF=1");
  delay(1000);

  sim.print("AT+CMGS=\"");
  sim.print(phoneNumber);
  sim.println("\"");

  delay(1000);

  sim.print(message);
  delay(500);

  sim.write(26);

  lcd.setCursor(0,1);
  lcd.print("SMS SENT");

  Serial.println("SMS SENT");

  delay(4000);


  // CALL
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Calling...");

  Serial.println("Calling...");

  sim.print("ATD");
  sim.print(phoneNumber);
  sim.println(";");

  delay(15000);

  sim.println("ATH");

  lcd.clear();
}


// ---------- SETUP ----------
void setup(){

  pinMode(SOUND_PIN,INPUT);
  pinMode(BUZZER,OUTPUT);
  pinMode(LED_PIN,OUTPUT);

  Serial.begin(9600);
  sim.begin(9600);

  dht.begin();

  lcd.begin(16,2);
  lcd.print("System Starting");
  delay(2000);
  lcd.clear();
}


// ---------- LOOP ----------
void loop(){

  int soundLevel = analogRead(SOUND_PIN);

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  unsigned long currentTime = millis();

  bool unsafeCondition = (soundLevel >= soundThreshold) || (temp > 32 || hum < 25);


  // ---------- LED BLINK ----------
  if(unsafeCondition){

    if(currentTime - previousBlink >= 500){

      previousBlink = currentTime;

      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }

  }else{

    digitalWrite(LED_PIN,LOW);
  }


  // ---------- SOUND ALERT ----------
  if(soundLevel >= soundThreshold && !alertSent){

    sendSMS("ALERT: Distress sound detected. Please check immediately.");

    alertSent = true;
  }


  // ---------- ENV ALERT ----------
  if((temp > 32 || hum < 25) && !envAlertSent){

    sendSMS("ALERT: Environmental conditions unsafe.");

    envAlertSent = true;
  }


  // ---------- BUZZER TIMER ----------
  if(unsafeCondition && buzzerStartTime == 0){
    buzzerStartTime = currentTime;
  }


  // ---------- BUZZER ON ----------
  if(unsafeCondition && !buzzerActive && currentTime - buzzerStartTime >= buzzerDelay){

    if(soundLevel >= soundThreshold)
      buzzerOn(1200);
    else
      buzzerOn(800);

    buzzerActive = true;
    buzzerOnTime = currentTime;
  }


  // ---------- BUZZER OFF ----------
  if(buzzerActive && currentTime - buzzerOnTime >= buzzerDuration){

    buzzerOff();
    buzzerActive = false;
  }


  // ---------- RESET WHEN SAFE ----------
  if(!unsafeCondition){

    buzzerOff();
    buzzerActive = false;
    buzzerStartTime = 0;

    alertSent = false;
    envAlertSent = false;
  }


  // ---------- SERIAL MONITOR ----------
  Serial.println("------ SYSTEM STATUS ------");

  Serial.print("Temp: ");
  Serial.println(temp);

  Serial.print("Humidity: ");
  Serial.println(hum);

  Serial.print("Sound: ");
  Serial.println(soundLevel);

  Serial.println("---------------------------");


  // ---------- LCD ----------
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.print(temp);
  lcd.print("C H:");
  lcd.print(hum);
  lcd.print("% ");

  lcd.setCursor(0,1);
  lcd.print("S:");
  lcd.print(soundLevel);
  lcd.print(" ");

  delay(500);
}
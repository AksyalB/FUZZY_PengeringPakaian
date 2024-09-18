#include <Arduino.h>
#include <Fuzzy.h>
#include "HX711.h"
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <L298N.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

#define DHT_PIN 4
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);
float humidFloat;

//HX711 constructor:
const int LOADCELL_DOUT_PIN = 27;
const int LOADCELL_SCK_PIN = 14;
HX711 scale;

float beratinput;

//nama variabel output
const unsigned int IN1 = 25;
const unsigned int IN2 = 26;
const int enA = 18;
const int motorSpeed = 255;
L298N motor(IN1, IN2);
float waktungering;

Fuzzy *fuzzy = new Fuzzy();

void setup() {
  Serial.begin(9600);

  //input fuzzy kelembapan
FuzzySet *kering_kelembapan = new FuzzySet(0, 0, 50, 60);
FuzzySet *sedang_kelembapan = new FuzzySet(50, 60 ,80, 90);
FuzzySet *lembab_kelembapan = new FuzzySet(80, 90, 100, 100);

//input fuzzy berat
FuzzySet *ringan_berat = new FuzzySet(0, 0, 420, 650);
FuzzySet *sedang_berat = new FuzzySet(430, 700, 700, 970);
FuzzySet *berat_berat = new FuzzySet(750, 970, 1000, 1000);

//output fuzzy kecepatan motor dc
FuzzySet *lambat_kecepatan = new FuzzySet(55, 55, 88.3, 121.6);
FuzzySet *sedang_kecepatan = new FuzzySet( 88.32, 121.4, 188.4, 221.4);
FuzzySet *cepat_kecepatan = new FuzzySet ( 188.4, 221.7, 255, 255);

//output fuzzy waktu pengeringan
FuzzySet *sangatcepat_waktu= new FuzzySet(0, 0, 4, 36);
FuzzySet *cepat_waktu = new FuzzySet(32.25, 50.25, 54.75, 72.75);
FuzzySet *normal_waktu = new FuzzySet(52.5, 72.5, 77.5, 97.5);
FuzzySet *lama_waktu = new FuzzySet(78.4, 104, 111, 137);
FuzzySet *sangatlama_waktu = new FuzzySet(126, 174, 210, 450);

  //fuzzy input kelembapan
  FuzzyInput *kelembapan = new FuzzyInput(1);

  kelembapan->addFuzzySet(kering_kelembapan);
  kelembapan->addFuzzySet(sedang_kelembapan);
  kelembapan->addFuzzySet(lembab_kelembapan);
  fuzzy->addFuzzyInput(kelembapan);

  //fuzzy input berat
  FuzzyInput *berat_baju = new FuzzyInput(2);

  berat_baju->addFuzzySet(ringan_berat);
  berat_baju->addFuzzySet(sedang_berat);
  berat_baju->addFuzzySet(berat_berat);
  fuzzy->addFuzzyInput(berat_baju);


  //fuzzy output kecepatan
  FuzzyOutput *kecepatan_dc = new FuzzyOutput(1);

  kecepatan_dc->addFuzzySet(lambat_kecepatan);
  kecepatan_dc->addFuzzySet(sedang_kecepatan);
  kecepatan_dc->addFuzzySet(cepat_kecepatan);
  fuzzy->addFuzzyOutput(kecepatan_dc);

  //fuzzy output kecepatan
  FuzzyOutput *waktu = new FuzzyOutput(2);

  waktu->addFuzzySet(sangatcepat_waktu);
  waktu->addFuzzySet(cepat_waktu);
  waktu->addFuzzySet(normal_waktu);
  waktu->addFuzzySet(lama_waktu);
  waktu->addFuzzySet(sangatlama_waktu);
  fuzzy->addFuzzyOutput(waktu);


  // fuzzy rule 1
  FuzzyRuleAntecedent *ifKelembapanKeringdanBeratRingan1 = new FuzzyRuleAntecedent();
  ifKelembapanKeringdanBeratRingan1->joinWithAND(kering_kelembapan, ringan_berat);

  FuzzyRuleConsequent *thenkecepatanLambatWaktuSangatcepat1 = new FuzzyRuleConsequent();
  thenkecepatanLambatWaktuSangatcepat1->addOutput(lambat_kecepatan);
  thenkecepatanLambatWaktuSangatcepat1->addOutput(sangatcepat_waktu);

  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, ifKelembapanKeringdanBeratRingan1, thenkecepatanLambatWaktuSangatcepat1);
  fuzzy->addFuzzyRule(fuzzyRule1);

  // fuzzy rule 2
  FuzzyRuleAntecedent *ifKelembapanKeringdanBeratSedang2 = new FuzzyRuleAntecedent();
  ifKelembapanKeringdanBeratSedang2->joinWithAND(kering_kelembapan, sedang_berat);

  FuzzyRuleConsequent *thenkecepatanLambatWaktucepat2 = new FuzzyRuleConsequent();
  thenkecepatanLambatWaktucepat2->addOutput(lambat_kecepatan);
  thenkecepatanLambatWaktucepat2->addOutput(cepat_waktu);

  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, ifKelembapanKeringdanBeratSedang2, thenkecepatanLambatWaktucepat2);
  fuzzy->addFuzzyRule(fuzzyRule2);

  // fuzzy rule 3
  FuzzyRuleAntecedent *ifKelembapanKeringdanBeratBerat3 = new FuzzyRuleAntecedent();
  ifKelembapanKeringdanBeratBerat3->joinWithAND(kering_kelembapan, berat_berat);

  FuzzyRuleConsequent *thenkecepatanSedangWaktuNormal3 = new FuzzyRuleConsequent();
  thenkecepatanSedangWaktuNormal3->addOutput(sedang_kecepatan);
  thenkecepatanSedangWaktuNormal3->addOutput(normal_waktu);

  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, ifKelembapanKeringdanBeratBerat3, thenkecepatanSedangWaktuNormal3);
  fuzzy->addFuzzyRule(fuzzyRule3);

  // fuzzy rule 4
  FuzzyRuleAntecedent *ifKelembapanSedangdanBeratRingan4 = new FuzzyRuleAntecedent();
  ifKelembapanSedangdanBeratRingan4->joinWithAND(sedang_kelembapan, ringan_berat);

  FuzzyRuleConsequent *thenkecepatanLambatWaktuNormal4 = new FuzzyRuleConsequent();
  thenkecepatanLambatWaktuNormal4->addOutput(lambat_kecepatan);
  thenkecepatanLambatWaktuNormal4->addOutput(normal_waktu);

  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, ifKelembapanSedangdanBeratRingan4, thenkecepatanLambatWaktuNormal4);
  fuzzy->addFuzzyRule(fuzzyRule4);

    // fuzzy rule 5
  FuzzyRuleAntecedent *ifKelembapanSedangdanBeratSedang5 = new FuzzyRuleAntecedent();
  ifKelembapanSedangdanBeratSedang5->joinWithAND(sedang_kelembapan, sedang_berat);

  FuzzyRuleConsequent *thenkecepatanSedangWaktuNormal5 = new FuzzyRuleConsequent();
  thenkecepatanSedangWaktuNormal5->addOutput(sedang_kecepatan);
  thenkecepatanSedangWaktuNormal5->addOutput(normal_waktu);

  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, ifKelembapanSedangdanBeratSedang5, thenkecepatanSedangWaktuNormal5);
  fuzzy->addFuzzyRule(fuzzyRule5);

    // fuzzy rule 6
  FuzzyRuleAntecedent *ifKelembapanSedangdanBeratBerat6 = new FuzzyRuleAntecedent();
  ifKelembapanSedangdanBeratBerat6->joinWithAND(sedang_kelembapan, berat_berat);

  FuzzyRuleConsequent *thenkecepatanSedangWaktuLama6 = new FuzzyRuleConsequent();
  thenkecepatanSedangWaktuLama6->addOutput(sedang_kecepatan);
  thenkecepatanSedangWaktuLama6->addOutput(lama_waktu);

  FuzzyRule *fuzzyRule6 = new FuzzyRule(6, ifKelembapanSedangdanBeratBerat6, thenkecepatanSedangWaktuLama6);
  fuzzy->addFuzzyRule(fuzzyRule6);

    // fuzzy rule 7
  FuzzyRuleAntecedent *ifKelembapanLembabdanBeratRingan7 = new FuzzyRuleAntecedent();
  ifKelembapanLembabdanBeratRingan7->joinWithAND(lembab_kelembapan, ringan_berat);

  FuzzyRuleConsequent *thenkecepatanSedangWaktuLama7 = new FuzzyRuleConsequent();
  thenkecepatanSedangWaktuLama7->addOutput(sedang_kecepatan);
  thenkecepatanSedangWaktuLama7->addOutput(lama_waktu);

  FuzzyRule *fuzzyRule7 = new FuzzyRule(7, ifKelembapanLembabdanBeratRingan7, thenkecepatanSedangWaktuLama7);
  fuzzy->addFuzzyRule(fuzzyRule7);

    // fuzzy rule 8
  FuzzyRuleAntecedent *ifKelembapanLembabdanBeratSedang8 = new FuzzyRuleAntecedent();
  ifKelembapanLembabdanBeratSedang8->joinWithAND(lembab_kelembapan, sedang_berat);

  FuzzyRuleConsequent *thenkecepatanCepatWaktuLama8 = new FuzzyRuleConsequent();
  thenkecepatanCepatWaktuLama8->addOutput(cepat_kecepatan);
  thenkecepatanCepatWaktuLama8->addOutput(lama_waktu);

  FuzzyRule *fuzzyRule8 = new FuzzyRule(8, ifKelembapanLembabdanBeratSedang8, thenkecepatanCepatWaktuLama8);
  fuzzy->addFuzzyRule(fuzzyRule8);

  // fuzzy rule 9
  FuzzyRuleAntecedent *ifKelembapanLembabdanBeratBerat9 = new FuzzyRuleAntecedent();
  ifKelembapanLembabdanBeratBerat9->joinWithAND(lembab_kelembapan, berat_berat);

  FuzzyRuleConsequent *thenkecepatanCepatWaktuSangatLama9 = new FuzzyRuleConsequent();
  thenkecepatanCepatWaktuSangatLama9->addOutput(cepat_kecepatan);
  thenkecepatanCepatWaktuSangatLama9->addOutput(sangatlama_waktu);

  FuzzyRule *fuzzyRule9 = new FuzzyRule(9, ifKelembapanLembabdanBeratBerat9, thenkecepatanCepatWaktuSangatLama9);
  fuzzy->addFuzzyRule(fuzzyRule9);

  pinMode(DHT_PIN, INPUT);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  lcd.init();                   
  lcd.backlight();
  scale.set_scale(882.31);
}

void loop() {
humidFloat = dht.readHumidity();
long beratinput = scale.get_units(); 
// beratinput = raw /1000;

//sensor berat masukin

fuzzy->setInput(1, humidFloat);
fuzzy->setInput(2, beratinput);
fuzzy->fuzzify();

float motordc = fuzzy->defuzzify(1);
float waktupeng = fuzzy->defuzzify(2);

//input
Serial.print("Kelembapan:");
Serial.print(humidFloat);
Serial.println(" %");
delay(500);

Serial.print("Berat: ");
Serial.print(beratinput);
Serial.println(" g");
delay(500);

Serial.print("Hasil Output Kecepatan: ");
Serial.print(motordc);
Serial.println(" m/s");
lcd.setCursor(0,0);
lcd.print("kecepatan putaran: ");
lcd.setCursor(0,1);
lcd.println(motordc);
lcd.setCursor(7,1);
lcd.println("m/s");

analogWrite(enA, motordc);
digitalWrite(IN1, LOW);
digitalWrite(IN2, HIGH);


Serial.print("Hasil Output Waktu: ");
Serial.print(waktupeng);
Serial.println(" menit");
lcd.setCursor(0,2);
lcd.print("waktu pengeringan: ");
lcd.setCursor(0,3);
lcd.println(waktupeng);
lcd.setCursor(7,3);
lcd.println("menit");

Serial.println(" ");

delay(500);



}
// http://un7fgo.gengen.ru (C) 2020
// https://github.com/UN7FGO 
// 
// Скетч для "шилда" на Ардуино Уно, генератора на базе модуля SI5351

// Подключаем библиотеку для нашего LCD дистплея, подключенного по I2C протоколу
#include <avr/eeprom.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "si5351.h"
#include <RotaryEncoder.h>          // библиотека для энкодера

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Определяем контакты, к которым у нас подключен энкодер
#define ENC_CLK_PIN 8
#define ENC_DT_PIN  9
#define ENC_SW_PIN  7

RotaryEncoder encoder(ENC_DT_PIN, ENC_CLK_PIN);   


Si5351 si5351;


long int freq = 12345678;
long int fr, k, dfreq, oldfreq, Enc;
long int pressed;
String Ss;
int pos = 3;
int vfo = 1;
int ppos;



void setup() {
  pinMode (ENC_CLK_PIN,INPUT_PULLUP);
  pinMode (ENC_DT_PIN,INPUT_PULLUP);
  pinMode (ENC_SW_PIN,INPUT_PULLUP);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  bool i2c_found;
  i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  if(!i2c_found)
  { Serial.println("Device not found on I2C bus!");}
  
  freq = eeprom_read_dword(0x00); 

}

void loop() {
  if ( freq != oldfreq ) {
    if (vfo == 1) {
        si5351.set_freq(freq, SI5351_CLK0);
    } else {
        si5351.set_freq(freq, SI5351_CLK1);
    }
    oldfreq = freq;
    RefreshDisplay();
  }

  // обрабатываем кнопку энкодера
  if (digitalRead(ENC_SW_PIN) == 0) {
    // запомнаем время нажатия кнопки
    pressed = millis();
    // ждем, пока кнопку отпустят
    while (digitalRead(ENC_SW_PIN) == 0) {
    }
    // считаем время, сколько была нажата кнопка
    pressed = millis() - pressed;
    // если время нажатия больше 1 секунды, то переключаем диапазон
    if ( pressed > 1000 ) {
      if (vfo == 1) {
        vfo = 2;
        eeprom_write_dword(0x00, freq); 
        freq = eeprom_read_dword(0x08); 
      } else {
        vfo = 1;
        eeprom_write_dword(0x08, freq); 
        freq = eeprom_read_dword(0x00); 
      }
      RefreshDisplay();      
    } else {
      // если кнопка былв нажаты менее 1 секунды, меняем шаг перестройки
      // переходим на следующий шаг
      pos--;
      if ( pos < 1)  {
        pos = 8;
      }
      RefreshDisplay();
    }
  }



  // обрабатываем энкодер
  encoder.tick();
  Enc = encoder.getPosition();
  // проверяем, был ли произведен поворот ручки энкодера
  if (Enc != 0){ 
    dfreq = intpow(pos);
    // определяем направление вращения энкодера
    if (Enc < 0) {
       // повернули энкодер "по часовой стрелке" (CW)
       freq += dfreq;
       if (freq > 99999999) { freq = 99999999; }
     } else {
       // повернули энкодер "против часовой стрелки" (CCW)
       freq -= dfreq;
       if (freq < 100000) { freq = 100000; }
     }
     encoder.setPosition(0);
   }

}

long int intpow(int p) {
  long int k = 1;
  for (int j=1; j<p; j++) {
    k = k * 10;
  }
  return k;
}

void RefreshDisplay() {
  display.clearDisplay();
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setCursor(0, 0);     // Start at top-left corner
  if (vfo == 1){ 
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE); // Draw white text
  }
  display.print(F("VFO1"));
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.print(F("  "));
  if (vfo == 2){ 
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE); // Draw white text
  }
  display.print(F("VFO2"));
  
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setCursor(0, 32);     // Start at top-left corner
  Ss = "";
  fr = freq;
  for (int i=8; i>0; i--) {
    k = intpow(i);
    Ss = Ss + String(fr / k);
    fr = fr % k; 
    if (i == 7 || i == 4) {
      Ss = Ss + ".";    
    }
  }
  display.setCursor(0, 24);     // Start at top-left corner
  ppos = 10 - pos;
  if (ppos < 7) { ppos--; }
  if (ppos < 3) { ppos--; }
  for (int i=0; i<=9; i++) {
    if (i == ppos) {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text      
    } else {
      display.setTextColor(SSD1306_WHITE); // Draw white text
    }
    display.print(Ss[i]);
  }
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setCursor(0, 56);     // Start at top-left corner
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.print(F("UN7FGO SI5351 SHIELD"));  
  display.display();
}

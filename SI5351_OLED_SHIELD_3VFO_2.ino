// http://un7fgo.gengen.ru (C) 2020
// https://github.com/UN7FGO 
// 
// Скетч для "шилда" на Ардуино Уно, генератора на базе модуля SI5351

// Подключаем библиотеку для нашего LCD дистплея, подключенного по I2C протоколу
#include <avr/eeprom.h>
#include <SPI.h>
#include <Wire.h>
#include "si5351.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "GyverEncoder.h"          // библиотека для энкодера

Si5351 si5351;

#define SCREEN_WIDTH  128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Определяем контакты, к которым у нас подключен энкодер
#define ENC_CLK_PIN 6
#define ENC_DT_PIN  5
#define ENC_SW_PIN  7

Encoder enc1(ENC_CLK_PIN, ENC_DT_PIN, ENC_NO_BUTTON); 

unsigned long int freq, fr, k, dfreq, oldfreq, Enc;
unsigned long int pressed;
unsigned long int frq[4];
String Ss;
int pos = 3;
int vfo = 1;
int ppos;

void setup() {
  frq[1] = eeprom_read_dword(0x00); 
  frq[2] = eeprom_read_dword(0x08); 
  frq[3] = eeprom_read_dword(0x10); 
  
  pinMode (ENC_CLK_PIN,INPUT_PULLUP);
  pinMode (ENC_DT_PIN,INPUT_PULLUP);
  pinMode (ENC_SW_PIN,INPUT_PULLUP);

  bool i2c_found;
  i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_6PF, 0, 0);

  enc1.setType(TYPE2);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
  }

  freq = eeprom_read_dword(0x10); 
  si5351.set_freq(freq*100, SI5351_CLK2);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);  
  freq = eeprom_read_dword(0x08); 
  si5351.set_freq(freq*100, SI5351_CLK1);
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);  
  freq = eeprom_read_dword(0x00); 
  si5351.set_freq(freq*100, SI5351_CLK0);  
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);  
  oldfreq = 0;
  RefreshDisplay();
  
}

void loop() {
  if ( freq != oldfreq ) {
    delay(20);
    if (vfo==1) {
          si5351.set_freq(freq*100, SI5351_CLK0);
    }
    if (vfo==2) {
          si5351.set_freq(freq*100, SI5351_CLK1);
    }
    if (vfo==3) {
          si5351.set_freq(freq*100, SI5351_CLK2);
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
      switch (vfo) {
        case 1:
            vfo = 2;
            eeprom_write_dword(0x00, freq); 
            freq = eeprom_read_dword(0x08); 
            break;
        case 2:
            vfo = 3;
            eeprom_write_dword(0x08, freq); 
            freq = eeprom_read_dword(0x10); 
            break;
        case 3:
            vfo = 1;
            eeprom_write_dword(0x10, freq); 
            freq = eeprom_read_dword(0x00); 
            break;
        default:
            break;
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
  enc1.tick();
  // проверяем, был ли произведен поворот ручки энкодера
  if (enc1.isTurn()){ 
    dfreq = intpow(pos);
    // определяем направление вращения энкодера
    if (enc1.isRight()) {
       // повернули энкодер "по часовой стрелке" (CW)
       freq += dfreq;
       if (freq > 99999999) { freq = 99999999; }
     } else {
       // повернули энкодер "против часовой стрелки" (CCW)
       freq -= dfreq;
       if (freq < 100000) { freq = 100000; }
     }
     RefreshDisplay();
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
  int ost,yy;
  String Ss;
  frq[vfo] = freq;
  display.clearDisplay();
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setTextSize(2);      
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);    
    Ss = ""; 
    fr = freq;
    for (int i=8; i>0; i--) {
      k = intpow(i);
      ost = fr / k;
      Ss += ost;
      fr = fr % k; 
      if (i == 7 || i == 4) {
        Ss += ".";    
      }
    }
    for (int i=0; i<=9; i++) {
      display.print(Ss[i]);
    }

  yy = 119 - pos*12;  
  if (pos>3) { yy -= 12;}
  if (pos>6) { yy -= 12;}
  display.drawLine(yy, 16, yy+10, 16, SSD1306_WHITE);
  display.drawLine(yy, 17, yy+10, 17, SSD1306_WHITE);
  display.drawLine(yy, 18, yy+10, 18, SSD1306_WHITE);

    
  yy = 24;
  display.setTextSize(1);
  for (int j=1; j<4; j++) {
    if (j!=vfo) {
      Ss = "VFO "; 
      Ss += j;
      Ss += " - ";
      fr = frq[j];
      for (int i=8; i>0; i--) {
        k = intpow(i);
        ost = fr / k;
        Ss += ost;
        fr = fr % k; 
        if (i == 7 || i == 4) {
          Ss += ".";    
        }
      }
      display.setCursor(0, yy);    
      for (int i=0; i<=17; i++) {
        display.print(Ss[i]);
      }
      yy += 16;
    }
  } 
  display.setCursor(0, 57);     // Start at top-left corner
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.print(F(" 3VFO SI5351 SHIELD"));  
  display.display();
}

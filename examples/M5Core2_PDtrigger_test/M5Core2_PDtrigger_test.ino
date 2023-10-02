/***************************************************
 * M5Core2 PD Trigger (CH224K & ACS712xLCTR-05B)
 * 2023/09/10
 * Copyright(c) @tomorrow56 all rights reserved
****************************************************/

#include <M5Core2.h>
//#include "M5StackUpdater.h"

//fot calibration parameter measurement
//#define CALIBRATE

// M5Stack Pin config
// no PSRAM model only 
/*
#define VBUS_I          35
#define VI_I            36
#define CFG1_O          16
#define CFG2_O          17
#define CFG3_O          13
#define VBUSEN_O         2
#define PG_I            12
*/

// M5Stack Core2 Pin config
#define VBUS_I          35
#define VI_I            36
#define CFG1_O          13
#define CFG2_O          14
#define CFG3_O          19
#define VBUSEN_O        32
#define PG_I            27

/***********
CFG Pin Settig
 +----+----+----+-----+
 |CFG1|CFG2|CFG3| OUT |
 +----+----+----+-----+
 |  1 |  - |  - |   5V|
 |  0 |  0 |  0 |   9V|
 |  0 |  0 |  1 |  12V|
 |  0 |  1 |  1 |  15V|
 |  0 |  1 |  0 |  20V|
 +----+----+----+-----+
***********/

/**********
* Calibration parameter
* Changed to match actual measurements with your equipment
**********/
float vScale = 7.83;  // actual VBUS/VBUS_I
float vi_0A = 2.36;  // VI_I(V) @ 0A output
float vi_2A = 2.76;  // VI_I(V) @ 2A output

uint8_t PDO = 0;
bool OE = false;

uint32_t updateTime = 0;       // time for next update
uint8_t interval = 200;  // Update interval

/*
#define CURRENT_100MA  (0x01 << 0)
#define CURRENT_200MA  (0x01 << 1)
#define CURRENT_400MA  (0x01 << 2)
#define CURRENT_800MA  (0x01 << 3)
#define CURRENT_1600MA  (0x01 << 4)
*/

void setup(void) {
  // M5Stack::begin(LCDEnable, SDEnable, SerialEnable, I2CEnable);
  M5.begin(true, true, true, true);
//  M5.Power.begin();
//  M5.Power.setVinMaxCurrent(CURRENT_100MA);

/*
  if(digitalRead(BUTTON_A_PIN) == 0) {
    Serial.println("Will Load menu binary");
    updateFromFS(SD);
    ESP.restart();
  }
*/

  // Pin initialize
  pinMode(VBUS_I, INPUT);
  pinMode(VI_I, INPUT);
  pinMode(PG_I, INPUT);

  pinMode(CFG1_O, OUTPUT);
  pinMode(CFG2_O, OUTPUT);
  pinMode(CFG3_O, OUTPUT);
  pinMode(VBUSEN_O, OUTPUT);

  // Set PD5V
  digitalWrite(CFG1_O, HIGH); 
  digitalWrite(CFG2_O, LOW); 
  digitalWrite(CFG3_O, LOW);
  PDO = 0;
  // Output disable
  digitalWrite(VBUSEN_O, LOW); 
  OE = false;

  // M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(TFT_BLACK);
  drawTitle("M5 PD Trigger");
  drawBtnMenu("DOWN", "ON", "UP");
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  updateTime = millis(); // Next update time
}

void loop() {
  float vbus_v;
  float vbus_i;
  char buf1[5];
  char buf2[5];

  M5.update();

  if (updateTime <= millis()) {
    updateTime = millis() + interval; // Update interval

    vbus_v = readVoltage(analogRead(VBUS_I)) * vScale;  // read data from CH1
    vbus_i = (readVoltage(analogRead(VI_I)) - vi_0A) / ((vi_2A - vi_0A) / 2);    // read data from CH2

    #ifndef CALIBRATE
      dtostrf(vbus_v,4,1,buf1);
      drawText("Vout = " + (String)buf1 + " V    ", 1, 0);
      Serial.printf("Vout = %s V\n", buf1);

      dtostrf(vbus_i,5,2,buf2);
      drawText("Iout = " + (String)buf2 + " A    ", 1, 1);
      Serial.printf("Iout = %s A\n", buf2);
    #else
      drawText("Vout(raw) = " + (String)readVoltage(analogRead(VBUS_I)) + " V    ", 1, 0);
      Serial.println("Vout(raw) = " + (String)readVoltage(analogRead(VBUS_I)) + " V");

      drawText("Iout(raw) = " + (String)readVoltage(analogRead(VI_I)) + " V    ", 1, 1);
      Serial.println("Iout(raw) = " + (String)readVoltage(analogRead(VI_I)) + " V");
    #endif


    switch(PDO){
      case 0:
      digitalWrite(CFG1_O, HIGH); 
      digitalWrite(CFG2_O, LOW); 
      digitalWrite(CFG3_O, LOW);
      drawSubTitle("PDO 5V    ");
      break;
      case 1:
      digitalWrite(CFG1_O, LOW); 
      digitalWrite(CFG2_O, LOW); 
      digitalWrite(CFG3_O, LOW);
      drawSubTitle("PDO 9V    ");
      break;
      case 2:
      digitalWrite(CFG1_O, LOW); 
      digitalWrite(CFG2_O, LOW); 
      digitalWrite(CFG3_O, HIGH);
      drawSubTitle("PDO 12V    ");
      break;
      case 3:
      digitalWrite(CFG1_O, LOW); 
      digitalWrite(CFG2_O, HIGH); 
      digitalWrite(CFG3_O, HIGH);
      drawSubTitle("PDO 15V    ");
      break;
      case 4:
      digitalWrite(CFG1_O, LOW); 
      digitalWrite(CFG2_O, HIGH); 
      digitalWrite(CFG3_O, LOW);
      drawSubTitle("PDO 20V    ");
      break;
      default:
      digitalWrite(CFG1_O, HIGH); 
      digitalWrite(CFG2_O, LOW); 
      digitalWrite(CFG3_O, LOW);
      drawSubTitle("PDO 5V    ");
      break;
    }

    if(OE == true){
      digitalWrite(VBUSEN_O, HIGH);
      Serial.println("Output Enabled");
    }else{
      digitalWrite(VBUSEN_O, LOW); 
      Serial.println("Output Disabled");
    }
    
    drawText("PG", 0, 3);
    if(vbus_v >= 3){
      if(digitalRead(PG_I) == LOW){
        M5.Lcd.fillRect(40, 150, 30, 30, TFT_GREEN);
        Serial.println("PG OK");
      }else{
        M5.Lcd.fillRect(40, 150, 30, 30, TFT_RED);
        Serial.println("PG NG");
      }
    }else{
      M5.Lcd.fillRect(40, 150, 30, 30, TFT_BLACK);
      M5.Lcd.drawRect(40, 150, 30, 30, TFT_WHITE);
    }

  }

  if(M5.BtnA.wasPressed()){
    if(PDO > 0){
      PDO--;
    }
  }

  if(M5.BtnB.wasPressed()){
    if(OE == false){
      OE = true;
      drawBtnMenu("DOWN", "OFF", "UP");
      M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    }else{
      OE = false;
      drawBtnMenu("DOWN", "ON", "UP");
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    }
  }

  if(M5.BtnC.wasPressed()){
    if(PDO < 4){
      PDO++;
    }
  }
}

void drawTitle(String Title){
  M5.Lcd.setTextSize(1);
  M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLUE);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
  M5.Lcd.drawCentreString(Title, 160, 2, 4);
}

void drawSubTitle(String SubTitle){
  M5.Lcd.setTextSize(1);
  M5.Lcd.drawString(SubTitle, 10, 33, 4);
}

void drawText(String Text, int xPos, int yPos){
  M5.Lcd.setTextSize(1);
  M5.Lcd.drawString(Text, xPos * 30, yPos * 30 + 60 + 3, 4);
}

void drawBtnMenu(String A, String B, String C){
  M5.Lcd.setTextSize(1);
  M5.Lcd.fillRect(0, 210, 320, 30, TFT_BLUE);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
  M5.Lcd.drawCentreString(A, 65, 214, 4);
  M5.Lcd.drawCentreString(B, 160, 214, 4);
  M5.Lcd.drawCentreString(C, 255, 214, 4);
}

float readVoltage(uint16_t Vread){
  float Vdc;
  // Convert the read data into voltage
  if(Vread < 5){
    Vdc = 0;
  }else if(Vread <= 1084){
    Vdc = 0.11 + (0.89 / 1084) * Vread;
  }else if(Vread <= 2303){
    Vdc = 1.0 + (1.0 / (2303 - 1084)) * (Vread - 1084);
  }else if(Vread <= 3179){
    Vdc = 2.0 + (0.7 / (3179 - 2303)) * (Vread - 2303);
  }else if(Vread <= 3659){
    Vdc = 2.7 + (0.3 / (3659 - 3179)) * (Vread - 3179);
  }else if(Vread <= 4071){
    Vdc = 3.0 + (0.2 / (4071 - 3659)) * (Vread - 3659);
  }else{
    Vdc = 3.2;
  }
  return Vdc;
}
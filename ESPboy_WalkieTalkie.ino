 /*
ESPboy Walkie-Talkie, by RomanS
using SA868 or SA818 modules
ESPboy project: wwww.espboy.com
*/

#include "lib/User_Setup.h"  //TFT_eSPI setup file for ESPboy
#define USER_SETUP_LOADED

#include <SoftwareSerial.h> //ESP software serial
#include <ESP_EEPROM.h>
#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"

#define MODULE_RX      D6
#define MODULE_TX      D8

#define CARRIER_PIN 14
#define PPT_PIN 15

#define DEFAULT_CHAN 1
#define DEFAULT_VOL 8
#define MAGIC_NO 0x55EE


struct DataStruct{
 uint32_t magicNo;
 uint8_t chan;
 uint8_t vol;
}dataStr;


float LPDchannel[70]={
0, 433.075, 433.1, 433.125, 433.15, 433.175, 433.2, 433.225, 433.25, 433.275, 433.3, 
433.325, 433.35, 433.375, 433.4, 433.425, 433.45, 433.475, 433.5, 433.525, 433.55, 
433.575, 433.6, 433.625, 433.65, 433.675, 433.7, 433.725, 433.75, 433.775, 433.8, 
433.825, 433.85, 433.875, 433.9, 433.925, 433.95, 433.975, 434, 434.025, 434.05, 
434.075, 434.1, 434.125, 434.15, 434.175, 434.2, 434.225, 434.25, 434.275, 434.3, 
434.325, 434.35, 434.375, 434.4, 434.425, 434.45, 434.475, 434.5, 434.525, 434.55, 
434.575, 434.6, 434.625, 434.65, 434.675, 434.7, 434.725, 434.75, 434.775};

enum  TR_STATE {MODULE_TRANSMIT, MODULE_RECEIVE} TRstate;
enum  CR_STATE {CR_YES, CR_NO} CRstate;
bool updateChanFlag = true;
bool updateUIFlag = true; 

ESPboyInit myESPboy;
SoftwareSerial ESerial(MODULE_RX, MODULE_TX);


void drawUI(){
 uint32_t drawColor;
 String messString;

  if (!updateChanFlag){
      drawColor = TFT_YELLOW;
      messString = "";    
    
    if (TRstate == MODULE_TRANSMIT){
      drawColor = TFT_RED;
      messString = F("TRANSMIT");
      }
    if (CRstate == CR_YES){
      drawColor = TFT_GREEN;
      messString = F("CARRIER");
      }
  }
  else{
      drawColor = TFT_LIGHTGREY;
      messString = F("UPDATING");    
  }

  myESPboy.tft.fillScreen(TFT_BLACK);
  
  myESPboy.tft.setTextSize(2);
  myESPboy.tft.setTextColor(drawColor);
  myESPboy.tft.drawString (messString, (128-(messString.length()*6*2))/2, 112);

  messString = "LPD " + (String)dataStr.chan;
  myESPboy.tft.drawString (messString, (128-(messString.length()*6*2))/2, 70);

  messString = String(LPDchannel[dataStr.chan],3);
  myESPboy.tft.setTextSize(3);
  myESPboy.tft.drawString (messString, (128-(messString.length()*6*3))/2, 40);

  myESPboy.tft.setTextSize(1);

  messString = "VOL  ";
  for (uint8_t i=0; i<dataStr.vol; i++) messString += "|";
  myESPboy.tft.drawString (messString, 0, 0);

  if (CRstate == CR_YES){
    messString = "RSSI ";
    uint8_t rssi = map (moduleGetRSSI(),0,130,1,8);
    for (uint8_t i=0; i<rssi; i++) messString += "|";
    myESPboy.tft.drawString (messString, 0, 10);
  }
}


void loadData(){
  EEPROM.get(0, dataStr);
  if (dataStr.magicNo != MAGIC_NO)
   {
    dataStr.magicNo = MAGIC_NO;
    dataStr.chan = DEFAULT_CHAN;
    dataStr.vol = DEFAULT_VOL;
    saveData();
   }
}


void saveData(){
  EEPROM.put(0, dataStr);
  EEPROM.commit();
}


void moduleInit(){
  ESerial.print("AT+DMOCONNECT\r\n");
  delay(300);
};


void moduleSetFreq(float frq){
  String toSend="";
  toSend+="AT+DMOSETGROUP=1,";
  toSend+=String(frq,4);
  toSend+=",";
  toSend+=String(frq,4);
  toSend+=",";
  toSend+="0000,";
  toSend+="3,";
  toSend+="0000";
  toSend+="\r\n";
  ESerial.print(toSend);
  delay(200);
};


void moduleSetVol(uint8_t vol){
  String toSend="";
  if(vol>8) vol=8;
  toSend+="AT+DMOSETVOLUME=";
  toSend+=(String)vol;
  toSend+="\r\n";
  ESerial.print(toSend);
  delay(200);
};


uint16_t moduleGetRSSI(){
  char rssi[20];
  uint8_t cnt=0;
  ESerial.print("AT+RSSI?\r\n");
  delay(100);
  while (ESerial.available()) rssi[cnt++]=ESerial.read();
  rssi[cnt]=0;
  cnt=0;
  for(uint8_t i=0; i<strlen(rssi); i++){
    cnt++;
    if (rssi[i]=='=') break;
  }
  strcpy(&rssi[0], &rssi[cnt]);
  return (atoi(rssi));
}


void setup() {
 myESPboy.begin("Walkie-Talkie");
 EEPROM.begin(sizeof(dataStr));
 Serial.begin(115200); Serial.println();
 ESerial.begin(9600);
 
 myESPboy.mcp.pinMode(PPT_PIN, OUTPUT);
 myESPboy.mcp.digitalWrite(PPT_PIN, HIGH);
 myESPboy.mcp.pinMode(CARRIER_PIN, INPUT); 

 loadData();
 moduleInit();

 if(ESerial.available()) {
  myESPboy.tft.setTextColor(TFT_GREEN);
  myESPboy.tft.drawString(F("Module init OK"), 23,120);
 }
 else {
  myESPboy.tft.setTextColor(TFT_RED);
  myESPboy.tft.drawString(F("Module init FAIL"), 15,120);
  while(1) delay(100);}

 moduleSetVol(dataStr.vol);
 moduleSetFreq(LPDchannel[dataStr.chan]);

 myESPboy.tft.fillScreen(TFT_BLACK);

 TRstate = MODULE_RECEIVE;
 CRstate = CR_NO;
}



void loop() {
  static uint32_t cnt = millis();
  
  if(ESerial.available()){
    while(ESerial.available()) ESerial.read();/*Serial.write(ESerial.read());*/}

  if (!myESPboy.mcp.digitalRead(CARRIER_PIN) && CRstate == CR_NO){
    CRstate = CR_YES;
    updateUIFlag = true;
  };

  if (myESPboy.mcp.digitalRead(CARRIER_PIN) && CRstate == CR_YES){
    CRstate = CR_NO;
    updateUIFlag = true;
  };  

  if ((myESPboy.getKeys()&PAD_LFT || myESPboy.getKeys()&PAD_RGT || myESPboy.getKeys()&PAD_ACT || myESPboy.getKeys()&PAD_ESC) && TRstate == MODULE_RECEIVE){
    TRstate = MODULE_TRANSMIT;
    CRstate = CR_NO;
    myESPboy.mcp.digitalWrite(PPT_PIN, LOW);
    updateUIFlag = true;
  }
    
  if (!(myESPboy.getKeys()&PAD_LFT || myESPboy.getKeys()&PAD_RGT || myESPboy.getKeys()&PAD_ACT || myESPboy.getKeys()&PAD_ESC) && TRstate == MODULE_TRANSMIT){
    TRstate = MODULE_RECEIVE;
    CRstate = CR_NO;
    myESPboy.mcp.digitalWrite(PPT_PIN, HIGH);
    updateUIFlag = true;
  }


  if (myESPboy.getKeys()&PAD_UP) {dataStr.chan++; if (dataStr.chan>69)dataStr.chan=1; updateUIFlag=true; updateChanFlag=true; cnt = millis(); delay(100);}
  if (myESPboy.getKeys()&PAD_DOWN) {dataStr.chan--; if (dataStr.chan<1)dataStr.chan=69; updateUIFlag=true; updateChanFlag=true; cnt = millis(); delay(100);}

  if (myESPboy.getKeys()&PAD_RIGHT && dataStr.vol<8){dataStr.vol++; updateUIFlag=true; delay(100);} 
  if (myESPboy.getKeys()&PAD_LEFT && dataStr.vol>0){dataStr.vol--; updateUIFlag=true; delay(100);} 

  if (updateChanFlag && millis() > cnt+1000){
    updateUIFlag=true;
    updateChanFlag=false;
    saveData();
    moduleSetVol(dataStr.vol);
    moduleSetFreq(LPDchannel[dataStr.chan]);
  }

  if (updateUIFlag){
    updateUIFlag=false;
    drawUI();
  }
  
  delay(100);
}

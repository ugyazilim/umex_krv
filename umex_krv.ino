#include <esp_now.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <time.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// ===== Wi-Fi ve Güvenlik Ayarları =====
const char* ssid     = "Efsane_Karavan_Superbox";
const char* password = "Comologgo.500";
const char* www_user = "admin";
const char* www_pass = "12345";

// ===== TELEGRAM BOT AYARLARI =====
#define BOT_TOKEN "8928848027:AAEvENJNYMt4Du1rKB8LNgGQBeyTNu-mdnE"
#define CHAT_ID   "8890439228"

WiFiClientSecure telegramClient;
UniversalTelegramBot bot(BOT_TOKEN, telegramClient);

unsigned long lastTelegramCheck  = 0;
const unsigned long telegramInterval = 5000;

// ===== BULUT API (yeni.enjoyburgerhouse.com) =====
const char* cloudApiHost   = "yeni.enjoyburgerhouse.com";
const char* cloudApiBase   = "https://yeni.enjoyburgerhouse.com/api/v1";
const char* cloudDeviceKey = "a7f3c9e2b18d4f650e9a1c3d5e7b9f12";
const char* cloudDeviceId  = "karavan-1";

WiFiClientSecure cloudClient;
unsigned long lastCloudPush = 0;
unsigned long lastCloudPoll = 0;
const unsigned long cloudPushInterval = 15000;
const unsigned long cloudPollInterval = 3000;

IPAddress local_IP(192, 168, 1, 50);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

WebServer server(80);

// ===== ALT SOKETLER (SENSÖR VE I2C) =====
#define BATTERY_PIN          1  // ADC1 (Akü Voltajı)
#define MQ2_PIN              2  // ADC1 (Gaz Sensörü)
#define RAIN_PIN             8  // ADC1 (Yağmur Sensörü)
#define DS18B20_OUT_PIN      7  // Dış Ortam
#define DS18B20_IN_PIN       6  // İç Ortam
#define I2C_SCL              5  // Akıllı Terazi SCL
#define I2C_SDA              4  // Akıllı Terazi SDA

// ===== SOL ULN2003 RÖLE GRUBU (7 ÇIKIŞ) =====
#define WEBASTO_POWER_RELAY_PIN  36  // ULN IN1
#define UPS_RELAY_PIN            37  // ULN IN2
#define MAIN_BATTERY_RELAY_PIN   38  // ULN IN3
const int RELAY4 = 39;               // ULN IN4
const int RELAY3 = 40;               // ULN IN5
const int RELAY2 = 41;               // ULN IN6
const int RELAY1 = 42;               // ULN IN7

// ===== SAĞ ULN2003 RÖLE GRUBU (7 ÇIKIŞ) =====
#define WEBASTO_RELAY_PIN        11  // ULN IN1
#define GAS_SIREN_PIN            10  // ULN IN2
#define RAIN_SIREN_PIN           9   // ULN IN3
const int RELAY8 = 18;               // ULN IN4
const int RELAY7 = 17;               // ULN IN5
const int RELAY6 = 16;               // ULN IN6
const int RELAY5 = 15;               // ULN IN7

OneWire oneWireIn(DS18B20_IN_PIN);
DallasTemperature sensorIn(&oneWireIn);

OneWire oneWireOut(DS18B20_OUT_PIN);
DallasTemperature sensorOut(&oneWireOut);

// MPU6050 Ham I2C
#define MPU_ADDR 0x68
bool mpuConnected = false;

bool stateR[20]      = {false};
bool stateMainBat    = false;
bool stateUPS        = false;

// ===== ESP-NOW =====
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

typedef struct cmd_packet    { uint8_t board_id; uint32_t relay_states; bool water_heater_state; } cmd_packet;
typedef struct sensor_packet { uint8_t board_id; float temp_ds18b20; float temp_water; int raw_water; } sensor_packet;
typedef struct btn_packet    { uint8_t board_id; uint8_t btn_id; } btn_packet; 

void toggleRelay(int id);
String buildStatusJson();
bool executeCloudCommand(const String& action, JsonObject params);
void cloudPushStatus();
void cloudPollCommands();
void cloudAckCommand(const String& commandId, bool success, const String& result = "");

// ===================================================
// CACHE SİSTEMİ — TÜM SENSÖRLER
// ===================================================
int    live_rssi2       = -100;
float  live_tempExtra   = -99.0;
int    cache_rssi2      = -100;
float  cache_tempExtra  = -99.0;
unsigned long lastSeen2 = 0;

int    live_rssi3  = -100;
int    cache_rssi3 = -100;
unsigned long lastSeen3 = 0;

int    live_rssi4            = -100;
float  live_tempWater        = -99.0;
int    live_waterLevelPercent = 0;
int    live_waterLiters       = 0;

int    cache_rssi4            = -100;
float  cache_tempWater        = -99.0;
int    cache_waterLevelPercent = 0;
int    cache_waterLiters       = 0;
unsigned long lastSeen4 = 0;

float  live_temperature  = 0.0;   float  cache_temperature  = 0.0;
float  live_tempOutside  = -99.0; float  cache_tempOutside  = -99.0;
float  live_pitch        = 0.0;   float  cache_pitch        = 0.0;
float  live_roll         = 0.0;   float  cache_roll         = 0.0;
int    live_mq2          = 0;     int    cache_mq2          = 0;
int    live_rainPercent  = 0;     int    cache_rainPercent  = 0;
float  live_batteryVoltage = 0.0; float  cache_batteryVoltage = 0.0;
int    live_batteryPercent = 0;   int    cache_batteryPercent = 0;

bool otaActive = false;

const unsigned long slaveTimeout  = 60000;
unsigned long lastSensorReadTime  = 0;
const unsigned long sensorInterval = 2000;
unsigned long lastMasterSync      = 0;

// ===== WEBASTO, BOYLER, FAN VE ALARM BELLEĞİ =====
#define ADDR_TARGET          10
#define ADDR_AUTO_MODE       14
#define ADDR_WATER_TARGET    15
#define ADDR_WATER_AUTO_MODE 19
#define ADDR_GAS_ALARM       20
#define ADDR_RAIN_ALARM      21
#define ADDR_FRIDGE_TARGET   22
#define ADDR_FRIDGE_AUTO_MODE 26
#define RELAY_NAME_LENGTH    24
#define ADDR_RELAY_NAMES     32

float targetTemp        = 24.0;
bool webastoOn          = false; bool webastoAutoMode   = true;
bool stateWebastoPower  = false;
bool isSettingMode      = false; unsigned long lastChangeTime = 0;
unsigned long lastTurnOffTime = 0;
const unsigned long protectionTime = 390000; bool firstRun = true;
unsigned long totalWebastoOnTimeMs  = 0; unsigned long currentCycleStartTime = 0;

float targetWaterTemp   = 40.0;
bool waterHeaterOn      = false; bool waterHeaterAutoMode = true;
float targetFridgeTemp  = 45.0;
bool fridgeAutoMode     = true;  bool fridgeFanOn         = false;
bool gasAlarmEnabled    = true;
bool rainAlarmEnabled   = true;

const char* defaultRelayNames[20] = {
  "İÇ AYDINLATMA","DIŞ AYDINLATMA","HİDROFOR PMP.","TELEVİZYON","BUZDOLABI",
  "220V PRİZLER","USB/ÇAKMAKLIK","EKSTRA YÜK","BOŞ 2","BOŞ 3",
  "BOŞ 4","BOŞ 5","BOŞ 6","BOŞ 7","BOŞ 8","BOŞ 9","BOŞ 10","BOŞ 11","BOŞ 12","BOŞ 13"
};
String relayNames[20];

// ===== YARDIMCI FONKSİYONLAR =====
String getDayName(int weekday) {
  switch(weekday) {
    case 0: return "Pazar";     case 1: return "Pazartesi"; case 2: return "Salı";
    case 3: return "Çarşamba";  case 4: return "Perşembe";  case 5: return "Cuma";
    case 6: return "Cumartesi"; default: return "";
  }
}
String htmlEscape(String s) { s.replace("&","&amp;"); s.replace("<","&lt;"); s.replace(">","&gt;"); s.replace("\"","&quot;"); s.replace("'","&#39;"); return s; }
String jsonEscape(String s) { s.replace("\\","\\\\"); s.replace("\"","\\\""); s.replace("\n"," "); s.replace("\r"," "); return s; }
int calculateLiFePO4Percent(float v) { return constrain((int)round(((v-10.8)/(14.6-10.8))*100.0),0,100); }

bool checkAuth() { if (!server.authenticate(www_user, www_pass)) { server.requestAuthentication(); return false; } return true; }

// DÜZELTME: Webasto tetiği için ULN2003 mantığı (Açmak için HIGH)
void triggerWebasto() { digitalWrite(WEBASTO_RELAY_PIN, HIGH); delay(3500); digitalWrite(WEBASTO_RELAY_PIN, LOW); }

void loadRelayNames() {
  for (int i = 0; i < 20; i++) {
    char buf[RELAY_NAME_LENGTH+1]; int base = ADDR_RELAY_NAMES + i*RELAY_NAME_LENGTH;
    for (int j = 0; j < RELAY_NAME_LENGTH; j++) buf[j] = EEPROM.read(base+j);
    buf[RELAY_NAME_LENGTH] = '\0';
    bool valid = false;
    for (int j = 0; j < RELAY_NAME_LENGTH; j++) { if (buf[j]=='\0') break; if ((byte)buf[j]>=32 && (byte)buf[j]<=254) { valid=true; break; } }
    relayNames[i] = valid ? String(buf) : String(defaultRelayNames[i]);
    relayNames[i].trim();
    if (relayNames[i].length()==0) relayNames[i] = String(defaultRelayNames[i]);
  }
}
void saveRelayName(int idx, String name) {
  if (idx<0||idx>=20) return;
  name.trim();
  if (name.length()==0) name=String(defaultRelayNames[idx]);
  if (name.length()>RELAY_NAME_LENGTH-1) name=name.substring(0,RELAY_NAME_LENGTH-1);
  relayNames[idx]=name; int base=ADDR_RELAY_NAMES+idx*RELAY_NAME_LENGTH;
  for (int j=0;j<RELAY_NAME_LENGTH;j++) EEPROM.write(base+j,(j<(int)name.length())?name[j]:'\0');
  EEPROM.commit();
}

// ===== ESP-NOW SLAVE GÜNCELLEME =====
void updateSlaves() {
  cmd_packet pkt;
  pkt.board_id=2; pkt.relay_states=(stateR[8]<<0)|(stateR[9]<<1)|(stateR[10]<<2)|(stateR[11]<<3)|(fridgeFanOn<<4);
  esp_now_send(broadcastAddress,(uint8_t*)&pkt,sizeof(pkt)); delay(10);
  pkt.board_id=3; pkt.relay_states=(stateR[12]<<0)|(stateR[13]<<1)|(stateR[14]<<2)|(stateR[15]<<3)|(stateR[16]<<4)|(stateR[17]<<5)|(stateR[18]<<6)|(stateR[19]<<7);
  esp_now_send(broadcastAddress,(uint8_t*)&pkt,sizeof(pkt)); delay(10);
  pkt.board_id=4;
  pkt.water_heater_state=waterHeaterOn;
  esp_now_send(broadcastAddress,(uint8_t*)&pkt,sizeof(pkt));
}

// ===== ESP-NOW VERİ ALMA CALLBACK =====
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  int rssi_now = info->rx_ctrl->rssi;
  uint8_t board_id = data[0];

  if (board_id == 2) {
    lastSeen2 = millis(); live_rssi2  = rssi_now; cache_rssi2 = rssi_now;
    if (len == sizeof(sensor_packet)) {
      sensor_packet pkt; memcpy(&pkt, data, sizeof(pkt));
      float t = pkt.temp_ds18b20;
      if (t != 85.0f && t != -127.0f && t > -50.0f && t < 100.0f) { live_tempExtra  = t; cache_tempExtra = t; }
      else { live_tempExtra = -99.0f; }
    }
  }
  else if (board_id == 3) {
    lastSeen3 = millis(); live_rssi3  = rssi_now; cache_rssi3 = rssi_now;
    if (len == sizeof(btn_packet)) {
      btn_packet pkt; memcpy(&pkt, data, sizeof(pkt));
      if (pkt.btn_id >= 13 && pkt.btn_id <= 20) {
        toggleRelay(pkt.btn_id);
      }
    }
  }
  else if (board_id == 4) {
    lastSeen4 = millis(); live_rssi4  = rssi_now; cache_rssi4 = rssi_now;
    if (len == sizeof(sensor_packet)) {
      sensor_packet pkt; memcpy(&pkt, data, sizeof(pkt));
      float tw = pkt.temp_water;
      if (tw != 85.0f && tw != -127.0f && tw > -50.0f && tw < 100.0f) { live_tempWater  = tw; cache_tempWater = tw; }
      else { live_tempWater = -99.0f; }

      int wlp = constrain(map(pkt.raw_water, 0, 4095, 0, 100), 0, 100);
      int wlt = (wlp * 100) / 100;
      live_waterLevelPercent  = wlp; cache_waterLevelPercent = wlp;
      live_waterLiters        = wlt; cache_waterLiters       = wlt;
    }
  }
}

// ===== MANTIK KONTROL FONKSİYONLARI =====
void checkWebastoLogic() {
  if (!stateWebastoPower) return;
  if (!webastoAutoMode) {
    if (webastoOn && cache_temperature >= targetTemp) {
      triggerWebasto();
      webastoOn=false; lastTurnOffTime=millis(); firstRun=false;
      totalWebastoOnTimeMs += (millis()-currentCycleStartTime);
    }
    return;
  }
  
  if (webastoOn && cache_temperature >= targetTemp) {
    triggerWebasto(); webastoOn=false; lastTurnOffTime=millis(); firstRun=false;
    totalWebastoOnTimeMs += (millis()-currentCycleStartTime);
  }
  else if (!webastoOn && cache_temperature <= (targetTemp-2.0f)) {
    unsigned long timeSinceOff = millis()-lastTurnOffTime;
    if (timeSinceOff >= protectionTime || firstRun) { triggerWebasto(); webastoOn=true; currentCycleStartTime=millis(); }
  }
}

void checkWaterHeaterLogic() {
  float tw = (live_tempWater > -50.0f) ? live_tempWater : cache_tempWater;
  if (tw <= -50.0f) { if (waterHeaterOn) { waterHeaterOn=false; updateSlaves(); } return; }
  
  if (!waterHeaterAutoMode) {
    if (waterHeaterOn && tw >= targetWaterTemp) {
      waterHeaterOn=false; updateSlaves();
    }
    return;
  }
  
  if (waterHeaterOn  && tw >= targetWaterTemp)          { waterHeaterOn=false; updateSlaves(); }
  else if (!waterHeaterOn && tw <= (targetWaterTemp-2.0f)) { waterHeaterOn=true;  updateSlaves(); }
}

void checkFridgeLogic() {
  float te = (live_tempExtra > -50.0f) ? live_tempExtra : cache_tempExtra;
  if (te <= -50.0f) { if (fridgeFanOn) { fridgeFanOn=false; updateSlaves(); } return; }
  if (!fridgeAutoMode) return;
  if (!fridgeFanOn && te >= targetFridgeTemp)            { fridgeFanOn=true;  updateSlaves(); }
  else if (fridgeFanOn && te <= (targetFridgeTemp-2.0f)) { fridgeFanOn=false; updateSlaves(); }
}

// ===== TELEGRAM =====
void handleTelegramMessages(int n) {
  for (int i=0;i<n;i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) { bot.sendMessage(chat_id,"Yetkisiz Kullanici!",""); continue; }
    String text=bot.messages[i].text; String from_name=bot.messages[i].from_name;
    if (text=="/start"||text=="/yardim") {
      String w="Merhaba "+from_name+", Ölmezler Dogada Karavan Sistemine Baglandiniz.\n\n";
      w+="Kullanabileceginiz Komutlar:\n";
      w+="🔍 /durum : Tüm sensör ve sistem özetini gönderir.\n⚠️ /tumu_kapat : Tüm röleleri kapatir.\n";
      w+="⚡ /webasto_guc : Webasto ana şalterini açar/kapatır.\n🔥 /webasto_tetik : Webasto durumunu degistirir.\n";
      w+="💧 /boyler_tetik : Su isiticiyi tetikler.\n\n";
      w+="💡 Röleleri acip kapatmak icin /durum yazarak cikan listedeki mavi komutlara tiklayabilirsiniz.";
      bot.sendMessage(chat_id,w,"");
    }
    else if (text=="/durum") {
      String s="  <b>ÖLMEZLER DOĞADA KONTROL PANELİ</b>\n\n";
      s+="         <b>SICAKLIK VE ORTAM</b>\n";
      s+="🔸 İç Ortam: <b>"+String(cache_temperature,1)+" °C</b>\n";
      s+="🔸 Dış Ortam: <b>"+(cache_tempOutside>-50.0f?String(cache_tempOutside,1)+" °C":"HATA")+"</b>\n";
      s+="🔸 Su (Boyler): <b>"+(cache_tempWater>-50.0f?String(cache_tempWater,1)+" °C":"Bekleniyor")+"</b>\n";
      s+="🔸 Buzdolabı Isı: <b>"+(cache_tempExtra>-50.0f?String(cache_tempExtra,1)+" °C":"Bekleniyor")+"</b>\n\n";
      s+="         <b>ISITICI VE SOĞUTUCU AYARLARI</b>\n";
      s+="🔥 Webasto Hedef: <b>"+String(targetTemp,0)+" °C</b> (Oto: "+(webastoAutoMode?"Açık":"Kapalı")+")\n";
      s+="💧 Boyler Hedef: <b>"+String(targetWaterTemp,0)+" °C</b> (Oto: "+(waterHeaterAutoMode?"Açık":"Kapalı")+")\n";
      s+="❄️ Buzdolabı Hedef: <b>"+String(targetFridgeTemp,0)+" °C</b> (Oto: "+(fridgeAutoMode?"Açık":"Kapalı")+")\n\n";
      s+="         <b>DEPO VE ENERJİ</b>\n";
      s+="💧 Temiz Su: <b>%"+String(cache_waterLevelPercent)+"</b> ("+String(cache_waterLiters)+" L)\n";
      s+="⚡ Akü (LiFePO4): <b>"+String(cache_batteryVoltage,1)+" V</b> (%"+String(cache_batteryPercent)+")\n\n";
      s+="         <b>GÜVENLİK SİSTEMİ</b>\n";
      s+="💨 LPG/Duman: <b>"+String(cache_mq2)+"</b>\n";
      s+="🌧 Yağmur: <b>%"+String(cache_rainPercent)+"</b>\n\n";
      s+="         <b>AKILLI TERAZİ</b>\n";
      s+="📐 Ön-Arka: <b>"+String(cache_pitch,1)+"°</b>\n";
      s+="📐 Sağ-Sol: <b>"+String(cache_roll,1)+"°</b>\n\n";
      s+="         <b>AĞ DURUMU</b>\n";
      s+="📡 Kart 1: <b>"+String(WiFi.RSSI())+" dBm</b>\n";
      s+="📡 Kart 2: <b>"+(cache_rssi2<=-100?"Sinyal Yok":String(cache_rssi2)+" dBm")+"</b>\n";
      s+="📡 Kart 3: <b>"+(cache_rssi3<=-100?"Sinyal Yok":String(cache_rssi3)+" dBm")+"</b>\n";
      s+="📡 Kart 4: <b>"+(cache_rssi4<=-100?"Sinyal Yok":String(cache_rssi4)+" dBm")+"</b>\n\n";
      s+="         <b>AÇIK OLAN DONANIMLAR</b>\n"; String aciklar="";
      if(stateMainBat)aciklar+="Ana Şalter, "; if(stateUPS)aciklar+="İnverter, ";
      if(stateWebastoPower)aciklar+="Webasto Ana Güç, "; if(webastoOn)aciklar+="Webasto, ";
      if(waterHeaterOn)aciklar+="Boyler, ";
      for(int j=0;j<20;j++){if(stateR[j])aciklar+=relayNames[j]+", ";}
      if(aciklar=="") aciklar="Açık cihaz bulunmuyor.";
      else aciklar=aciklar.substring(0,aciklar.length()-2);
      s+="🟢 "+aciklar+"\n\n";
      s+="         <b>TEK TIKLA KONTROL MENÜSÜ</b>\n⚡ /webasto_guc | 🔥 /webasto_tetik\n💧 /boyler_tetik | ❄️ /fan_tetik\n\n";
      s+="⚙️ /w_auto (Webasto) | /b_auto (Boyler) | /f_auto (Fan)\n";
      s+="🌡 Webasto: /w_azalt ➖ /w_arttir ➕\n🌡 Boyler: /b_azalt ➖ /b_arttir ➕\n🌡 B.Fan: /f_azalt ➖ /f_arttir ➕\n\n";
      for(int j=0;j<20;j++){s+="/ac_"+String(j+1)+" | /kapat_"+String(j+1)+" 👉 "+relayNames[j]+"\n";}
      s+="\n <b>EKRANI GÜNCELLE:</b> /durum\n";
      bot.sendMessage(chat_id,s,"HTML");
    }
    else if (text.startsWith("/ac_")) {
      int id=text.substring(4).toInt();
      if(id>=1&&id<=20){if(!stateR[id-1])toggleRelay(id); bot.sendMessage(chat_id,"✅ "+relayNames[id-1]+" Açıldı.","");}
      else bot.sendMessage(chat_id,"Hata: Geçersiz numara!","");
    }
    else if (text.startsWith("/kapat_")) {
      int id=text.substring(7).toInt();
      if(id>=1&&id<=20){if(stateR[id-1])toggleRelay(id); bot.sendMessage(chat_id,"❌ "+relayNames[id-1]+" Kapatıldı.","");}
      else bot.sendMessage(chat_id,"Hata: Geçersiz numara!","");
    }
    else if (text=="/tumu_kapat") {
      for(int j=0;j<20;j++) stateR[j]=false;
      // DÜZELTME: Kapatmak için artık ULN2003'e LOW gönderiyoruz
      digitalWrite(RELAY1,LOW); digitalWrite(RELAY2,LOW); digitalWrite(RELAY3,LOW); digitalWrite(RELAY4,LOW);
      digitalWrite(RELAY5,LOW); digitalWrite(RELAY6,LOW); digitalWrite(RELAY7,LOW); digitalWrite(RELAY8,LOW);
      updateSlaves(); bot.sendMessage(chat_id,"⚠️ Tüm röle kanallari kapatildi!","");
    }
    else if (text=="/webasto_guc") {
      if (stateWebastoPower) { 
        if (webastoOn || (!firstRun && (millis() - lastTurnOffTime < protectionTime))) {
          bot.sendMessage(chat_id,"⚠️ HATA: Webasto çalışıyor veya koruma/soğutma fanı aktif!\n\nCihazın yanmasını önlemek için sistem 'Sistem Hazır' konumuna geçmeden ŞALTERİ KAPATAMAZSINIZ.","");
          continue; 
        }
      }
      stateWebastoPower=!stateWebastoPower; 
      digitalWrite(WEBASTO_POWER_RELAY_PIN,stateWebastoPower?HIGH:LOW); // DÜZELTME
      if(!stateWebastoPower){webastoOn=false;digitalWrite(WEBASTO_RELAY_PIN,LOW);} // DÜZELTME
      bot.sendMessage(chat_id,stateWebastoPower?"⚡ Webasto Ana Gücü AÇILDI.":"🔌 Webasto Ana Gücü KESİLDİ.","");
    }
    else if (text=="/webasto_tetik") {
      if(!stateWebastoPower){bot.sendMessage(chat_id,"⚠️ Webasto ana gücü kesik!","");}
      else{triggerWebasto();webastoOn=!webastoOn;if(webastoOn)currentCycleStartTime=millis();else lastTurnOffTime=millis();bot.sendMessage(chat_id,webastoOn?"🔥 Webasto Calistirildi.":"❄️ Webasto Kapatildi.","");}
    }
    else if (text=="/boyler_tetik"){waterHeaterOn=!waterHeaterOn;updateSlaves();bot.sendMessage(chat_id,waterHeaterOn?"💧 Boyler aktif edildi.":"🛑 Boyler kapatildi.","");}
    else if (text=="/fan_tetik")   {fridgeFanOn=!fridgeFanOn;updateSlaves();bot.sendMessage(chat_id,fridgeFanOn?"❄️ Buzdolabı Fanı AKTİF.":"🛑 Buzdolabı Fanı KAPATILDI.","");}
    else if (text=="/f_auto")  {fridgeAutoMode=!fridgeAutoMode;EEPROM.put(ADDR_FRIDGE_AUTO_MODE,fridgeAutoMode?1:0);EEPROM.commit();bot.sendMessage(chat_id,fridgeAutoMode?"⚙️ Fan Oto Mod AÇIK.":"⚙️ Fan Oto Mod KAPALI.","");}
    else if (text=="/f_arttir"){targetFridgeTemp++;if(targetFridgeTemp>50)targetFridgeTemp=50;EEPROM.put(ADDR_FRIDGE_TARGET,targetFridgeTemp);EEPROM.commit();bot.sendMessage(chat_id,"❄️ Fan Hedef: "+String(targetFridgeTemp,0)+" °C","");}
    else if (text=="/f_azalt") {targetFridgeTemp--;if(targetFridgeTemp<40)targetFridgeTemp=40;EEPROM.put(ADDR_FRIDGE_TARGET,targetFridgeTemp);EEPROM.commit();bot.sendMessage(chat_id,"❄️ Fan Hedef: "+String(targetFridgeTemp,0)+" °C","");}
    else if (text=="/w_auto")  {webastoAutoMode=!webastoAutoMode;EEPROM.put(ADDR_AUTO_MODE,webastoAutoMode?1:0);EEPROM.commit();bot.sendMessage(chat_id,webastoAutoMode?"⚙️ Webasto Oto Mod AÇIK.":"⚙️ Webasto Oto Mod KAPALI.","");}
    else if (text=="/b_auto")  {waterHeaterAutoMode=!waterHeaterAutoMode;EEPROM.put(ADDR_WATER_AUTO_MODE,waterHeaterAutoMode?1:0);EEPROM.commit();bot.sendMessage(chat_id,waterHeaterAutoMode?"⚙️ Boyler Oto Mod AÇIK.":"⚙️ Boyler Oto Mod KAPALI.","");}
    else if (text=="/w_arttir"){targetTemp++;if(targetTemp>35)targetTemp=35;EEPROM.put(ADDR_TARGET,targetTemp);EEPROM.commit();bot.sendMessage(chat_id,"🔥 Webasto Hedef: "+String(targetTemp,0)+" °C","");}
    else if (text=="/w_azalt") {targetTemp--;if(targetTemp<1)targetTemp=1;EEPROM.put(ADDR_TARGET,targetTemp);EEPROM.commit();bot.sendMessage(chat_id,"❄️ Webasto Hedef: "+String(targetTemp,0)+" °C","");}
    else if (text=="/b_arttir"){targetWaterTemp++;if(targetWaterTemp>50)targetWaterTemp=50;EEPROM.put(ADDR_WATER_TARGET,targetWaterTemp);EEPROM.commit();bot.sendMessage(chat_id,"🔥 Boyler Hedef: "+String(targetWaterTemp,0)+" °C","");}
    else if (text=="/b_azalt") {targetWaterTemp--;if(targetWaterTemp<20)targetWaterTemp=20;EEPROM.put(ADDR_WATER_TARGET,targetWaterTemp);EEPROM.commit();bot.sendMessage(chat_id,"❄️ Boyler Hedef: "+String(targetWaterTemp,0)+" °C","");}
  }
}

// ===== BULUT API FONKSİYONLARI =====
String buildStatusJson() {
  StaticJsonDocument<5120> doc;

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10)) {
    char tB[10], dB[15];
    strftime(tB, sizeof(tB), "%H:%M", &timeinfo);
    strftime(dB, sizeof(dB), "%d.%m.%Y", &timeinfo);
    doc["time"] = tB;
    doc["date"] = dB;
    doc["day"]  = getDayName(timeinfo.tm_wday);
  } else {
    doc["time"] = "--:--";
    doc["date"] = "--.--.----";
    doc["day"]  = "----";
  }

  doc["device_id"] = cloudDeviceId;
  doc["bat_main"] = stateMainBat ? 1 : 0;
  doc["ups"]      = stateUPS ? 1 : 0;

  for (int i = 0; i < 20; i++) {
    char rk[4], nk[4];
    snprintf(rk, sizeof(rk), "r%d", i + 1);
    snprintf(nk, sizeof(nk), "n%d", i + 1);
    doc[rk] = stateR[i] ? 1 : 0;
    doc[nk] = relayNames[i];
  }

  doc["temp"]        = cache_temperature;
  doc["t_out"]       = cache_tempOutside;
  doc["t_wat"]       = cache_tempWater;
  doc["t_ext"]       = cache_tempExtra;
  doc["water"]       = cache_waterLevelPercent;
  doc["water_liter"] = cache_waterLiters;
  doc["bat"]         = cache_batteryVoltage;
  doc["bat_percent"] = cache_batteryPercent;
  doc["mq2"]         = cache_mq2;
  doc["rain"]        = cache_rainPercent;
  doc["pitch"]       = cache_pitch;
  doc["roll"]        = cache_roll;
  doc["rs2"]         = cache_rssi2;
  doc["rs3"]         = cache_rssi3;
  doc["rs4"]         = cache_rssi4;
  doc["wh_target"]   = targetWaterTemp;
  doc["wh_on"]       = waterHeaterOn ? 1 : 0;
  doc["wh_auto"]     = waterHeaterAutoMode ? 1 : 0;
  doc["f_target"]    = targetFridgeTemp;
  doc["f_on"]        = fridgeFanOn ? 1 : 0;
  doc["f_auto"]      = fridgeAutoMode ? 1 : 0;
  doc["w_pwr"]       = stateWebastoPower ? 1 : 0;
  doc["w_target"]    = targetTemp;
  doc["w_on"]        = webastoOn ? 1 : 0;
  doc["w_auto"]      = webastoAutoMode ? 1 : 0;
  doc["gas_al"]      = gasAlarmEnabled ? 1 : 0;
  doc["rain_al"]     = rainAlarmEnabled ? 1 : 0;

  String info = "";
  if (!stateWebastoPower) info = "GUC KESIK (SISTEM KAPALI)";
  else if (webastoOn) {
    unsigned long ms = totalWebastoOnTimeMs + (millis() - currentCycleStartTime);
    info = "Calisma: " + String((ms / 60000) / 60) + "sa " + String((ms / 60000) % 60) + "dk";
  } else if (!webastoAutoMode) info = "Oto Mod Kapali";
  else {
    unsigned long tso = millis() - lastTurnOffTime;
    info = (!firstRun && tso < protectionTime)
      ? "Koruma: " + String((protectionTime - tso) / 1000) + " sn"
      : "Sistem Hazir";
  }

  doc["w_info"] = info;
  doc["rssi"]   = WiFi.RSSI();

  String out;
  serializeJson(doc, out);
  return out;
}

int cloudParamInt(JsonObject params, const char* key, int fallback = 0) {
  if (!params[key].is<int>() && !params[key].is<double>()) return fallback;
  return params[key].as<int>();
}

bool cloudHasParam(JsonObject params, const char* key) {
  return !params[key].isNull();
}

bool executeCloudCommand(const String& action, JsonObject params) {
  if (action == "toggleMainBat") {
    stateMainBat = !stateMainBat;
    digitalWrite(MAIN_BATTERY_RELAY_PIN, stateMainBat ? HIGH : LOW);
    return true;
  }
  if (action == "toggleUPS") {
    stateUPS = !stateUPS;
    digitalWrite(UPS_RELAY_PIN, stateUPS ? HIGH : LOW);
    return true;
  }
  if (action == "toggleWebastoPower") {
    if (stateWebastoPower) {
      if (webastoOn || (!firstRun && (millis() - lastTurnOffTime < protectionTime))) {
        return false;
      }
    }
    stateWebastoPower = !stateWebastoPower;
    digitalWrite(WEBASTO_POWER_RELAY_PIN, stateWebastoPower ? HIGH : LOW);
    if (!stateWebastoPower) { webastoOn = false; digitalWrite(WEBASTO_RELAY_PIN, LOW); }
    return true;
  }
  if (action == "toggle") {
    int id = cloudParamInt(params, "id", -1);
    if (id < 1 || id > 20) return false;
    toggleRelay(id);
    return true;
  }
  if (action == "setRelayName") {
    int id = cloudParamInt(params, "id", -1);
    if (id < 1 || id > 20) return false;
    if (!params["name"].is<const char*>()) return false;
    const char* name = params["name"].as<const char*>();
    if (!name || strlen(name) == 0) return false;
    saveRelayName(id - 1, String(name));
    return true;
  }
  if (action == "allOff") {
    for (int i = 0; i < 20; i++) stateR[i] = false;
    digitalWrite(RELAY1, LOW); digitalWrite(RELAY2, LOW); digitalWrite(RELAY3, LOW); digitalWrite(RELAY4, LOW);
    digitalWrite(RELAY5, LOW); digitalWrite(RELAY6, LOW); digitalWrite(RELAY7, LOW); digitalWrite(RELAY8, LOW);
    updateSlaves();
    return true;
  }
  if (action == "setTarget") {
    if (!cloudHasParam(params, "val")) return false;
    targetTemp += cloudParamInt(params, "val");
    targetTemp = constrain(targetTemp, 1, 35);
    EEPROM.put(ADDR_TARGET, targetTemp); EEPROM.commit();
    return true;
  }
  if (action == "setWaterTarget") {
    if (!cloudHasParam(params, "val")) return false;
    targetWaterTemp += cloudParamInt(params, "val");
    targetWaterTemp = constrain(targetWaterTemp, 20, 50);
    EEPROM.put(ADDR_WATER_TARGET, targetWaterTemp); EEPROM.commit();
    return true;
  }
  if (action == "setFridgeTarget") {
    if (!cloudHasParam(params, "val")) return false;
    targetFridgeTemp += cloudParamInt(params, "val");
    targetFridgeTemp = constrain(targetFridgeTemp, 40, 50);
    EEPROM.put(ADDR_FRIDGE_TARGET, targetFridgeTemp); EEPROM.commit();
    return true;
  }
  if (action == "toggleAuto") {
    webastoAutoMode = !webastoAutoMode;
    EEPROM.put(ADDR_AUTO_MODE, webastoAutoMode ? 1 : 0); EEPROM.commit();
    return true;
  }
  if (action == "toggleWaterAuto") {
    waterHeaterAutoMode = !waterHeaterAutoMode;
    EEPROM.put(ADDR_WATER_AUTO_MODE, waterHeaterAutoMode ? 1 : 0); EEPROM.commit();
    return true;
  }
  if (action == "toggleFridgeAuto") {
    fridgeAutoMode = !fridgeAutoMode;
    EEPROM.put(ADDR_FRIDGE_AUTO_MODE, fridgeAutoMode ? 1 : 0); EEPROM.commit();
    return true;
  }
  if (action == "manualWebasto") {
    if (!stateWebastoPower) return false;
    triggerWebasto();
    if (webastoOn) {
      webastoOn = false;
      lastTurnOffTime = millis();
      firstRun = false;
      totalWebastoOnTimeMs += (millis() - currentCycleStartTime);
    } else {
      webastoOn = true;
      currentCycleStartTime = millis();
    }
    return true;
  }
  if (action == "manualWaterHeater") {
    waterHeaterOn = !waterHeaterOn;
    updateSlaves();
    return true;
  }
  if (action == "manualFridge") {
    fridgeFanOn = !fridgeFanOn;
    updateSlaves();
    return true;
  }
  if (action == "toggleGasAlarm") {
    gasAlarmEnabled = !gasAlarmEnabled;
    EEPROM.put(ADDR_GAS_ALARM, gasAlarmEnabled ? 1 : 0); EEPROM.commit();
    return true;
  }
  if (action == "toggleRainAlarm") {
    rainAlarmEnabled = !rainAlarmEnabled;
    EEPROM.put(ADDR_RAIN_ALARM, rainAlarmEnabled ? 1 : 0); EEPROM.commit();
    return true;
  }
  return false;
}

void cloudAckCommand(const String& commandId, bool success, const String& result) {
  if (WiFi.status() != WL_CONNECTED) return;

  StaticJsonDocument<256> doc;
  doc["id"] = commandId;
  doc["success"] = success;
  doc["result"] = result;

  String body;
  serializeJson(doc, body);

  String url = String(cloudApiBase) + "/commands/ack";
  HTTPClient http;
  http.setTimeout(8000);
  http.begin(cloudClient, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Key", cloudDeviceKey);

  int code = http.POST(body);
  http.end();

  Serial.printf("[Cloud] ACK %s -> HTTP %d\n", commandId.c_str(), code);
}

void cloudPushStatus() {
  if (WiFi.status() != WL_CONNECTED) return;

  String payload = buildStatusJson();
  String url = String(cloudApiBase) + "/status/push";

  HTTPClient http;
  http.setTimeout(10000);
  http.begin(cloudClient, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Key", cloudDeviceKey);

  int code = http.POST(payload);
  http.end();

  Serial.printf("[Cloud] Push -> HTTP %d\n", code);
}

void cloudPollCommands() {
  if (WiFi.status() != WL_CONNECTED) return;

  String url = String(cloudApiBase) + "/commands/poll";
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(cloudClient, url);
  http.addHeader("X-Device-Key", cloudDeviceKey);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[Cloud] Poll -> HTTP %d\n", code);
    http.end();
    return;
  }

  String response = http.getString();
  http.end();

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    Serial.println("[Cloud] Poll JSON hatasi");
    return;
  }

  JsonArray commands = doc["data"]["commands"].as<JsonArray>();
  if (commands.isNull()) return;

  for (JsonObject cmd : commands) {
    String id = cmd["id"] | "";
    String action = cmd["action"] | "";
    JsonObject params = cmd["params"].is<JsonObject>() ? cmd["params"].as<JsonObject>() : JsonObject();
    if (id == "" || action == "") continue;

    bool ok = executeCloudCommand(action, params);
    cloudAckCommand(id, ok, ok ? "OK" : "FAILED");
    Serial.printf("[Cloud] Komut: %s -> %s\n", action.c_str(), ok ? "OK" : "FAIL");
  }

  // Komut uygulandıysa durumu hemen gönder
  if (!commands.isNull() && commands.size() > 0) {
    cloudPushStatus();
  }
}

// ===== HTML ARAYÜZÜ =====
String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Ölmezler Doğada Karavan Kontrol</title><meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no'>
<style>@import url('https://fonts.googleapis.com/css2?family=Quicksand:wght@700&display=swap');
body { font-family: 'Quicksand', sans-serif; background-color: #2b2b36; margin: 0; padding: 0; color: #ecf0f1; font-weight: 700; user-select: none; overflow-x: hidden; } *, *:before, *:after { box-sizing: border-box; } @keyframes alertBlink { 0% { box-shadow: inset 0 0 0px transparent, 0 0 0px transparent; border: 1px solid transparent; background-color: #343441; } 50% { box-shadow: inset 0 0 25px rgba(231,76,60,0.3), 0 0 15px rgba(231,76,60,0.8); border: 1px solid #e74c3c; background-color: rgba(231,76,60,0.15); } 100% { box-shadow: inset 0 0 0px transparent, 0 0 0px transparent; border: 1px solid transparent; background-color: #343441; } } .alert-blink { animation: alertBlink 1s infinite; border-radius: 12px; } .main-bg { background-color: #2b2b36; max-width: 1920px; margin: 0 auto; display: flex; flex-direction: column; } .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid #3f3f4e; padding-bottom: 8px; margin-bottom: 12px; gap: 10px; } .time-box { text-align: left; line-height: 1.2; } .time-box .time { font-size: 24px; color: #f1c40f; } .time-box .date { font-size: 12px; color: #95a5a6; } .brand-box { text-align: center; } .brand-box h1 { margin: 0; font-size: 22px; letter-spacing: 2px; color: #ffffff; } .brand-box .sub { font-size: 10px; color: #3498db; letter-spacing: 2px; text-transform: uppercase; } .conn-box { display: flex; flex-direction: column; align-items: flex-end; gap: 4px; font-size: 11px; color: #bdc3c7; } .wifi-bars { display: flex; align-items: flex-end; gap: 4px; height: 20px; } .wifi-bars span { display: inline-block; width: 5px; background-color: #4a4a5a; border-radius: 2px; transition: 0.3s; } .wifi-bars span:nth-child(1){height:20%;} .wifi-bars span:nth-child(2){height:40%;} .wifi-bars span:nth-child(3){height:60%;} .wifi-bars span:nth-child(4){height:80%;} .wifi-bars span:nth-child(5){height:100%;} .wifi-bars.s1 span:nth-child(1){background-color:#e74c3c;} .wifi-bars.s2 span:nth-child(1),.wifi-bars.s2 span:nth-child(2){background-color:#e67e22;} .wifi-bars.s3 span:nth-child(1),.wifi-bars.s3 span:nth-child(2),.wifi-bars.s3 span:nth-child(3){background-color:#f1c40f;} .wifi-bars.s4 span:nth-child(1),.wifi-bars.s4 span:nth-child(2),.wifi-bars.s4 span:nth-child(3),.wifi-bars.s4 span:nth-child(4){background-color:#2ecc71;} .wifi-bars.s5 span{background-color:#2ecc71;} .dashboard { display: grid; flex-grow: 1; min-height: 0; } .left-panel, .right-panel { display: flex; flex-direction: column; gap: 12px; } .sensor-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; } .sensor-card { background: #343441; padding: 10px; border-radius: 12px; text-align: center; border-bottom: 3px solid #3f3f4e; } .sensor-title { font-size: 12px; color: #95a5a6; margin-bottom: 3px; } .sensor-val { font-size: 18px; color: #f1c40f; } .sensor-val.blue { color: #3498db; } .sensor-val.orange { color: #e67e22; } .control-box { background: #343441; border-radius: 12px; padding: 12px; transition: 0.3s; } .box-title { font-size: 15px; margin-bottom: 10px; color: #ecf0f1; border-bottom: 1px solid #4a4a5a; padding-bottom: 6px; display: flex; justify-content: space-between; align-items: center; gap: 8px; } .gauges-box { display: flex; justify-content: space-around; padding: 5px 0; gap: 10px; } .gauge-wrapper { display: flex; flex-direction: column; align-items: center; } .gauge { width: 75px; height: 75px; border-radius: 50%; background: #1a252f; display: flex; align-items: center; justify-content: center; box-shadow: 0 4px 10px rgba(0,0,0,0.5); transition: background 0.3s; margin-top: 5px; } .gauge-inner { width: 60px; height: 60px; background-color: #343441; border-radius: 50%; display: flex; align-items: center; justify-content: center; box-shadow: inset 0 2px 5px rgba(0,0,0,0.8); } .power-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; } .power-card { background-color: #343441; padding: 8px; border-radius: 12px; cursor: pointer; transition: all 0.3s ease; border: 2px solid transparent; display: flex; flex-direction: column; align-items: center; text-align: center; } .power-card svg { margin-bottom: 3px; transition: 0.3s; width: 28px; height: 28px; } .power-card.on { border-color: #27ae60; box-shadow: 0 0 12px rgba(39,174,96,0.3); } .power-card.on svg { color: #27ae60; filter: drop-shadow(0 0 4px #27ae60); } .power-card.off { border-color: #e74c3c; box-shadow: 0 0 12px rgba(231,76,60,0.3); } .power-card.off svg { color: #e74c3c; filter: drop-shadow(0 0 4px #e74c3c); } .power-title { font-size: 11px; font-weight: bold; color: #ecf0f1; } .power-status { font-size: 11px; font-weight: bold; color: #ffffff; } .relay-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px; } .relay-btn { background: #2b2b36; border: 1px solid #3f3f4e; border-radius: 10px; padding: 8px 2px; cursor: pointer; display: flex; flex-direction: column; align-items: center; justify-content: space-between; height: 75px; transition: 0.2s; outline: none; } .relay-btn:active { transform: scale(0.95); } .relay-btn .label { color: #bdc3c7; font-size: 9px; text-transform: uppercase; font-family: 'Quicksand'; font-weight: 700; line-height: 1.1; word-wrap: break-word; overflow: hidden; text-overflow: ellipsis; display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; width: 100%; text-align: center; } .relay-btn svg { width: 22px; height: 22px; stroke: #3498db; fill: none; stroke-width: 1.5; stroke-linecap: round; stroke-linejoin: round; margin: auto 0; transition: 0.3s; } .relay-btn .status-line { width: 60%; height: 4px; border-radius: 2px; background: #e74c3c; transition: 0.3s; box-shadow: 0 0 8px rgba(231,76,60,0.6); } .relay-btn.on .status-line { background: #2ecc71; box-shadow: 0 0 10px rgba(46,204,113,0.8); } .relay-btn.on svg { stroke: #2ecc71; filter: drop-shadow(0 0 3px #2ecc71); } .edit-names-btn { background:#34495e; border:none; color:white; padding:5px 8px; border-radius:6px; font-family:'Quicksand'; font-weight:700; cursor:pointer; font-size:11px; } .btn-all { width:100%; padding:10px; font-size:14px; font-family:'Quicksand'; font-weight:700; border:none; border-radius:10px; background:#8e44ad; color:white; cursor:pointer; text-transform:uppercase; letter-spacing:2px; margin-top:8px;} .heater-grid { display: grid; grid-template-columns: 1fr; gap: 10px; } @media (min-width: 600px) { .heater-grid { grid-template-columns: repeat(3, 1fr); } } .heater-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px; gap: 5px; } .heater-target { display: flex; align-items: center; gap: 8px; background: #2b2b36; padding: 5px; border-radius: 10px; justify-content: center; } .heater-target span { font-size: 20px; color: #e67e22; width: 35px; text-align: center; } .btn-circle { width: 30px; height: 30px; border-radius: 50%; border: none; background: #3f3f4e; color: white; font-size: 18px; font-weight: 700; cursor: pointer; display: flex; align-items: center; justify-content: center; transition: 0.2s; } .btn-circle:active { transform: scale(0.9); } .btn-action { background:#2980b9; border:none; color:white; padding:6px 10px; border-radius:6px; font-family:'Quicksand'; font-weight:700; cursor:pointer; font-size:11px; } .btn-auto { width:100%; padding:8px; border:none; border-radius:8px; font-family:'Quicksand'; font-weight:700; font-size:12px; margin-top:6px; cursor:pointer; transition:0.3s; color:white;} .disabled-area { opacity:0.3; pointer-events:none; filter:grayscale(100%); transition:0.3s; } .pwr-btn { padding:5px 8px; border-radius:6px; font-family:'Quicksand'; font-weight:700; font-size:11px; color:white; border:none; cursor:pointer; transition:0.3s; } .pwr-on{background:#27ae60;} .pwr-off{background:#e74c3c;} .on-bg{background-color:#27ae60!important;} .off-bg{background-color:#e74c3c!important;} .terazi-rain-grid { display: grid; grid-template-columns: 1fr; gap: 12px; } .terazi-wrap, .rain-wrap { display: flex; justify-content: space-between; align-items: center; background: #1a252f; padding: 12px; border-radius: 12px; box-shadow: inset 0 0 8px rgba(0,0,0,0.5); gap: 10px; height: 82px; } .cv-pitch{transform-origin:60% 75%;transition:transform 0.4s ease-out;} .cv-roll{transform-origin:50% 75%;transition:transform 0.4s ease-out;} .val-text{font-size:18px;color:#f1c40f;font-weight:700;background:#1a252f;padding:5px 12px;border-radius:10px;box-shadow:inset 0 2px 4px rgba(0,0,0,0.5);border:1px solid #3f3f4e;display:inline-block;}
@media (orientation: landscape) {
  html,body{height:100%;overflow:hidden;}
  .main-bg{height:100vh;padding:10px 15px;}
  .dashboard{grid-template-columns:320px 1fr;gap:12px;height:calc(100vh - 65px);}
  .left-panel{height:100%;overflow-y:auto;padding-right:4px;}
  .right-panel{height:100%;overflow-y:auto;padding-right:4px;display:flex;flex-direction:column;justify-content:space-between;gap:10px;}
  .power-grid,.heater-grid{flex-shrink:0;}
  .right-panel > .control-box{flex-grow:1.5;display:flex;flex-direction:column;margin-bottom:0;padding-bottom:10px;}
  .relay-grid{flex-grow:1;grid-template-columns:repeat(10,1fr);gap:6px;align-items:stretch;}
  .relay-btn{height:100%;min-height:50px;padding:5px 2px;justify-content:center;gap:5px;}
  .relay-btn svg{width:20px;height:20px;margin:0;}
  .relay-btn .label{font-size:8px;}
  .btn-all{margin-top:auto;flex-shrink:0;}
  .terazi-rain-grid{flex-grow:1;grid-template-columns:1fr 1fr;align-items:stretch;display:grid;gap:12px;}
  .terazi-rain-grid .control-box{display:flex;flex-direction:column;justify-content:space-between;height:100%;margin:0;padding-bottom:10px;}
  .terazi-wrap,.rain-wrap{flex-grow:1;height:100%;margin-top:5px;}
  .left-panel::-webkit-scrollbar,.right-panel::-webkit-scrollbar{width:4px;}
  .left-panel::-webkit-scrollbar-thumb,.right-panel::-webkit-scrollbar-thumb{background:rgba(255,255,255,0.15);border-radius:2px;}
}
@media (orientation: portrait) {
  html,body{overflow-y:auto;}
  .main-bg{min-height:100vh;height:auto;padding:12px;}
  .relay-grid{grid-template-columns:repeat(4,1fr);gap:6px;}
  .relay-btn{height:75px;}
  .relay-btn .label{font-size:9px;}
  .terazi-wrap,.rain-wrap{flex-direction:row;justify-content:space-around;}
  .left-panel,.right-panel{display:contents;}
  .dashboard{display:flex;flex-direction:column;gap:15px;}
  .network-box{order:1;} .sensor-grid{order:2;} .energy-box{order:3;} .power-grid-wrap{order:4;margin-top:0;}
  .relay-box{order:5;} .heater-grid{order:6;display:flex!important;flex-direction:column!important;gap:15px;}
  .heater-boiler{order:1;} .heater-webasto{order:2;} .heater-fridge{order:3;}
  .terazi-rain-grid{order:7;display:flex!important;flex-direction:column!important;gap:15px;}
  .terazi-box{order:1;} .rain-box{order:2;} .gas-alarm-box{order:8;margin-top:0;}
}
</style></head><body><div class='main-bg'>

<div class='header'>
  <div class='time-box'><div id='sys_time' class='time'>--:--</div><div id='sys_date' class='date'>--.--.---- <span id='sys_day'></span></div></div>
  <div class='brand-box'><h1>ÖLMEZLER DOĞADA</h1><div class='sub'>KARAVAN KONTROL SİSTEMİ</div></div>
  <div style='display:flex;align-items:center;gap:15px;'>
    <button id='mic_btn' onclick='startVoiceControl()' style='background:#e74c3c;border:none;border-radius:50%;width:45px;height:45px;cursor:pointer;box-shadow:0 0 10px rgba(231,76,60,0.6);display:flex;justify-content:center;align-items:center;transition:0.3s;'>
      <svg viewBox='0 0 24 24' width='22' height='22' stroke='white' stroke-width='2' fill='none' stroke-linecap='round' stroke-linejoin='round'><path d='M12 2a3 3 0 0 0-3 3v7a3 3 0 0 0 6 0V5a3 3 0 0 0-3-3Z'></path><path d='M19 10v2a7 7 0 0 1-14 0v-2'></path><line x1='12' y1='19' x2='12' y2='23'></line><line x1='8' y1='23' x2='16' y2='23'></line></svg>
    </button>
    <div class='conn-box'><div id='wifi_bars' class='wifi-bars s0'><span></span><span></span><span></span><span></span><span></span></div><div id='wifi_text'>Sinyal Bekleniyor</div></div>
  </div>
</div>

<div class='dashboard'>
 
  <div class='left-panel'>

    <div class='control-box network-box'>
      <div class='box-title'>Sistem Ağ Durumu</div>
      <div style='display:flex;justify-content:space-around;align-items:center;gap:10px;'>
        <div style='text-align:center;color:#95a5a6;font-size:12px;'>Kart 2<br><span id='web_rs2' style='font-size:15px;'>-- dBm</span></div>
        <div style='text-align:center;color:#95a5a6;font-size:12px;'>Kart 3<br><span id='web_rs3' style='font-size:15px;'>-- dBm</span></div>
        <div style='text-align:center;color:#95a5a6;font-size:12px;'>Kart 4<br><span id='web_rs4' style='font-size:15px;'>-- dBm</span></div>
      </div>
    </div>

    <div class='sensor-grid'>
      <div class='sensor-card'><div class='sensor-title'>Gün Doğumu</div><div id='web_sunrise' class='sensor-val orange'>--:--</div></div>
      <div class='sensor-card'><div class='sensor-title'>Gün Batımı</div><div id='web_sunset' class='sensor-val orange'>--:--</div></div>
      <div class='sensor-card'><div class='sensor-title'>İç Ortam Sıcaklığı</div><div id='web_temp' class='sensor-val'>-- °C</div></div>
      <div class='sensor-card'><div class='sensor-title'>Dış Ortam Sıcaklığı</div><div id='web_t_out' class='sensor-val blue'>-- °C</div></div>
      <div class='sensor-card'><div class='sensor-title'>Su Sıcaklığı</div><div id='web_t_wat' class='sensor-val blue'>-- °C</div></div>
      <div class='sensor-card'><div class='sensor-title'>Konum</div><div id='web_location' class='sensor-val' style='font-size:14px;word-break:break-all;'>Yükleniyor...</div></div>
    </div>

    <div class='control-box energy-box'>
      <div class='box-title'>Depo ve Enerji Durumu</div>
      <div class='gauges-box'>
        <div class='gauge-wrapper'><div class='sensor-title'>Temiz Su Miktarı</div><div class='gauge' id='water_gauge'><div class='gauge-inner'><span id='web_water_p' style='color:#3498db;font-size:16px;'>-- %</span></div></div><div id='web_water_l' style='margin-top:5px;color:#bdc3c7;font-size:11px;'>-- L</div></div>
        <div class='gauge-wrapper'><div class='sensor-title'>LiFePO4 Voltajı</div><div class='gauge' id='bat_gauge'><div class='gauge-inner'><span id='web_bat_p' style='font-size:16px;'>-- %</span></div></div><div id='web_bat_v' style='margin-top:5px;color:#bdc3c7;font-size:11px;'>-- V</div></div>
      </div>
    </div>

    <div id='box_gas_alarm' class='control-box gas-alarm-box' onclick='toggleGasAlarm()' style='cursor:pointer;'>
      <div class='box-title'><span>Gaz Güvenlik Sistemi</span><span id='web_gas_al' style='font-size:10px;padding:3px 8px;border-radius:6px;color:#fff;'>YÜKLENİYOR...</span></div>
      <div style='display:grid;grid-template-columns:1fr;gap:10px;text-align:center;'>
        <div style='background:#2b2b36;padding:10px;border-radius:8px;'><div class='sensor-title'>LPG / Duman</div><div id='web_mq2' style='font-size:20px;font-weight:bold;'>--</div></div>
      </div>
    </div>

  </div>

  <div class='right-panel'>

    <div class='power-grid power-grid-wrap'>
      <div id='card_bat_main' class='power-card off' onclick='toggleMainBat()'><svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><rect x='2' y='7' width='16' height='10' rx='2' ry='2'></rect><line x1='22' y1='11' x2='22' y2='13'></line><line x1='6' y1='12' x2='6.01' y2='12'></line></svg><div class='power-title'>ANA AKÜ (ŞALTER)</div><div id='text_bat_main' class='power-status'>KAPALI</div></div>
      <div id='card_ups' class='power-card off' onclick='toggleUPS()'><svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><rect x='3' y='4' width='18' height='16' rx='2' ry='2'></rect><path d='M8 12 Q 10 8 12 12 T 16 12'></path></svg><div class='power-title'>SMART INVERTER</div><div id='text_ups' class='power-status'>KAPALI</div></div>
    </div>

    <div class='control-box relay-box'>
      <div class='box-title'><span>Karavan Kontrol Ünitesi</span><button class='edit-names-btn' onclick='editRelayNames()'>Düzenle</button></div>
      <div class='relay-grid'>
  )rawliteral";

  for (int i=1; i<=20; i++) {
    html += "<button id='card_r"+String(i)+"' class='relay-btn off' onclick='sendToggle("+String(i)+")'>";
    html += "<div id='label_r"+String(i)+"' class='label'>"+htmlEscape(relayNames[i-1])+"</div>";
    html += "<svg viewBox='0 0 24 24'><rect x='2' y='7' width='20' height='15' rx='2' ry='2'/></svg>";
    html += "<div class='status-line'></div></button>";
  }

  html += R"rawliteral(
      </div>
      <button class='btn-all' onclick='sendAllOff()'>TÜMÜNÜ KAPAT</button>
    </div>

    <div class='terazi-rain-grid'>
      <div class='control-box terazi-box'>
        <div class='box-title'>Akıllı Karavan Terazisi</div>
        <div class='terazi-wrap'>
          <div style='text-align:center;'><div class='sensor-title' style='margin-bottom:3px;color:#bdc3c7;font-size:11px;'>Ön - Arka</div><div id='web_pitch' class='val-text'>-- °</div></div>
          <div style='display:flex;gap:12px;align-items:flex-end;justify-content:center;'>
            <svg class='cv-pitch' id='cv_pitch' viewBox='0 0 100 60' width='75' height='45'><line x1='5' y1='45' x2='25' y2='45' stroke='#95a5a6' stroke-width='4'/><path d='M 25 20 Q 30 5 45 5 L 85 5 Q 95 5 95 15 L 95 45 L 25 45 Z' fill='#ecf0f1'/><rect x='35' y='15' width='20' height='12' rx='2' fill='#3498db'/><rect x='65' y='15' width='15' height='25' rx='2' fill='#bdc3c7'/><circle cx='60' cy='45' r='10' fill='#2c3e50'/><circle cx='60' cy='45' r='4' fill='#bdc3c7'/></svg>
            <svg class='cv-roll' id='cv_roll' viewBox='0 0 60 60' width='45' height='45'><path d='M 10 45 L 10 15 Q 10 5 20 5 L 40 5 Q 50 5 50 15 L 50 45 Z' fill='#ecf0f1'/><rect x='15' y='15' width='30' height='12' rx='2' fill='#3498db'/><circle cx='10' cy='45' r='8' fill='#2c3e50'/><circle cx='50' cy='45' r='8' fill='#2c3e50'/></svg>
          </div>
          <div style='text-align:center;'><div class='sensor-title' style='margin-bottom:5px;color:#bdc3c7;font-size:11px;'>Sağ - Sol</div><div id='web_roll' class='val-text'>-- °</div></div>
        </div>
      </div>

      <div id='box_rain_alarm' class='control-box rain-box' onclick='toggleRainAlarm()' style='cursor:pointer;'>
        <div class='box-title'><span>Hava ve Yağmur Durumu</span><span id='web_rain_al' style='font-size:10px;padding:3px 8px;border-radius:6px;color:#fff;'>YÜKLENİYOR...</span></div>
        <div class='rain-wrap'>
          <div style='display:flex;align-items:center;gap:10px;'>
            <svg viewBox='0 0 24 24' width='32' height='32' fill='none' stroke='#3498db' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M20 17.58A5 5 0 0 0 18 8h-1.26A8 8 0 1 0 4 16.25'/><line x1='8' y1='16' x2='6' y2='21'/><line x1='12' y1='17' x2='10' y2='22'/><line x1='16' y1='16' x2='14' y2='21'/></svg>
            <div><div id='web_rain_status' style='font-size:15px;color:#2ecc71;'>Hava Temiz</div><div class='sensor-title' style='font-size:11px;margin:0;'>Yağmur Yoğunluğu</div></div>
          </div>
          <div id='web_rain' class='val-text'>-- %</div>
        </div>
      </div>
    </div>

    <div class='heater-grid'>

      <div class='control-box heater-boiler' style='margin:0;'>
        <div class='box-title'><span>Su Isıtıcı Rezistans</span></div>
        <div class='heater-row'><span style='color:#bdc3c7;'>Durum:</span><span id='web_wh_status' style='font-size:14px;'>--</span><button class='btn-action' onclick='manualWaterHeater()'>Tetikle</button></div>
        <div class='heater-target'><button class='btn-circle' onclick='changeWaterTarget(-1)'>-</button><span id='web_wh_target' style='color:#3498db;'>--</span><button class='btn-circle' onclick='changeWaterTarget(1)'>+</button></div>
        <button id='btn_wh_auto' class='btn-auto' onclick='toggleWaterAutoMode()'>Otomatik Mod: --</button>
      </div>

      <div class='control-box heater-webasto' style='margin:0;'>
        <div class='box-title'><span>Webasto Isıtıcı</span><button id='btn_w_pwr' class='pwr-btn pwr-off' onclick='toggleWebastoPower()'>GÜÇ: KAPALI</button></div>
        <div id='webasto_controls' class='disabled-area'>
          <div class='heater-row'><span style='color:#bdc3c7;'>Durum:</span><span id='web_w_status' style='font-size:14px;'>--</span><button class='btn-action' onclick='manualWebasto()'>Tetikle</button></div>
          <div id='web_w_info' style='font-size:11px;color:#95a5a6;margin-bottom:10px;text-align:center;'>--</div>
          <div class='heater-target'><button class='btn-circle' onclick='changeTarget(-1)'>-</button><span id='web_w_target' style='color:#e67e22;'>--</span><button class='btn-circle' onclick='changeTarget(1)'>+</button></div>
          <button id='btn_w_auto' class='btn-auto' onclick='toggleAutoMode()'>Otomatik Mod: --</button>
        </div>
      </div>

      <div class='control-box heater-fridge' style='margin:0;'>
        <div class='box-title'><span>Buzdolabı Fanı</span><span id='web_t_ext_fridge' style='color:#f1c40f;font-size:14px;font-weight:bold;'>-- °C</span></div>
        <div class='heater-row'><span style='color:#bdc3c7;'>Durum:</span><span id='web_f_status' style='font-size:14px;'>--</span><button class='btn-action' onclick='manualFridge()'>Tetikle</button></div>
        <div class='heater-target'><button class='btn-circle' onclick='changeFridgeTarget(-1)'>-</button><span id='web_f_target' style='color:#3498db;'>--</span><button class='btn-circle' onclick='changeFridgeTarget(1)'>+</button></div>
        <button id='btn_f_auto' class='btn-auto' onclick='toggleFridgeAuto()'>Otomatik Mod: --</button>
      </div>

    </div>
  </div>
</div>
</div>

<script>
window.lastData = {};
let rainWarned = false; let smokeWarned = false;

function speak(text) { if('speechSynthesis' in window){let m=new SpeechSynthesisUtterance(text);m.lang='tr-TR';window.speechSynthesis.speak(m);} }

function fetchSunTimes() {
  fetch('https://get.geojs.io/v1/ip/geo.json').then(r=>r.json()).then(d=>{
    let locEl = document.getElementById('web_location');
    if(locEl) {
      let city = d.city ? d.city : "";
      let region = d.region ? d.region.replace(" Province", "") : "";
      let country = d.country ? d.country : "";

      if (city !== "" && region !== "" && city !== region) {
        locEl.innerHTML = city + " / " + region;
      } else if (region !== "") {
        locEl.innerHTML = region;
      } else if (city !== "") {
        locEl.innerHTML = city;
      } else if (country !== "") {
        locEl.innerHTML = country;
      } else {
        locEl.innerHTML = "Konum Bulunamadı";
      }
    }
    return fetch('https://api.sunrise-sunset.org/json?lat='+d.latitude+'&lng='+d.longitude+'&formatted=0');
  }).then(r=>r.json()).then(d=>{
    let ss=new Date(d.results.sunset).toLocaleTimeString('tr-TR',{hour:'2-digit',minute:'2-digit'});
    let sr=new Date(d.results.sunrise).toLocaleTimeString('tr-TR',{hour:'2-digit',minute:'2-digit'});
    let ssEl=document.getElementById('web_sunset');  if(ssEl)ssEl.innerHTML=ss;
    let srEl=document.getElementById('web_sunrise'); if(srEl)srEl.innerHTML=sr;
  }).catch(e=>console.log(e));
}
fetchSunTimes(); setInterval(fetchSunTimes,3600000);

function set(id,val){let e=document.getElementById(id);if(e)e.innerHTML=val;}
function setStyle(id,prop,val){let e=document.getElementById(id);if(e)e.style[prop]=val;}
function setClass(id,cls){let e=document.getElementById(id);if(e)e.className=cls;}

function fetchStatus() {
  fetch('/status').then(r=>r.json()).then(d=>{
    window.lastData=d;

    set('sys_time',d.time); set('sys_date',d.date+' '+d.day);

    let rssi=d.rssi; let bars=0;
    if(rssi>-60)bars=5; else if(rssi>-70)bars=4; else if(rssi>-80)bars=3; else if(rssi>-90)bars=2; else if(rssi<0)bars=1;
    setClass('wifi_bars','wifi-bars s'+bars);
    set('wifi_text',bars>0?rssi+' dBm':'Bağlantı Yok');

    setClass('card_bat_main',d.bat_main==1?'power-card on':'power-card off');
    set('text_bat_main',d.bat_main==1?'AÇIK':'KAPALI');
    setClass('card_ups',d.ups==1?'power-card on':'power-card off');
    set('text_ups',d.ups==1?'AÇIK':'KAPALI');

    let wPwr=d.w_pwr; let btnPwr=document.getElementById('btn_w_pwr');
    let wCtrl=document.getElementById('webasto_controls');
    if(btnPwr&&wCtrl){if(wPwr==1){btnPwr.className='pwr-btn pwr-on';btnPwr.innerHTML='GÜÇ: AÇIK';wCtrl.className='';}else{btnPwr.className='pwr-btn pwr-off';btnPwr.innerHTML='GÜÇ: KAPALI';wCtrl.className='disabled-area';}}

    for(let i=1;i<=20;i++){
      let c=document.getElementById('card_r'+i);
      if(c)c.className=(d['r'+i]==1)?'relay-btn on':'relay-btn off';
      let l=document.getElementById('label_r'+i); if(l&&d['n'+i])l.innerHTML=d['n'+i];
    }

    set('web_temp',  d.temp.toFixed(1)+' °C');
    set('web_t_out', d.t_out.toFixed(1)+' °C');
    set('web_t_wat', d.t_wat.toFixed(1)+' °C');
    set('web_t_ext_fridge', d.t_ext.toFixed(1)+' °C');

    set('web_wh_target', d.wh_target.toFixed(0));
    let whSt=document.getElementById('web_wh_status');
    if(whSt){whSt.innerHTML=(d.wh_on==1)?'AÇIK (Isıtıyor)':'KAPALI';whSt.style.color=(d.wh_on==1)?'#27ae60':'#e74c3c';}
    let btnWhAuto=document.getElementById('btn_wh_auto');
    if(btnWhAuto){btnWhAuto.className=(d.wh_auto==1)?'btn-auto on-bg':'btn-auto off-bg';btnWhAuto.innerHTML='Otomatik Mod: '+((d.wh_auto==1)?'AÇIK':'KAPALI');}

    set('web_f_target', d.f_target.toFixed(0));
    let fSt=document.getElementById('web_f_status');
    if(fSt){fSt.innerHTML=(d.f_on==1)?'AÇIK (Soğutuyor)':'KAPALI';fSt.style.color=(d.f_on==1)?'#3498db':'#e74c3c';}
    let btnFAuto=document.getElementById('btn_f_auto');
    if(btnFAuto){btnFAuto.className=(d.f_auto==1)?'btn-auto on-bg':'btn-auto off-bg';btnFAuto.innerHTML='Otomatik Mod: '+((d.f_auto==1)?'AÇIK':'KAPALI');}

    set('web_w_target', d.w_target.toFixed(0));
    let wSt=document.getElementById('web_w_status');
    if(wSt){wSt.innerHTML=(d.w_on==1)?'AÇIK':'KAPALI';wSt.style.color=(d.w_on==1)?'#27ae60':'#e74c3c';}
    set('web_w_info', d.w_info);
    let btnWAuto=document.getElementById('btn_w_auto');
    if(btnWAuto){btnWAuto.className=(d.w_auto==1)?'btn-auto on-bg':'btn-auto off-bg';btnWAuto.innerHTML='Otomatik Mod: '+((d.w_auto==1)?'AÇIK':'KAPALI');}

    let wP=d.water;
    if(wP<0)wP=0; if(wP>100)wP=100;
    let wColor='hsl('+Math.floor(wP*1.2)+',85%,55%)';
    let wPEl=document.getElementById('web_water_p');
    if(wPEl){wPEl.innerHTML=wP+' %';wPEl.style.color=wColor;}
    set('web_water_l', d.water_liter+' L');
    let wG=document.getElementById('water_gauge'); if(wG)wG.style.background='conic-gradient('+wColor+' '+wP+'%, #1a252f 0)';
    
    let batV=d.bat; let batP=d.bat_percent;
    let bColor='hsl('+Math.floor(batP*1.2)+',85%,55%)';
    let bPEl=document.getElementById('web_bat_p');
    if(bPEl){bPEl.innerHTML=batP+' %';bPEl.style.color=bColor;}
    set('web_bat_v', batV.toFixed(1)+' V');
    let bG=document.getElementById('bat_gauge'); if(bG)bG.style.background='conic-gradient('+bColor+' '+batP+'%, #1a252f 0)';
    
    let rs2El=document.getElementById('web_rs2');
    if(rs2El){rs2El.innerHTML=d.rs2<=-100?'Sinyal Yok':d.rs2+' dBm';rs2El.style.color=d.rs2<=-100?'#e74c3c':'#3498db';}
    let rs3El=document.getElementById('web_rs3');
    if(rs3El){rs3El.innerHTML=d.rs3<=-100?'Sinyal Yok':d.rs3+' dBm';rs3El.style.color=d.rs3<=-100?'#e74c3c':'#e67e22';}
    let rs4El=document.getElementById('web_rs4');
    if(rs4El){rs4El.innerHTML=d.rs4<=-100?'Sinyal Yok':d.rs4+' dBm';rs4El.style.color=d.rs4<=-100?'#e74c3c':'#2ecc71';}

    let pitchEl=document.getElementById('web_pitch');
    if(pitchEl){pitchEl.innerHTML=d.pitch.toFixed(1)+' °';document.getElementById('cv_pitch').style.transform='rotate('+d.pitch+'deg)';}
    let rollEl=document.getElementById('web_roll');
    if(rollEl){rollEl.innerHTML=d.roll.toFixed(1)+' °';document.getElementById('cv_roll').style.transform='rotate('+d.roll+'deg)';}

    let mq2El=document.getElementById('web_mq2');
    if(mq2El){mq2El.innerHTML=d.mq2;mq2El.style.color=d.mq2>1200?'#e74c3c':'#2ecc71';}
    let gasBox=document.getElementById('box_gas_alarm');
    if(gasBox){if(d.mq2>1200)gasBox.classList.add('alert-blink');else gasBox.classList.remove('alert-blink');}

    let rainEl=document.getElementById('web_rain');
    if(rainEl){
      rainEl.innerHTML=d.rain+' %';
      let rStatus='Hava Temiz';
      if(d.rain>65)rStatus='Sağanak Yağmur'; else if(d.rain>20)rStatus='Yağmur Yağıyor'; else if(d.rain>5)rStatus='Hafif Çiseleme';
      let rStEl=document.getElementById('web_rain_status');
      if(rStEl){rStEl.innerHTML=rStatus;rStEl.style.color=d.rain>5?'#3498db':'#2ecc71';}
    }
    let rainBox=document.getElementById('box_rain_alarm');
    if(rainBox){if(d.rain>20)rainBox.classList.add('alert-blink');else rainBox.classList.remove('alert-blink');}

    let gasAlEl=document.getElementById('web_gas_al');
    if(gasAlEl){gasAlEl.innerHTML=d.gas_al?'🔔 SİREN AÇIK':'🔕 SİREN KAPALI';gasAlEl.style.background=d.gas_al?'#27ae60':'#e74c3c';}
    let rainAlEl=document.getElementById('web_rain_al');
    if(rainAlEl){rainAlEl.innerHTML=d.rain_al?'🔔 SİREN AÇIK':'🔕 SİREN KAPALI';rainAlEl.style.background=d.rain_al?'#27ae60':'#e74c3c';}

    if(d.rain>20&&!rainWarned){speak("Dikkat, yağmur başladı. Dışardaki eşyalarınız ıslanmasın.");rainWarned=true;} else if(d.rain<=5){rainWarned=false;}
    if(d.mq2>1200&&!smokeWarned){speak("Tehlike! Karavanda duman veya gaz algılandı.");smokeWarned=true;} else if(d.mq2<=1200){smokeWarned=false;}

  }).catch(e=>console.log(e));
}
setInterval(fetchStatus,1000);

// Kontrol fonksiyonları
function toggleMainBat()   {fetch('/toggleMainBat').then(()=>setTimeout(fetchStatus,100));}
function toggleUPS()        {fetch('/toggleUPS').then(()=>setTimeout(fetchStatus,100));}
function sendToggle(id)     {fetch('/toggle?id='+id).then(()=>setTimeout(fetchStatus,100));}
function sendAllOff()       {fetch('/allOff').then(()=>setTimeout(fetchStatus,100));}
function changeTarget(v)    {fetch('/setTarget?val='+v).then(()=>setTimeout(fetchStatus,100));}
function changeWaterTarget(v){fetch('/setWaterTarget?val='+v).then(()=>setTimeout(fetchStatus,100));}
function changeFridgeTarget(v){fetch('/setFridgeTarget?val='+v).then(()=>setTimeout(fetchStatus,100));}
function toggleAutoMode()   {fetch('/toggleAuto').then(()=>setTimeout(fetchStatus,100));}
function toggleWaterAutoMode(){fetch('/toggleWaterAuto').then(()=>setTimeout(fetchStatus,100));}
function toggleFridgeAuto() {fetch('/toggleFridgeAuto').then(()=>setTimeout(fetchStatus,100));}
function manualWebasto()    {fetch('/manualWebasto').then(()=>setTimeout(fetchStatus,100));}
function manualWaterHeater(){fetch('/manualWaterHeater').then(()=>setTimeout(fetchStatus,100));}
function manualFridge()     {fetch('/manualFridge').then(()=>setTimeout(fetchStatus,100));}

function toggleWebastoPower(){
  fetch('/toggleWebastoPower').then(r => {
    if(r.status === 403){
      alert('⚠️ UYARI: Webasto şu an çalışıyor veya soğuma/koruma modunda!\n\nCihazın arızalanmasını önlemek için sistem tamamen durmadan ("Sistem Hazır" yazmadan) ana gücü kapatamazsınız.');
    }
    setTimeout(fetchStatus,100);
  });
}

function toggleGasAlarm()   {fetch('/toggleGasAlarm').then(()=>setTimeout(fetchStatus,100));}
function toggleRainAlarm()  {fetch('/toggleRainAlarm').then(()=>setTimeout(fetchStatus,100));}
function editRelayNames()   {for(let i=1;i<=20;i++){let c=document.getElementById('label_r'+i)?.innerText||'';let n=prompt('Röle '+i+' ismi:',c);if(n!==null)fetch('/setRelayName?id='+i+'&name='+encodeURIComponent(n)).then(()=>setTimeout(fetchStatus,200));}}

// Ses tanıma
const SpeechRecognition=window.SpeechRecognition||window.webkitSpeechRecognition;
if(SpeechRecognition){
  const rec=new SpeechRecognition();rec.lang='tr-TR';rec.continuous=false;rec.interimResults=false;
  rec.onstart=function(){document.getElementById('mic_btn').style.background='#2ecc71';document.getElementById('mic_btn').style.boxShadow='0 0 15px rgba(46,204,113,0.8)';};
  rec.onend=function(){document.getElementById('mic_btn').style.background='#e74c3c';document.getElementById('mic_btn').style.boxShadow='0 0 10px rgba(231,76,60,0.6)';};
  rec.onresult=function(e){
    let cmd=e.results[0][0].transcript.toLowerCase();
    if(cmd.includes('iç ortam sıcaklığı')){speak("İç ortam sıcaklığı "+window.lastData.temp.toFixed(1)+" derece.");}
    else if(cmd.includes('dış ortam sıcaklığı')){speak("Dış ortam sıcaklığı "+window.lastData.t_out.toFixed(1)+" derece.");}
    else if(cmd.includes('su sıcaklığı')){speak("Su sıcaklığı "+window.lastData.t_wat.toFixed(1)+" derece.");}
    else if(cmd.includes('temiz su')){speak("Temiz su miktarınız yüzde "+window.lastData.water+".");}
    else if(cmd.includes('akü voltajı')){speak("Lifepo4 akü voltajı "+window.lastData.bat.toFixed(1)+" volt.");}
    else if(cmd.includes('durum raporu')||cmd.includes('rapor ver')){speak("Sırasıyla söylüyorum Hüseyin. İç ortam sıcaklığı "+window.lastData.temp.toFixed(1)+" derece, dış ortam sıcaklığı "+window.lastData.t_out.toFixed(1)+" derece, temiz su miktarı yüzde "+window.lastData.water+", akü voltajı "+window.lastData.bat.toFixed(1)+" volt, yağmur durumu yüzde "+window.lastData.rain+".");}
    else if(cmd.includes('iç aydınlatma')||cmd.includes('iç ışıkları')){sendToggle(1);speak("İç aydınlatma durumu değiştirildi.");}
    else if(cmd.includes('dış aydınlatma')||cmd.includes('dış ışıkları')){sendToggle(2);speak("Dış aydınlatma durumu değiştirildi.");}
    else if(cmd.includes('hidrofor')||cmd.includes('su pompası')){sendToggle(3);speak("Hidrofor tetiklendi.");}
    else speak("Lütfen tekrar edin.");
  };
  window.startVoiceControl=function(){rec.start();};
}else{window.startVoiceControl=function(){alert('Tarayıcınız ses tanımayı desteklemiyor. Lütfen Chrome veya Safari kullanın.');};}
</script></body></html>
  )rawliteral";
  return html;
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  loadRelayNames();

  sensorOut.begin();
  sensorIn.begin();
  telegramClient.setInsecure();
  cloudClient.setInsecure();

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);
  mpuConnected = (Wire.endTransmission() == 0);
  Serial.println(mpuConnected ? "MPU6050 bağlandı." : "MPU6050 bulunamadı.");

  pinMode(MQ2_PIN,  INPUT); pinMode(RAIN_PIN, INPUT);

  // DÜZELTME: Tüm Röle ve Sirenler ULN2003 ile uyumlu olarak "LOW" (Bırakık) başlatılıyor.
  pinMode(GAS_SIREN_PIN,  OUTPUT); digitalWrite(GAS_SIREN_PIN,  LOW);
  pinMode(RAIN_SIREN_PIN, OUTPUT); digitalWrite(RAIN_SIREN_PIN, LOW);
  pinMode(MAIN_BATTERY_RELAY_PIN, OUTPUT); digitalWrite(MAIN_BATTERY_RELAY_PIN, LOW);
  pinMode(UPS_RELAY_PIN,          OUTPUT); digitalWrite(UPS_RELAY_PIN,          LOW);
  pinMode(WEBASTO_RELAY_PIN,      OUTPUT); digitalWrite(WEBASTO_RELAY_PIN,      LOW);
  pinMode(WEBASTO_POWER_RELAY_PIN,OUTPUT); digitalWrite(WEBASTO_POWER_RELAY_PIN,LOW);
  
  pinMode(RELAY1,OUTPUT); digitalWrite(RELAY1,LOW); pinMode(RELAY2,OUTPUT); digitalWrite(RELAY2,LOW);
  pinMode(RELAY3,OUTPUT); digitalWrite(RELAY3,LOW); pinMode(RELAY4,OUTPUT); digitalWrite(RELAY4,LOW);
  pinMode(RELAY5,OUTPUT); digitalWrite(RELAY5,LOW); pinMode(RELAY6,OUTPUT); digitalWrite(RELAY6,LOW);
  pinMode(RELAY7,OUTPUT); digitalWrite(RELAY7,LOW); pinMode(RELAY8,OUTPUT); digitalWrite(RELAY8,LOW);
  pinMode(BATTERY_PIN, INPUT);

  // EEPROM Yükle
  EEPROM.get(ADDR_TARGET,       targetTemp);
  if(isnan(targetTemp)||targetTemp<1||targetTemp>35)       targetTemp=24.0f;
  EEPROM.get(ADDR_WATER_TARGET, targetWaterTemp);  if(isnan(targetWaterTemp)||targetWaterTemp<20||targetWaterTemp>50) targetWaterTemp=30.0f;
  EEPROM.get(ADDR_FRIDGE_TARGET,targetFridgeTemp); if(isnan(targetFridgeTemp)||targetFridgeTemp<40||targetFridgeTemp>50) targetFridgeTemp=45.0f;
  
  byte b;
  EEPROM.get(ADDR_AUTO_MODE,       b); webastoAutoMode    = (b!=0);
  EEPROM.get(ADDR_WATER_AUTO_MODE, b); waterHeaterAutoMode= (b!=0);
  EEPROM.get(ADDR_FRIDGE_AUTO_MODE,b); fridgeAutoMode     = (b!=0);
  EEPROM.get(ADDR_GAS_ALARM,       b); gasAlarmEnabled    = (b!=0);
  EEPROM.get(ADDR_RAIN_ALARM,      b); rainAlarmEnabled   = (b!=0);

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP,gateway,subnet,primaryDNS,secondaryDNS)) Serial.println("Sabit IP ayarlanamadi!");
  WiFi.begin(ssid, password);
  while (WiFi.status()!=WL_CONNECTED) { delay(500); Serial.print("."); }
  WiFi.setSleep(false);
  Serial.println("\nWiFi bağlandı: " + WiFi.localIP().toString());
  Serial.println("Bulut API: " + String(cloudApiBase));

  // ESP-NOW
  if (esp_now_init()==ESP_OK) {
    esp_now_register_recv_cb(OnDataRecv);
    memcpy(peerInfo.peer_addr,broadcastAddress,6); peerInfo.channel=0; peerInfo.encrypt=false;
    esp_now_add_peer(&peerInfo);
  }

  // OTA
  ArduinoOTA.setHostname("Karavan_Master");
  ArduinoOTA.setPassword("12345");
  ArduinoOTA.onStart([]() {
    otaActive = true;
    lastSeen2 = lastSeen3 = lastSeen4 = millis();
    Serial.println("OTA Basladi — cache korunuyor.");
  });
  ArduinoOTA.onEnd([]() {
    otaActive = false;
    lastSeen2 = lastSeen3 = lastSeen4 = millis();
    Serial.println("OTA Tamamlandi.");
  });
  ArduinoOTA.begin();
  configTime(3*3600, 0, "pool.ntp.org", "time.nist.gov");

  // Web Sunucusu
  server.on("/", []() { if(!checkAuth())return; server.send(200,"text/html",getHTML()); });
  
  // DÜZELTME: Toggle mantıkları tersine çevrildi (HIGH=Açık, LOW=Kapalı)
  server.on("/toggleMainBat",       []() { if(!checkAuth())return; stateMainBat=!stateMainBat; digitalWrite(MAIN_BATTERY_RELAY_PIN,stateMainBat?HIGH:LOW); server.send(200,"text/plain","OK"); });
  server.on("/toggleUPS",           []() { if(!checkAuth())return; stateUPS=!stateUPS; digitalWrite(UPS_RELAY_PIN,stateUPS?HIGH:LOW); server.send(200,"text/plain","OK"); });
  server.on("/toggleWebastoPower",  []() { 
    if(!checkAuth())return; 
    if (stateWebastoPower) { 
      if (webastoOn || (!firstRun && (millis() - lastTurnOffTime < protectionTime))) {
        server.send(403,"text/plain","BLOCKED");
        return;
      }
    }
    stateWebastoPower=!stateWebastoPower; 
    digitalWrite(WEBASTO_POWER_RELAY_PIN,stateWebastoPower?HIGH:LOW); 
    if(!stateWebastoPower){webastoOn=false;digitalWrite(WEBASTO_RELAY_PIN,LOW);} 
    server.send(200,"text/plain","OK"); 
  });
  server.on("/toggle",              []() { if(!checkAuth())return; if(server.hasArg("id"))toggleRelay(server.arg("id").toInt()); server.send(200,"text/plain","OK"); });
  server.on("/setRelayName",        []() { if(!checkAuth())return; if(server.hasArg("id")&&server.hasArg("name")){int id=server.arg("id").toInt();if(id>=1&&id<=20)saveRelayName(id-1,server.arg("name"));} server.send(200,"text/plain","OK"); });
  
  server.on("/allOff",              []() { if(!checkAuth())return; for(int i=0;i<20;i++)stateR[i]=false; 
    digitalWrite(RELAY1,LOW); digitalWrite(RELAY2,LOW); digitalWrite(RELAY3,LOW); digitalWrite(RELAY4,LOW); 
    digitalWrite(RELAY5,LOW); digitalWrite(RELAY6,LOW); digitalWrite(RELAY7,LOW); digitalWrite(RELAY8,LOW); 
    updateSlaves(); server.send(200,"text/plain","OK"); 
  });
  
  server.on("/setTarget",           []() { if(!checkAuth())return; if(server.hasArg("val")){targetTemp+=server.arg("val").toInt();targetTemp=constrain(targetTemp,1,35);EEPROM.put(ADDR_TARGET,targetTemp);EEPROM.commit();} server.send(200,"text/plain","OK"); });
  server.on("/setWaterTarget",      []() { if(!checkAuth())return; if(server.hasArg("val")){targetWaterTemp+=server.arg("val").toInt();targetWaterTemp=constrain(targetWaterTemp,20,50);EEPROM.put(ADDR_WATER_TARGET,targetWaterTemp);EEPROM.commit();} server.send(200,"text/plain","OK"); });
  server.on("/setFridgeTarget",     []() { if(!checkAuth())return; if(server.hasArg("val")){targetFridgeTemp+=server.arg("val").toInt();targetFridgeTemp=constrain(targetFridgeTemp,40,50);EEPROM.put(ADDR_FRIDGE_TARGET,targetFridgeTemp);EEPROM.commit();} server.send(200,"text/plain","OK"); });
  server.on("/toggleAuto",          []() { if(!checkAuth())return; webastoAutoMode=!webastoAutoMode; EEPROM.put(ADDR_AUTO_MODE,webastoAutoMode?1:0); EEPROM.commit(); server.send(200,"text/plain","OK"); });
  server.on("/toggleWaterAuto",     []() { if(!checkAuth())return; waterHeaterAutoMode=!waterHeaterAutoMode; EEPROM.put(ADDR_WATER_AUTO_MODE,waterHeaterAutoMode?1:0); EEPROM.commit(); server.send(200,"text/plain","OK"); });
  server.on("/toggleFridgeAuto",    []() { if(!checkAuth())return; fridgeAutoMode=!fridgeAutoMode; EEPROM.put(ADDR_FRIDGE_AUTO_MODE,fridgeAutoMode?1:0); EEPROM.commit(); server.send(200,"text/plain","OK"); });
  server.on("/manualWebasto",       []() { if(!checkAuth())return; if(!stateWebastoPower){server.send(403,"text/plain","POWER_OFF");return;} triggerWebasto(); if(webastoOn){webastoOn=false;lastTurnOffTime=millis();firstRun=false;totalWebastoOnTimeMs+=(millis()-currentCycleStartTime);}else{webastoOn=true;currentCycleStartTime=millis();} server.send(200,"text/plain","OK"); });
  server.on("/manualWaterHeater",   []() { if(!checkAuth())return; waterHeaterOn=!waterHeaterOn; updateSlaves(); server.send(200,"text/plain","OK"); });
  server.on("/manualFridge",        []() { if(!checkAuth())return; fridgeFanOn=!fridgeFanOn; updateSlaves(); server.send(200,"text/plain","OK"); });
  server.on("/toggleGasAlarm",      []() { if(!checkAuth())return; gasAlarmEnabled=!gasAlarmEnabled; EEPROM.put(ADDR_GAS_ALARM,gasAlarmEnabled?1:0); EEPROM.commit(); server.send(200,"text/plain","OK"); });
  server.on("/toggleRainAlarm",     []() { if(!checkAuth())return; rainAlarmEnabled=!rainAlarmEnabled; EEPROM.put(ADDR_RAIN_ALARM,rainAlarmEnabled?1:0); EEPROM.commit(); server.send(200,"text/plain","OK"); });
  
  server.on("/status", []() {
    if(!checkAuth()) return;
    server.send(200, "application/json", buildStatusJson());
  });
  server.begin();
  Serial.println("Web sunucusu başlatıldı.");
}

// ===== LOOP =====
void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  unsigned long now = millis();

  if (now - lastMasterSync >= 25000) { lastMasterSync=now; updateSlaves(); }

  if (isSettingMode && (now-lastChangeTime>=3000)) { EEPROM.put(ADDR_TARGET,targetTemp); EEPROM.commit(); isSettingMode=false; }

  // Telegram
  if (now - lastTelegramCheck >= telegramInterval) {
    lastTelegramCheck = now;
    int n = bot.getUpdates(bot.last_message_received+1);
    while(n){ handleTelegramMessages(n); n=bot.getUpdates(bot.last_message_received+1); }
  }

  // Bulut API senkronizasyonu
  if (!otaActive && WiFi.status() == WL_CONNECTED) {
    if (now - lastCloudPoll >= cloudPollInterval) {
      lastCloudPoll = now;
      cloudPollCommands();
    }
    if (now - lastCloudPush >= cloudPushInterval) {
      lastCloudPush = now;
      cloudPushStatus();
    }
  }

  // SLAVE TIMEOUT
  if (!otaActive) {
    if (now-lastSeen2 > slaveTimeout) { live_rssi2 = -100; live_tempExtra = -99.0f; }
    if (now-lastSeen3 > slaveTimeout) { live_rssi3 = -100; }
    if (now-lastSeen4 > slaveTimeout) { live_rssi4 = -100; live_tempWater = -99.0f; live_waterLevelPercent = 0; live_waterLiters = 0; }
  } else {
    lastSeen2 = lastSeen3 = lastSeen4 = now;
  }

  // SENSÖR OKUMA
  if (now - lastSensorReadTime >= sensorInterval) {
    lastSensorReadTime = now;

    // DS18B20 İÇ ORTAM
    sensorIn.requestTemperatures();
    float tIn = sensorIn.getTempCByIndex(0);
    if (tIn != DEVICE_DISCONNECTED_C && tIn != 85.0f && tIn > -50.0f) {
      live_temperature = tIn;
      cache_temperature = tIn; 
      if (!isSettingMode) checkWebastoLogic();
    }

    // DS18B20 DIŞ ORTAM
    sensorOut.requestTemperatures();
    float tOut = sensorOut.getTempCByIndex(0);
    if (tOut != DEVICE_DISCONNECTED_C && tOut != 85.0f && tOut > -50.0f) {
      live_tempOutside  = tOut;
      cache_tempOutside = tOut;
    } else {
      live_tempOutside = -99.0f;
    }

    // MQ2 Gaz ve Siren Kontrolü (DÜZELTME: Sirenler HIGH ile çalar)
    int mq = analogRead(MQ2_PIN);
    live_mq2  = mq;
    cache_mq2 = mq; 
    if (gasAlarmEnabled && mq>1200) digitalWrite(GAS_SIREN_PIN, HIGH);
    else digitalWrite(GAS_SIREN_PIN, LOW);

    // Yağmur ve Siren Kontrolü
    int rawRain = analogRead(RAIN_PIN);
    int rp = constrain(map(rawRain,0,4095,0,100),0,100);
    live_rainPercent  = rp; cache_rainPercent = rp; 
    if (rainAlarmEnabled && rp>20) digitalWrite(RAIN_SIREN_PIN, HIGH); 
    else digitalWrite(RAIN_SIREN_PIN, LOW);

    // MPU6050
    if (mpuConnected) {
      Wire.beginTransmission(MPU_ADDR);
      Wire.write(0x3B);
      Wire.endTransmission(false);
      Wire.requestFrom((uint16_t)MPU_ADDR,(uint8_t)6,true);
      if (Wire.available()>=6) {
        int16_t AcX=Wire.read()<<8|Wire.read();
        int16_t AcY=Wire.read()<<8|Wire.read();
        int16_t AcZ=Wire.read()<<8|Wire.read();
        float p=(atan2(-AcX,sqrt((float)AcY*AcY+(float)AcZ*AcZ))*180.0f)/M_PI;
        float r=(atan2(AcY,AcZ)*180.0f)/M_PI;
        live_pitch=p; cache_pitch=p;
        live_roll=r;  cache_roll=r;
      }
    } else {
      live_pitch=0.0f; live_roll=0.0f;
    }

    // Akü
    int rawBat = analogRead(BATTERY_PIN);
    float pinV = (rawBat/4095.0f)*3.3f;
    float batV = pinV*5.0f;
    int   batP = calculateLiFePO4Percent(batV);
    live_batteryVoltage  = batV; cache_batteryVoltage  = batV;
    live_batteryPercent  = batP; cache_batteryPercent  = batP;

    checkWaterHeaterLogic();
    checkFridgeLogic();
  }
}

// ===== RÖLE TOGGLE (DÜZELTME) =====
void toggleRelay(int id) {
  if (id<1||id>20) return;
  stateR[id-1]=!stateR[id-1];
  switch(id){
    case 1:  digitalWrite(RELAY1,stateR[0]?HIGH:LOW); break;
    case 2:  digitalWrite(RELAY2,stateR[1]?HIGH:LOW); break;
    case 3:  digitalWrite(RELAY3,stateR[2]?HIGH:LOW); break;
    case 4:  digitalWrite(RELAY4,stateR[3]?HIGH:LOW); break;
    case 5:  digitalWrite(RELAY5,stateR[4]?HIGH:LOW); break;
    case 6:  digitalWrite(RELAY6,stateR[5]?HIGH:LOW); break;
    case 7:  digitalWrite(RELAY7,stateR[6]?HIGH:LOW); break;
    case 8:  digitalWrite(RELAY8,stateR[7]?HIGH:LOW); break;
    default: break;
  }
  updateSlaves();
}
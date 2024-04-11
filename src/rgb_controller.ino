_/*
  Name:        rgb_controller.ino
  Created:     28.07.2022
  Author:      Zhukov Gleb (https://github.com/zbltrz)
  Description:


   данный проект - rgb-контроллер, позволяющий управлять светодиодными лентами в различной конфигурации

  устройства в системе работают по принципу master-slave,
    где master - устройство, выходящее в интернет и работающее с Telegram.
    slave - подчиненные устройства, принимающие команды от master.
    подчиненные устройства связываются с master по протоколу ESP-NOW.

    структуры для связи между устройствами:

  struct {
    uint8_t ID; //ID устройства (розетка, выключатель или сигнализация)
    uint8_t command; //команда устройства
    char router_ssid[35];
    char router_pass[65];
    uint8_t bright; //яркость (для диммера и rgb-контроллера)
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } from_device;


    struct {
    uint8_t ID; //ID устройства (розетка, выключатель или сигнализация)
    uint8_t command; //команда устройства
    char router_ssid[35];
    char router_pass[65];
    uint8_t bright; //яркость (для диммера и rgb-контроллера)
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } to_device;

  какие команды или ответы отправляет это устройство:

  ID:
  5 - rgb контроллер

       command:
       0 - выкл
       1 - вкл
       2 - запрос к master о настройке
       3 - яркость ОК
       4 - цвет ОК

       bright:
       0-255 - общая яркость

       r:
       0-255 - яркость красного

       g:
       0-255 - яркость зеленого

       b:
       0-255 - яркость синего

*/

#define FIRMWARE_VERSION "2.2"
#define NEXT_FIRMWARE_VERSION "2.3"


#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <TimerMs.h>
#include "EncButton.h"
#include <FastLED.h>

static const char serverCACert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFMDCCBBigAwIBAgISBNG02QSCDx60y4HsGSAPu9w7MA0GCSqGSIb3DQEBCwUA
MDIxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQswCQYDVQQD
EwJSMzAeFw0yMjA1MTYwNjUwMzJaFw0yMjA4MTQwNjUwMzFaMBcxFTATBgNVBAMT
DGdyaWItdGVjaC5ydTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAOPW
qDVWrwwt9CTQncV3a/KwQdshxna2wvKCaKfwogrzrD+PsIfFJVrV91lz/8IV9/d+
uDSb63sASxvl8oPuxHKaqHxr4nqCbEOIEyyy6pd7AvBXK3aWATnf5N2l8DdBlOMi
arRbag6U3I8quIFX79RkW98jmfE+BKHrIlq4HMIjaLHuh714wLLv5a1qUu/Nz0bC
cE6dxjKAlPczJDL8DM7U73S7F8TVJlKuFLNd2zJhY1jbBEo+n8UKR3wrnoUDrGlb
rqRJa2mSDnpKKd3WkU0fn8Dr8Y1JSZEYNuOW0UGeHH4dmG9EnQFYFZpxFzBBQB8a
kvDb954UL3TVONiDsHcCAwEAAaOCAlkwggJVMA4GA1UdDwEB/wQEAwIFoDAdBgNV
HSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDAYDVR0TAQH/BAIwADAdBgNVHQ4E
FgQUBX2HRLeU1UrzD/mPmxd+eMKhccQwHwYDVR0jBBgwFoAUFC6zF7dYVsuuUAlA
5h+vnYsUwsYwVQYIKwYBBQUHAQEESTBHMCEGCCsGAQUFBzABhhVodHRwOi8vcjMu
by5sZW5jci5vcmcwIgYIKwYBBQUHMAKGFmh0dHA6Ly9yMy5pLmxlbmNyLm9yZy8w
KQYDVR0RBCIwIIIMZ3JpYi10ZWNoLnJ1ghB3d3cuZ3JpYi10ZWNoLnJ1MEwGA1Ud
IARFMEMwCAYGZ4EMAQIBMDcGCysGAQQBgt8TAQEBMCgwJgYIKwYBBQUHAgEWGmh0
dHA6Ly9jcHMubGV0c2VuY3J5cHQub3JnMIIBBAYKKwYBBAHWeQIEAgSB9QSB8gDw
AHYAQcjKsd8iRkoQxqE6CUKHXk4xixsD6+tLx2jwkGKWBvYAAAGAy9juEQAABAMA
RzBFAiEA9y6HoJ1Wu9W9Xo2hnAlWnvxKaV3YkIEZn7gO9xFjO8oCIDmH4k3mgNCh
+gylONt8hgCz7w6YVJr1IBDrPkq4HPwMAHYARqVV63X6kSAwtaKJafTzfREsQXS+
/Um4havy/HD+bUcAAAGAy9juIAAABAMARzBFAiBVDQAKe5sokZM0gkAezyfhD9RJ
qFrtUS6idpATyAZ+MQIhAKp3Q1XGm5DdYxrxE+1zXafCW6q/rXUA7xO1/r06SviF
MA0GCSqGSIb3DQEBCwUAA4IBAQA/DsuHgVbU//0KezIcctG+z+UM3v0YyZYK7awx
zZolPTiBQhCIAxbPVxBtbRnTkJllZ55fTFTLVoVR6HgH74qi22tBHo3Yp9LRm/M4
i7qXLiH1ZyvPVaFHNhDtn+L4BrLIZqap+nrcn3hV7XEZjcGQaaMogBHH7sw3zugN
wqtotE1YWMARhmil6vM8F1V9RWkq0OV1K0emLSmvXGIuK1T173BSkdJ0/2H7gs0W
ygujuoyEjQyQd2WCZ+eZZw9BN98lsR18gOt9fcQE+nN44+4HjWgBDCiuFwTlvOJ/
xtrpyKpz/BNmYBm2wtuug6nbeqzg6oLHTDqlCpzO/jLjZaZt
-----END CERTIFICATE-----
)EOF";



EncButton<EB_TICK, 4> butt(INPUT_PULLUP); //для работы кнопки
////////////////////WIFI////////////////////

#define WIFI_SSID "grib socket" //для настройки канала

////////////////////TIME////////////////////
TimerMs tmr_check_channel(1000*60*5, 1, 0); //для синхронизации канала
TimerMs tmr_wi_fi(1000*60*60, 1, 0); //для периодического включения wi-fi, чтобы проверить доступность обновления прошивки


////////////////////LED////////////////////

// настройки пламени
#define HUE_GAP 21      // заброс по hue
#define FIRE_STEP 15    // шаг огня
#define HUE_START 0     // начальный цвет огня (0 красный, 80 зелёный, 140 молния, 190 розовый)
#define MIN_BRIGHT 70   // мин. яркость огня
#define MAX_BRIGHT 255  // макс. яркость огня
#define MIN_SAT 245     // мин. насыщенность
#define MAX_SAT 255     // макс. насыщенность

int counter = 0;

#define FOR_i(from, to) for(int i = (from); i < (to); i++)

#define LED_PIN     6
#define COLOR_ORDER GRB
#define CHIPSET     WS2812
#define NUM_LEDS    30


bool gReverseDirection = false;

CRGB leds[NUM_LEDS];

bool set_channel_flag; //для настройки канала (slave)


struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t bright;
  uint8_t strip_mode; //0 - off, 1 - solid, 2 - fire
} for_eeprom;

void setup() {

  //Serial.begin(115200);
  //Serial.println();


  configTime("MSK-3MSD-3", "time.google.com", "time.windows.com", "pool.ntp.org");


  EEPROM.begin(4096);
  EEPROM.get(2, for_eeprom);


  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  FastLED.setBrightness(for_eeprom.bright);

  if (for_eeprom.strip_mode == 1) {
    fill_solid( leds, NUM_LEDS, CRGB(for_eeprom.r, for_eeprom.g, for_eeprom.b));
    FastLED.show();
  }

  if (for_eeprom.strip_mode == 0) {
    FastLED.clear();
    FastLED.show();
  }


  set_channel();

  if (esp_now_init() != 0) {
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(data_recv);
  esp_now_register_send_cb(data_sent);

  char ssid[35];
  char pass[65];
  EEPROM.get(3430, ssid);
  EEPROM.get(3467, pass);
  WiFi.begin(ssid, pass);


  butt.setHoldTimeout(3000);
}




void loop() {

  butt.tick(); //обработка кнопки

  if (tmr_check_channel.tick()) { //если сработал таймер проверки канала

    uint8_t broadcast_address[6];

    EEPROM.get(150, broadcast_address);

    send_packet(broadcast_address, 7); //отправляем пакет c несуществующей командой, проверяем дошёл ли он в void data_send

  }

          if (set_channel_flag){
          set_channel_flag = false;
          set_channel();
        }

  if (tmr_wi_fi.tick()) {
    char ssid[35];
    char pass[65];
    EEPROM.get(3430, ssid);
    EEPROM.get(3467, pass);
    WiFi.begin(ssid, pass);
  }

  if (WiFi.status() == WL_CONNECTED) {
    //Serial.println("update");
    //Serial.println(setClock());
    update_firm();
  }

  if (for_eeprom.strip_mode == 2) {
    fire_tick();
  }

  if (butt.held()) { //если кнопка была зажата

    uint8_t broadcast_address[6];

    for (int i = 0; i < 6; i++) {
      broadcast_address[i] = 0xFF;
    }

    send_packet(broadcast_address, 2);

  }
}

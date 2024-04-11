int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i = 0; i < n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}



void set_channel() {

  int32_t channel;
  channel = getWiFiChannel(WIFI_SSID);

  if (channel >= 1) {
    ////Serial.print("Настраиваем основной канал: ");
    ////Serial.println(channel);
    wifi_promiscuous_enable(1);
    wifi_set_channel(channel);
    wifi_promiscuous_enable(0);
  }

  else {

    char ssid[35];
    EEPROM.get(3430, ssid);
    channel = getWiFiChannel(ssid);
    if (channel >= 1) {
      ////Serial.print("Настраиваем второстепенный канал: ");
      ////Serial.println(channel);
      wifi_promiscuous_enable(1);
      wifi_set_channel(channel);
      wifi_promiscuous_enable(0);
    }
  }
}

void update_firm() {

  if (setClock() == 2) {

    BearSSL::WiFiClientSecure client;

    BearSSL::X509List x509(serverCACert);
    client.setTrustAnchors(&x509);

    setClock();

    ESPhttpUpdate.rebootOnUpdate(false); // remove automatic update

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, "https://www.grib-tech.ru/update/rgb_controller_v" + String(NEXT_FIRMWARE_VERSION) + ".bin");

    switch (ret) {
      case HTTP_UPDATE_FAILED:

        ////Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        WiFi.disconnect();

        break;
      case HTTP_UPDATE_NO_UPDATES:
        WiFi.disconnect();
        break;
      case HTTP_UPDATE_OK:

        delay(1000);
        ESP.restart();

        break;
    }
  }
  else {
    WiFi.disconnect();
  }
}



uint8_t setClock() {

  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    yield();
    now = time(nullptr);
  }
  struct tm * timeinfo;
  timeinfo = localtime(&now);

  uint8_t hour = timeinfo->tm_hour;

  return hour;
}

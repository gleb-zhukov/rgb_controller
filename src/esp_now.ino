//функция при отправке данных
void data_sent(uint8_t *mac_addr, uint8_t send_status) {
  if (send_status != 0) {

    //Serial.println("Пакет не отправлен, настраиваем канал");
set_channel_flag = true;
  }
}


void send_packet(uint8_t *func_mac_addr, uint8_t func_command) { //выключателю
  struct {
    uint8_t ID; //ID устройства (розетка, выключатель, датчик и т.д.)
    uint8_t command; //команда устройству
  } to_device;
  to_device.ID = 5; //от rgb контроллера
  to_device.command = func_command;

  esp_now_send(func_mac_addr, (uint8_t *) &to_device, sizeof(to_device));

}

void data_recv(uint8_t * mac_addr, uint8_t *incomingData, uint8_t len) {


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


  memcpy(&from_device, incomingData, sizeof(from_device));

  if (from_device.ID == 0) { //сообщение от master

    //записываем в EEPROM ssid и пароль wifi с выходом в интернет
    //put сравнивает записанные данные с новыми, и если они не отличаются, перезапись не выполняется
    EEPROM.put(3467, from_device.router_pass);
    EEPROM.put(3430, from_device.router_ssid);
    EEPROM.put(150, mac_addr); //mac адрес master устройства
    EEPROM.commit();

    if (from_device.command == 4) {//запрос на включение
      for_eeprom.strip_mode = 1; //solid mode
      EEPROM.put(2, for_eeprom);
      EEPROM.commit();
      fill_solid(leds, NUM_LEDS, CRGB(for_eeprom.r, for_eeprom.g, for_eeprom.b));
      FastLED.setBrightness(for_eeprom.bright);
      FastLED.show();

      send_packet(mac_addr, 1); //ответ включено

    }
    else if (from_device.command == 5) {//запрос на выключение
      for_eeprom.strip_mode = 0; //off
      EEPROM.put(2, for_eeprom);
      EEPROM.commit();

      FastLED.clear();
      FastLED.show();
      send_packet(mac_addr, 0); //ответ выключено
    }
    else if (from_device.command == 6) {//запрос на настройку яркости
      uint8_t bright = map(from_device.bright, 0, 100, 0, 255); //конвертируем

      for_eeprom.bright = bright;
      EEPROM.put(2, for_eeprom);
      EEPROM.commit();

      if ((for_eeprom.strip_mode == 1) || (for_eeprom.strip_mode == 0)) { //если выключено или обычный режим
        fill_solid(leds, NUM_LEDS, CRGB(for_eeprom.r, for_eeprom.g, for_eeprom.b));
        FastLED.setBrightness(for_eeprom.bright);
        FastLED.show();
      }
      else {
        FastLED.setBrightness(for_eeprom.bright);
      }

      send_packet(mac_addr, 3); //ответ яркость ОК
    }
    else if (from_device.command == 7) {//запрос на настройку цвета
      for_eeprom.strip_mode = 1; //solid mode
      for_eeprom.r = from_device.r;
      for_eeprom.g = from_device.g;
      for_eeprom.b = from_device.b;
      EEPROM.put(2, for_eeprom);
      EEPROM.commit();

      fill_solid(leds, NUM_LEDS, CRGB(for_eeprom.r, for_eeprom.g, for_eeprom.b));
      FastLED.show();
      send_packet(mac_addr, 4); //ответ цвет ОК
    }
    else if (from_device.command == 8) {//запрос на режим огня
      for_eeprom.strip_mode = 2; //fire mode;
      EEPROM.put(2, for_eeprom);
      EEPROM.commit();
      send_packet(mac_addr, 4); //ответ цвет ОК
    }
  }
}

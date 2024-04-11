void fire_tick() {
  static uint32_t prevTime;

  // двигаем пламя
  if (millis() - prevTime > 20) {
    prevTime = millis();
    FOR_i(0, NUM_LEDS) {
      leds[i] = getFireColor((inoise8(i * FIRE_STEP, counter)));
    }
    counter += 20;
    FastLED.show();
  }
}

// возвращает цвет огня для одного пикселя
CHSV getFireColor(int val) {
  // чем больше val, тем сильнее сдвигается цвет, падает насыщеность и растёт яркость
  return CHSV(
           HUE_START + map(val, 0, 255, 0, HUE_GAP), // H
           constrain(map(val, 0, 255, MAX_SAT, MIN_SAT), 0, 255), // S
           constrain(map(val, 0, 255, MIN_BRIGHT, MAX_BRIGHT), 0, 255) // V
         );
}

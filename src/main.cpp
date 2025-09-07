#include <Arduino.h>
#include "M5Cardputer.h"

void setup() {
  auto cfg = M5.config();
  Serial.begin(115200);
  Serial.print(5);
  M5Cardputer.begin(cfg);

  while (1) {
    M5Cardputer.update();
  }
}

void loop() {
}
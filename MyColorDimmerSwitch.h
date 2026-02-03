#pragma once

#include <Arduino.h>
#include "Zigbee.h"

class MyColorDimmerSwitch : public ZigbeeColorDimmerSwitch {
 public:
  explicit MyColorDimmerSwitch(uint8_t endpoint);

  // Recall a scene (non-group): sent via bindings, group_id is 0x0000.
  void recallScene(uint8_t scene_id);

  // Continuous dimming (Level Control cluster).
  // `up=true` moves up, `up=false` moves down.
  // `rate=0xFF` uses device default.
  void startDimming(bool up, uint8_t rate = 0xFF);
  void stopDimming();
};

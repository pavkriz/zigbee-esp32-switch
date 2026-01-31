#pragma once

#include <Arduino.h>
#include "Zigbee.h"

class MyColorDimmerSwitch : public ZigbeeColorDimmerSwitch {
 public:
  using ZigbeeColorDimmerSwitch::ZigbeeColorDimmerSwitch;
};

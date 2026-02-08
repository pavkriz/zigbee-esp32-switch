#pragma once

#include <Arduino.h>
#include "Zigbee.h"
#include "PatchedZigbeeColorDimmerSwitch.h"

class MyColorDimmerSwitch : public PatchedZigbeeColorDimmerSwitch {
 public:
  explicit MyColorDimmerSwitch(uint8_t endpoint);

  // Recall a scene (non-group): sent via bindings, group_id is 0x0000.
  void recallScene(uint8_t scene_id);

  // Continuous dimming (Level Control cluster).
  // `up=true` moves up, `up=false` moves down.
  void startDimming(bool up, uint8_t rate);
  void stopDimming();

  void onLightSceneChange(void (*callback)(int)) {
    _on_light_scene_change = callback;
  }


 protected:
  void (*_on_light_scene_change)(int);

  void zbAttributeRead(uint16_t cluster_id,
                       const esp_zb_zcl_attribute_t *attribute,
                       uint8_t src_endpoint,
                       esp_zb_zcl_addr_t src_address) override;
};

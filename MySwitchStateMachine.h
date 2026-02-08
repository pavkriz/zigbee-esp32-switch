#pragma once

#include <Arduino.h>
#include <JC_Button.h>

#include "MyColorDimmerSwitch.h"

class MySwitchStateMachine {
 public:

  MySwitchStateMachine(MyColorDimmerSwitch& sw, Button& up, Button& down);

  void begin();
  void tick();

  // Forward the Zigbee on/off attribute updates here.
  void onLightStateChange(bool on);
  void onLightSceneChange(int scene_id);
  void onLightSceneNotFound();

  void setLongPressMs(uint16_t ms);
  void setMoveRate(uint8_t rate);

 private:

  enum class DimmingDirection : uint8_t {
    None,
    Up,
    Down,
  };

  bool lightOnForDecision() const;
  int sceneForDecision() const;

  void handleShortUp(bool light_on);
  void handleShortDown(bool light_on);

  MyColorDimmerSwitch& _sw;
  Button& _up;
  Button& _down;

  uint16_t _longPressMs = 500;
  uint8_t _moveRate = 50;

  bool _expectedOn = false;
  bool _lastKnownOn = false;
  bool _lastKnownOnValid = false;
  int _expectedScene = 0;
  int _lastKnownScene = 0;
  bool _lastKnownSceneValid = false;

  bool _upPressed = false;
  bool _downPressed = false;
  uint32_t _upPressMs = 0;
  uint32_t _downPressMs = 0;

  DimmingDirection _dimming = DimmingDirection::None;
};

#include "MySwitchStateMachine.h"

MySwitchStateMachine::MySwitchStateMachine(MyColorDimmerSwitch& sw, Button& up, Button& down)
  : _sw(sw), _up(up), _down(down) {}

void MySwitchStateMachine::begin() {
  _up.begin();
  _down.begin();
}

void MySwitchStateMachine::setLongPressMs(uint16_t ms) {
  _longPressMs = ms;
}

void MySwitchStateMachine::setMoveRate(uint8_t rate) {
  _moveRate = rate;
}

void MySwitchStateMachine::onLightStateChange(bool on) {
  _lastKnownOn = on;
  _lastKnownOnValid = true;
  _expectedOn = on;
}

void MySwitchStateMachine::onLightSceneChange(int sceneId) {
  _lastKnownScene = sceneId;
  _lastKnownSceneValid = true;
  _expectedScene = sceneId;
}

bool MySwitchStateMachine::lightOnForDecision() const {
  // We rely on pushed updates, but keep _expectedOn as an optimistic fallback
  // so UX stays responsive while waiting for a report/response.
  return _lastKnownOnValid ? _lastKnownOn : _expectedOn;
}

int MySwitchStateMachine::sceneForDecision() const {
  return _lastKnownSceneValid ? _lastKnownScene : _expectedScene;
}

void MySwitchStateMachine::handleShortUp(bool light_on) {
  // When off: short up recalls scene 2.
  if (!light_on) {
    _sw.recallScene(2);
    _expectedScene = 2;
    _expectedOn = true;
  }
  // When on: short up cycles through scenes 2 and 3
  else {
    int to_be_scene = sceneForDecision() + 1;
    _sw.recallScene(to_be_scene);
    _expectedScene = to_be_scene;
    _expectedOn = true;
  }
}

void MySwitchStateMachine::handleShortDown(bool light_on) {
  if (!light_on) {
    // When off: short down recalls scene 1.
    _sw.recallScene(1);
    _expectedScene = 1;
    _expectedOn = true;
    return;
  }

  // When on: short down turns off.
  _sw.lightOff();
  _expectedOn = false;
}

void MySwitchStateMachine::tick() {
  const uint32_t now_ms = millis();

  _up.read();
  _down.read();

  const bool upWasPressed = _up.wasPressed();
  const bool upWasReleased = _up.wasReleased();
  const bool upIsPressed = _up.isPressed();

  const bool downWasPressed = _down.wasPressed();
  const bool downWasReleased = _down.wasReleased();
  const bool downIsPressed = _down.isPressed();

  // Track button edges.
  if (upWasPressed) {
    _upPressed = true;
    _upPressMs = now_ms;
  }
  if (downWasPressed) {
    _downPressed = true;
    _downPressMs = now_ms;
  }

  // Start dimming on long press (only when light is considered ON).
  const bool light_on_for_hold = lightOnForDecision();

  if (_upPressed && upIsPressed && _dimming == DimmingDirection::None) {
    if (now_ms - _upPressMs >= _longPressMs) {
      if (light_on_for_hold) {
        _dimming = DimmingDirection::Up;
        _sw.startDimming(true, _moveRate);
      }
    }
  }

  if (_downPressed && downIsPressed && _dimming == DimmingDirection::None) {
    if (now_ms - _downPressMs >= _longPressMs) {
      if (light_on_for_hold) {
        _dimming = DimmingDirection::Down;
        _sw.startDimming(false, _moveRate);
      }
    }
  }

  // If the opposite button is pressed while dimming, stop first.
  if (_dimming == DimmingDirection::Up && downWasPressed) {
    _sw.stopDimming();
    _dimming = DimmingDirection::None;
  }
  if (_dimming == DimmingDirection::Down && upWasPressed) {
    _sw.stopDimming();
    _dimming = DimmingDirection::None;
  }

  // Stop dimming on release.
  if (upWasReleased) {
    const bool wasLong = _upPressed && (now_ms - _upPressMs >= _longPressMs);
    _upPressed = false;

    if (_dimming == DimmingDirection::Up) {
      _sw.stopDimming();
      _dimming = DimmingDirection::None;
    } else if (!wasLong) {
      handleShortUp(lightOnForDecision());
    }
  }

  if (downWasReleased) {
    const bool wasLong = _downPressed && (now_ms - _downPressMs >= _longPressMs);
    _downPressed = false;

    if (_dimming == DimmingDirection::Down) {
      _sw.stopDimming();
      _dimming = DimmingDirection::None;
    } else if (!wasLong) {
      handleShortDown(lightOnForDecision());
    }
  }
}


/*

9:39:40.528 -> [ 63882][V][ZigbeeHandlers.cpp:458] zb_cmd_default_resp_handler(): Received default response: from address(0x3fe9), src_endpoint(1) to dst_endpoint(5), cluster(0x5) with status 0x8b
09:39:40.528 -> [ 63884][V][ZigbeeEP.cpp:640] zbDefaultResponse(): Default response received for endpoint 5
09:39:40.528 -> [ 63885][V][ZigbeeEP.cpp:641] zbDefaultResponse(): Status code: Not found
09:39:40.528 -> [ 63886][V][ZigbeeEP.cpp:642] zbDefaultResponse(): Response to command: 5
09:39:40.551 -> Zigbee default response: resp_to_cmd=5 status=0x8B
*/
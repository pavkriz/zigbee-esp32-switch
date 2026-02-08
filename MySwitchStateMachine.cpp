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

void MySwitchStateMachine::onLightSceneNotFound() {
  // leave _lastKnownScene unchanged, since the scene actually didn't change on the light
  // start "cycle to scene 2 fallback" process when scene not found and scene more than 2 was recalled
  if (_expectedScene > 2) {
    Serial.println("Scene not found, falling back to scene 2");
    _sw.recallScene(2);
    _expectedScene = 2;
  } else {
    // else we have recalled scene 2 or 1 and it was not found,
    // which means the light likely doesn't have scenes configured, so we can just turn it on as a fallback
    Serial.println("Scene not found, turning on the light as a fallback");
    _sw.lightOn();
  }
  _expectedOn = true;
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
  }  else {
    // When on: short up cycles through scenes 2, 3, 4, ..., 2.
    // If some scene does not exist, it falls back to scene 2, and then cycles from there; see onLightSceneNotFound().
    // When cycling, be optimistic (use _expectedScene as current scene) and do not wait for the light to report the scene change, 
    // to keep UX responsive (otherwise, some short presses may look be "missing" since we do not have a new scene reported yet).
    // The onLightSceneNotFound (which does the cycle back to scene 2) response to command is much faster than waiting for the scene change report, 
    // so cycling will work well.
    int to_be_scene = _expectedScene + 1;
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

  const bool light_on_for_hold = lightOnForDecision();

  // Button up shortcut to turn the light on to scene 2 immediatelly when it's off 
  // (do dot distinguish short/long press in this case, just turn on on press), to improve responsiveness.
  if (upWasPressed && !light_on_for_hold) {
    handleShortUp(light_on_for_hold);
    _upPressed = false; // don't start long press dimming after this
    _shortUpShortcutInProgress = true; // used to ignore the release event of this press
  }

  // Start dimming on long press (only when light is considered ON).
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
    } else if (!wasLong && !_shortUpShortcutInProgress) {
      handleShortUp(lightOnForDecision());
    }
    _shortUpShortcutInProgress = false; // reset the flag after handling the release
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

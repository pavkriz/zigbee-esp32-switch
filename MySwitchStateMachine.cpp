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
  _lastKnownValid = true;
  _expectedOn = on;
}
bool MySwitchStateMachine::lightOnForDecision() const {
  // We rely on pushed updates, but keep _expectedOn as an optimistic fallback
  // so UX stays responsive while waiting for a report/response.
  return _lastKnownValid ? _lastKnownOn : _expectedOn;
}

void MySwitchStateMachine::handleShortUp(bool light_on) {
  // When off: short up recalls scene 1.
  if (!light_on) {
    _sw.recallScene(1);
    _expectedOn = true;
  }
  // When on: short up does nothing.
}

void MySwitchStateMachine::handleShortDown(bool light_on) {
  if (!light_on) {
    // When off: short down recalls scene 2.
    _sw.recallScene(2);
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

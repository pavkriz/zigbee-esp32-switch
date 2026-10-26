// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @brief This example demonstrates Zigbee color dimmer switch.
 *
 * The example demonstrates how to use Zigbee library to control a RGB light bulb.
 * The RGB light bulb is a Zigbee end device, which is controlled by a Zigbee coordinator (Switch).
 * To turn on/off the light, push the button on the switch.
 * To change the color or level of the light, send serial commands to the switch.
 *
 * By setting the switch to allow multiple binding, so it can bind to multiple lights.
 * Also every 30 seconds, all bound lights are printed to the serial console.
 *
 * Proper Zigbee mode must be selected in Tools->Zigbee mode
 * and also the correct partition scheme must be selected in Tools->Partition Scheme.
 *
 * Please check the README.md for instructions and more detailed description.
 *
 * Created by Jan Procházka (https://github.com/P-R-O-C-H-Y/)
 */

#ifndef ZIGBEE_MODE_ZCZR
#error "Zigbee coordinator mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"
#include <JC_Button.h>              // https://github.com/JChristensen/JC_Button

#include "MyColorDimmerSwitch.h"
#include "MySwitchStateMachine.h"

/* Zigbee color dimmer switch configuration */
#define SWITCH_ENDPOINT_NUMBER 1
// RGB LED, 
//    the order is GREEN, RED, BLUE on Waweshare Zero board, 
//    and RED, GREEN, BLUE on Super Mini board.
//#define IS_WAWESHARE_ZERO_BOARD // comment out the above line if you are using Super Mini board, and uncomment it if you are using Waweshare Zero board
#ifdef IS_WAWESHARE_ZERO_BOARD
#define myNeopixelWrite(pin, r, g, b) neopixelWrite(pin, g, r, b)
#else
#define myNeopixelWrite(pin, r, g, b) neopixelWrite(pin, r, g, b)
#endif
#define WS2812_PIN 8
#define FACTORY_RESET_LONG_PRESS_MS 5000
uint8_t levelToBeSet = 0;

// BOOT button for factory reset (long press)
Button btnBoot(BOOT_PIN);
bool factoryResetArmed = false;
bool wasConnected = false;

// define user-controlled buttons
Button btnUp(11);
Button btnDown(10);

/* Zigbee switch */
MyColorDimmerSwitch zbSwitch = MyColorDimmerSwitch(SWITCH_ENDPOINT_NUMBER);
MySwitchStateMachine switchStateMachine = MySwitchStateMachine(zbSwitch, btnUp, btnDown);

static void onZigbeeDefaultResponse(zb_cmd_type_t resp_to_cmd, esp_zb_zcl_status_t status) {
  Serial.printf("Zigbee default response: resp_to_cmd=%d status=0x%02X\n",
                (int)resp_to_cmd,
                (unsigned int)status);
}

static void onLightStateChange(bool state) {
  Serial.printf("Light state changed to %d\r\n", state);
  switchStateMachine.onLightStateChange(state);
}

static void onLightSceneChange(int scene_id) {
  Serial.printf("Light scene changed to %d\r\n", scene_id);
  switchStateMachine.onLightSceneChange(scene_id);
}

static void onLightSceneNotFound() {
  Serial.println("Scene not found on the light!");
  switchStateMachine.onLightSceneNotFound();
}

void identify(uint16_t time) {
  static uint8_t blink = 1;
  log_d("Identify called for %d seconds", time);
  if (time == 0) {
    // If identify time is 0, stop blinking
    myNeopixelWrite(WS2812_PIN, 0, 0, 0);
    blink = 1;  // next time identify is called, it should start with LED on
    return;
  }
  myNeopixelWrite(WS2812_PIN, 0, 255 * blink, 0);  // toggle green LED on and off to indicate identify
  blink = !blink;
}

/********************* Arduino functions **************************/
void setup() {
  Serial.begin(115200);

  // Ensure WS2812 LED is off at startup
  myNeopixelWrite(WS2812_PIN, 0, 0, 0);

  // rapidly blink the LED to indicate startup
  for (int i = 0; i < 5; i++) {
    myNeopixelWrite(WS2812_PIN, 64, 64, 64);
    delay(100);
    myNeopixelWrite(WS2812_PIN, 0, 0, 0);
    delay(100);
  }

  myNeopixelWrite(WS2812_PIN, 64, 64, 64);  // leave the led white

  // Init BOOT button for factory reset
  btnBoot.begin();

  //Optional: set Zigbee device name and model
  zbSwitch.setManufacturerAndModel("Espressif", "ZigbeeSwitchDim");
  zbSwitch.setPowerSource(ZB_POWER_SOURCE_MAINS);

  zbSwitch.onIdentify(identify);
  zbSwitch.onDefaultResponse(onZigbeeDefaultResponse);
  zbSwitch.onLightStateChange(onLightStateChange);
  zbSwitch.onLightSceneChange(onLightSceneChange);
  zbSwitch.onLightSceneNotFound(onLightSceneNotFound);

  switchStateMachine.begin();

  zbSwitch.setManualBinding(true);  //Set manual binding to true, so binding is done on Home Assistant side

  //Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbSwitch);

  //When all EPs are registered, start Zigbee with ZIGBEE_ROUTER mode
  if (!Zigbee.begin(ZIGBEE_ROUTER)) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  }

  Serial.println("Waiting for network connection...");


  bool connected = Zigbee.connected();
  if (!connected) {
    myNeopixelWrite(WS2812_PIN, 32, 128, 0);  // Yellow = not connected
  }
}

void loop() {
  // Check Zigbee network connection and show yellow LED when disconnected
  bool connected = Zigbee.connected();
  if (!connected && !factoryResetArmed && wasConnected) {
    myNeopixelWrite(WS2812_PIN, 32, 128, 0);  // Yellow = not connected
  } else if (connected && !wasConnected && !factoryResetArmed) {
    // Just (re)connected — turn off the yellow LED
    myNeopixelWrite(WS2812_PIN, 0, 0, 0);
    Serial.println("Connected to Zigbee network");
  }
  wasConnected = connected;

  // Only process user button inputs when connected to a Zigbee network
  if (connected) {
    switchStateMachine.tick();
  }


  // Handle BOOT button for factory reset (long press)
  btnBoot.read();
  if (btnBoot.isPressed() && btnBoot.pressedFor(FACTORY_RESET_LONG_PRESS_MS) && !factoryResetArmed) {
    factoryResetArmed = true;
    myNeopixelWrite(WS2812_PIN, 128, 0, 0);  // Light WS2812 red to confirm
    Serial.println("Factory reset armed! Release button to reset.");
  }
  if (btnBoot.wasReleased()) {
    if (factoryResetArmed) {
      Serial.println("Performing Zigbee factory reset...");
      myNeopixelWrite(WS2812_PIN, 0, 0, 0);  // Turn off LED
      factoryResetArmed = false;
      Zigbee.factoryReset(true);
    }
  }
  // Handle serial input to control color and level of the light
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();

    if (command == "on") {
      zbSwitch.lightOn();
    } else if (command == "off") {
      zbSwitch.lightOff();
    } else if (command == "toggle") {
      zbSwitch.lightToggle();
    } else if (command == "red") {
      zbSwitch.setLightColor(255, 0, 0);
    } else if (command == "green") {
      zbSwitch.setLightColor(0, 255, 0);
    } else if (command == "blue") {
      zbSwitch.setLightColor(0, 0, 255);
    } else if (command == "white") {
      zbSwitch.setLightColor(255, 255, 255);
    } else if (command == "color") {
      //wait for color value
      Serial.println("Enter red value (0-255):");
      while (!Serial.available()) {
        delay(100);
      }
      int red = Serial.parseInt();
      Serial.println("Enter green value (0-255):");
      while (!Serial.available()) {
        delay(100);
      }
      int green = Serial.parseInt();
      Serial.println("Enter blue value (0-255):");
      while (!Serial.available()) {
        delay(100);
      }
      int blue = Serial.parseInt();
      zbSwitch.setLightColor(red, green, blue);
    } else if (command == "level") {
      //wait for level value
      Serial.println("Enter level value (0-255):");
      while (!Serial.available()) {
        delay(100);
      }
      int level = Serial.parseInt();
      zbSwitch.setLightLevel(level);
    } else if (command == "0") {
      Serial.println("Getting light level");
      levelToBeSet = 0;
      zbSwitch.getLightState(); //
    } else if (command == "1") {
      Serial.println("Recalling scene 1");
      zbSwitch.recallScene(1);
    } else if (command == "2") {
      Serial.println("Recalling scene 2");
      zbSwitch.recallScene(2);
    } else if (command == "3") {
      levelToBeSet = 0;
      // requires mosquitto_pub -t zigbee2mqtt/<friendly_name>/set/level_config -m '{"execute_if_off": true}'
      zbSwitch.setLightLevel(1);
      delay(100);
      zbSwitch.lightToggle();
    } else if (command == "4") {
      levelToBeSet = 0;
      // requires mosquitto_pub -t zigbee2mqtt/<friendly_name>/set/level_config -m '{"execute_if_off": true}'
      zbSwitch.setLightLevel(255);
      delay(100);
      zbSwitch.lightToggle();
    } else {
      Serial.println("Unknown command");
    }
  }

  // Blink LED blue every 1 second
  static uint32_t last_blink = 0;
  static bool blink_state = false;
  if (millis() - last_blink > 1000) {
    last_blink = millis();
    blink_state = !blink_state;
    myNeopixelWrite(WS2812_PIN, 0, 0, blink_state ? 1 : 0);
  }

  // print the bound devices (lights) every 30 seconds
  static uint32_t last_print = 0;
  if (millis() - last_print > 30000) {
    last_print = millis();
    zbSwitch.printBoundDevices(Serial);
  }
}

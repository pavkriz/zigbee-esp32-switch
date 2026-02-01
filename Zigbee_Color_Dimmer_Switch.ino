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

#include "MyColorDimmerSwitch.h"

/* Zigbee color dimmer switch configuration */
#define SWITCH_ENDPOINT_NUMBER 5
uint8_t button = BOOT_PIN;
uint8_t levelToBeSet = 0;

/* Zigbee switch */
MyColorDimmerSwitch zbSwitch = MyColorDimmerSwitch(SWITCH_ENDPOINT_NUMBER);

static void onZigbeeDefaultResponse(zb_cmd_type_t resp_to_cmd, esp_zb_zcl_status_t status) {
  Serial.printf("Zigbee default response: resp_to_cmd=%d status=0x%02X\n",
                (int)resp_to_cmd,
                (unsigned int)status);
}

static void onLightStateChange(bool state) {
  Serial.printf("Light state %d\r\n", state);
}

/********************* Arduino functions **************************/
void setup() {
  Serial.begin(115200);

  //Init button switch
  pinMode(button, INPUT_PULLUP);

  //Optional: set Zigbee device name and model
  zbSwitch.setManufacturerAndModel("Espressif", "ZigbeeSwitchDim");
  zbSwitch.setPowerSource(ZB_POWER_SOURCE_MAINS);


  zbSwitch.onDefaultResponse(onZigbeeDefaultResponse);
  zbSwitch.onLightStateChange(onLightStateChange);

  zbSwitch.setManualBinding(true);  //Set manual binding to true, so binding is done on Home Assistant side

  //Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbSwitch);

  //When all EPs are registered, start Zigbee with ZIGBEE_ROUTER mode
  if (!Zigbee.begin(ZIGBEE_ROUTER)) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  }

  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
}

void loop() {
  // Handle button switch in loop()
  if (digitalRead(button) == LOW) {  // Push button pressed
    // Key debounce handling
    while (digitalRead(button) == LOW) {
      delay(50);
    }
    // Toggle light
    zbSwitch.lightToggle();
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

  // print the bound devices (lights) every 30 seconds
  static uint32_t last_print = 0;
  if (millis() - last_print > 30000) {
    last_print = millis();
    zbSwitch.printBoundDevices(Serial);
  }
}

/*
esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_scenes_recall_scene_cmd_req(&recall_scene_cmd);
        esp_zb_lock_release();
        */


        /*
        
Kdyz nejsou v cilovem device sceny:
0:42:31.402 -> Recalling scene 1
20:42:31.425 -> [192832][V][ZigbeeHandlers.cpp:458] zb_cmd_default_resp_handler(): Received default response: from address(0x3fe9), src_endpoint(1) to dst_endpoint(5), cluster(0x5) with status 0x8b
20:42:31.425 -> [192834][V][ZigbeeEP.cpp:640] zbDefaultResponse(): Default response received for endpoint 5
20:42:31.425 -> [192835][V][ZigbeeEP.cpp:641] zbDefaultResponse(): Status code: Not found
20:42:31.425 -> [192836][V][ZigbeeEP.cpp:642] zbDefaultResponse(): Response to command: 5
20:42:31.425 -> Zigbee default response: resp_to_cmd=5 status=0x8B        


kdzy tam jsou:
20:44:11.728 -> Recalling scene 2
20:44:11.793 -> [293189][V][ZigbeeHandlers.cpp:458] zb_cmd_default_resp_handler(): Received default response: from address(0x3fe9), src_endpoint(1) to dst_endpoint(5), cluster(0x5) with status 0x0
20:44:11.793 -> [293191][V][ZigbeeEP.cpp:640] zbDefaultResponse(): Default response received for endpoint 5
20:44:11.793 -> [293192][V][ZigbeeEP.cpp:641] zbDefaultResponse(): Status code: Success
20:44:11.793 -> [293193][V][ZigbeeEP.cpp:642] zbDefaultResponse(): Response to command: 5
20:44:11.793 -> Zigbee default response: resp_to_cmd=5 status=0x00

Status response
20:48:06.050 -> Getting light level
20:48:06.050 -> [527481][V][ZigbeeHandlers.cpp:144] zb_cmd_read_attr_resp_handler(): Read attribute response: from address(0x3fe9) src endpoint(1) to dst endpoint(5) cluster(0x6)
20:48:06.081 -> [527483][V][ZigbeeHandlers.cpp:153] zb_cmd_read_attr_resp_handler(): Read attribute response: status(0), cluster(0x6), attribute(0x0), type(0x10), value(0)
https://github.com/espressif/arduino-esp32/blob/6cfa93f4b4e2c0e3cf492e4a45780bfbe757fcd4/libraries/Zigbee/src/ZigbeeHandlers.cpp#L154
https://github.com/espressif/arduino-esp32/blob/6cfa93f4b4e2c0e3cf492e4a45780bfbe757fcd4/libraries/Zigbee/src/ep/ZigbeeColorDimmerSwitch.cpp#L829
https://github.com/espressif/arduino-esp32/blob/6cfa93f4b4e2c0e3cf492e4a45780bfbe757fcd4/libraries/Zigbee/src/ep/ZigbeeColorDimmerSwitch.h#L81

https://github.com/espressif/arduino-esp32/blob/6cfa93f4b4e2c0e3cf492e4a45780bfbe757fcd4/libraries/Zigbee/examples/Zigbee_On_Off_Switch/Zigbee_On_Off_Switch.ino#L97C5-L103C43
// Poll light state every second
    static uint32_t lastPoll = 0;
    if (millis() - lastPoll > 1000) {
      lastPoll = millis();
      zbSwitch.getLightState();
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);

  zbSwitch.onLightStateChange(onLightStateChange);


  static void onLightStateChange(bool state) {
  if (state != light_state) {
    light_state = state;
    Serial.printf("Light state changed to %d\r\n", state);
  }
}

        */
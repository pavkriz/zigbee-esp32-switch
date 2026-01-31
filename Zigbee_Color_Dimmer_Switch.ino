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
  if (levelToBeSet == 0) {
    return;
  }         
  // delay some time
  delay(400);
  zbSwitch.setLightLevel(levelToBeSet);
  levelToBeSet = 0;
}

/********************* Arduino functions **************************/
void setup() {
  Serial.begin(115200);

  // TODO add scenes cluster to the switch
  //esp_zb_cluster_list_add_scenes_cluster(cluster_list, esp_zb_scenes_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);


  //Init button switch
  pinMode(button, INPUT_PULLUP);

  //Optional: set Zigbee device name and model
  zbSwitch.setManufacturerAndModel("Espressif", "ZigbeeSwitchDim");
  zbSwitch.setPowerSource(ZB_POWER_SOURCE_MAINS);


  zbSwitch.onDefaultResponse(onZigbeeDefaultResponse);

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
        esp_zb_zcl_scenes_recall_scene_cmd_t recall_scene_cmd;
        memset(&recall_scene_cmd, 0, sizeof(recall_scene_cmd));
        recall_scene_cmd.zcl_basic_cmd.src_endpoint = SWITCH_ENDPOINT_NUMBER;
        recall_scene_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
        recall_scene_cmd.group_id = 0x0000;
        recall_scene_cmd.scene_id = 1; // Scene ID 1
        
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_scenes_recall_scene_cmd_req(&recall_scene_cmd);
        esp_zb_lock_release();
    } else if (command == "2") {
        Serial.println("Recalling scene 2");
        esp_zb_zcl_scenes_recall_scene_cmd_t recall_scene_cmd;
        memset(&recall_scene_cmd, 0, sizeof(recall_scene_cmd));
        recall_scene_cmd.zcl_basic_cmd.src_endpoint = SWITCH_ENDPOINT_NUMBER;
        recall_scene_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
        recall_scene_cmd.group_id = 0x0000;
        recall_scene_cmd.scene_id = 2; // Scene ID 2
        
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_scenes_recall_scene_cmd_req(&recall_scene_cmd);
        esp_zb_lock_release();
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
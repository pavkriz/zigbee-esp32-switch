# Arduino-ESP32 Zigbee Dimmer Switch with Scenes Support

This project is based on [Arduino-ESP32 Zigbee Color Dimmer Switch Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/Zigbee/examples/Zigbee_Color_Dimmer_Switch).

It adds scene switching and smart control based on current light state.

It is expected the Switch is bound to a Light (genOnOff, genLevelCtrl, genScenes) via Zigbee2MQTT and vice versa, ie. the Light is bound to the Switch (genOnOff, genLevelCtrl, genScenes) for switch to get notified about the light's state (possibly change by another remote control).

It was tester with [ESP32-H2-Zero Board](https://www.waveshare.com/wiki/ESP32-H2-Zero) and IKEA STOFTMOLN light.

Press onboard BOOT button for more then 5s (wait for the red onboard LED to light up) and ther release to perform the factory reset. Then, the onboard LED will be yellow unless paired and connected to an existing Zigbee network (e.g. via Zigbee2MQTT).

Note: The project was not tested battery-powered. It is not low-power optimized in current setup. It is intended to be supplied from low-voltage or main-to-low-voltage DC-DC power supply.

## Control logic

| Button vs. State   | Light is Off         | Light is On          |
|--------------------|----------------------|----------------------|
| **Short Up**       | Turn on to scene 2   | Cycle to next scene* |
| **Short Down**     | Turn on to scene 1   | Turn off             |
| **Long Up**        | Interpreted as short | Dim brighter         |
| **Long Down**      | Interpreted as short | Dim darker           |

*On short up press, the switch tries to recall scene ID+1 ID on the light. If the light responses with "not found" (meaning there is no scene eg. ID 4), the switch recalls the scene ID 2. This way, the switch cycles among scenes 2..N defined in the light.

Scenes IDs are meant to be like this:
1) Dark (night) mode
2) Main scene
3) Next main scene (eg. most bright)
4) Next auxiliary scene,...
5) ...

At least scenes ID 1 and 2 should be defined in the light.

## Using Arduino IDE

Before Compile/Verify:
* Install `JC_Button` library.
* Select the correct board: `Tools -> Board`; tested with `ESP32H2 Dev Module` setting.
* Select integrated USB serial: `Tools -> USB CDC On Boot: enabled`.
* Select the Coordinator Zigbee mode: `Tools -> Zigbee mode: Zigbee ZCZR (coordinator/router)`.
* Select Partition Scheme for Zigbee: `Tools -> Partition Scheme: Zigbee 4MB with spiffs`.
* Select the COM port: `Tools -> Port: xxx` where the `xxx` is the detected COM port.
* Optional: Set debug level to verbose to see all logs from Zigbee stack: `Tools -> Core Debug Level: Verbose`.
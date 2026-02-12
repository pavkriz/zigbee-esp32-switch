# Arduino-ESP32 Zigbee Dimmer Switch with Scenes Support

This project is based on [Arduino-ESP32 Zigbee Color Dimmer Switch Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/Zigbee/examples/Zigbee_Color_Dimmer_Switch).

It adds scene switching and smart control based on current light state.

It is expected you will bind the Switch to a Light (genOnOff, genLevelCtrl, genScenes) via Zigbee2MQTT and vice versa, ie. the Light will be bound to the Switch (genOnOff, genLevelCtrl, genScenes) for switch to get notified about the light's state (possible change by another remote control).

It was tested with [ESP32-H2-Zero Board](https://www.waveshare.com/wiki/ESP32-H2-Zero) and [IKEA STOFTMOLN light](https://www.zigbee2mqtt.io/devices/T2037.html#ikea-t2037) and [IKEA TRADFRI bulb E12/E14](https://www.zigbee2mqtt.io/devices/LED2101G4.html).

Press onboard BOOT button for more then 5s (wait for the red onboard LED to light up) and then release to perform the factory reset. Then, the onboard LED will be yellow unless paired and connected to an existing Zigbee network (e.g. via Zigbee2MQTT).

Note: The project was not tested battery-powered. It is not low-power optimized in current setup. It is intended to be supplied from low-voltage line or mains-line-to-low-voltage DC-DC power supply (such as those from Hi-Link Electronic). Currently it consumes approx. 40mA from 5V (baseline current, measured with ordinary multimeter; there will be higher spikes during transmission probably)

## Pinout

| Pin        | Function             |
|------------|----------------------|
| GPIO1      | Up button (to GND)   |
| GPIO2      | Down button (to GND) |
| GPIO9/BOOT | Onboard BOOT button  |
| GPIO8      | Onboard WS2812 LED   |

## Control logic

| Button vs. State   | Light is Off         | Light is On          |
|--------------------|----------------------|----------------------|
| **Short Up**       | Turn on to scene 2   | Cycle to next scene* |
| **Short Down**     | Turn on to scene 1   | Turn off             |
| **Long Up**        | Interpreted as short | Dim brighter         |
| **Long Down**      | Interpreted as short | Dim darker           |

*On short up press, the switch tries to recall scene ID+1 on the light. If the light responses with "not found" (meaning there is no scene eg. ID 4), the switch recalls the scene ID 2. This way, the switch cycles among scenes 2..N defined in the light.

Scenes IDs are meant to be like this:
1) Dark (night) mode
2) Main scene
3) Next main scene (eg. most bright)
4) Next auxiliary scene,...
5) ...

At least scenes ID 1 and 2 should be defined in the light.


## Identify

To invoke Identify command, use Device's Dev Console in Zigbee2MQTT.  
Unfortunatelly Zigbee2MQTT will not show "Identify" button on Device's Exposes tab.
Use 'Write attributes to endpoint' on Dev Console tab. Set endpoint number `1`, cluster `genIdentify`, attribute `identifyTime` and any number of seconds (eg. 5) to value. 
The device will blink onboard LED green for specified number of seconds.


## Using Arduino IDE

Before Compile/Verify:
* Install `JC_Button` library.
* Select the correct board: `Tools -> Board`; tested with `ESP32H2 Dev Module` setting.
* Select integrated USB serial: `Tools -> USB CDC On Boot: enabled`.
* Select the Coordinator Zigbee mode: `Tools -> Zigbee mode: Zigbee ZCZR (coordinator/router)`.
* Select Partition Scheme for Zigbee: `Tools -> Partition Scheme: Zigbee 4MB with spiffs`.
* Select the COM port: `Tools -> Port: xxx` where the `xxx` is the detected COM port.
* Optional: Set debug level to verbose to see all logs from Zigbee stack: `Tools -> Core Debug Level: Verbose`.

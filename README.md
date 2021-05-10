# mqtt-ultrasonic-water-level-sensor
 Water level sensor based on an esp8266.

![Breadboard](schematics/breadboard.png)

## Homebridge Support

### Installation

Follow official instructions to install [homebridge](https://homebridge.io) or use a prebuild image. Install [homebridge-mqttthing](https://www.npmjs.com/package/homebridge-mqttthing).

```sh
npm install -g homebridge
npm install -g homebridge-mqttthing
```


### Setup

Add accessory to `config.json` of homebridge:

```json
{
    "bridge": {...},
    "accessories": [
        ...,
        {
            "type": "leakSensor",
            "name": "Water Level Sensor 01",
            "url": "broker.mqttdashboard.com",
            "logMqtt": true,
            "codec": "waterlevel.js",
            "topics": {
                "getLeakDetected": "moritzmhmk/water",
                "getWaterLevel": "moritzmhmk/water",
                "getOnline": "moritzmhmk/water"
            },
            "accessory": "mqttthing"
        }
    ],
    "platforms": [...]
}
```

Create "codec" file `waterlevel.js` in the same path as the config.json:

```js
/**
 * Waterlevel codec - reports water level via leak sensor and triggers alarm
 * when a certain level is exceeded.
 */

"use strict";

module.exports = {
  init: function ({ log, config, publish, notify }) {
    let onlineTimeout;
    return {
      properties: {
        leakDetected: {
          decode: function (message) {
            return JSON.parse(message).distance < 50;
          },
        },
        waterLevel: {
          decode: function (message) {
            return percent(JSON.parse(message).distance, 100, 25);
          },
        },
        online: {
          decode: function (message, info, output) {
            if (onlineTimeout !== undefined) {
              clearTimeout(onlineTimeout);
            }
            // when no message was received for 2.5h set offline
            onlineTimeout = setTimeout(output.bind(null, "false"), 2.5 * 60 * 60 * 1000);
            return "true"
          },
        },
      },
    };
  },
};

let percent = (v, min, max) =>
  Math.min(100, Math.max(0, ((v - min) / (max - min)) * 100));
```

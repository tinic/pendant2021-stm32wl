// Decode decodes an array of bytes into an object.
//  - fPort contains the LoRaWAN fPort number
//  - bytes is an array of bytes, e.g. [225, 230, 255, 0]
//  - variables contains the device variables e.g. {"calibration": "3.5"} (both the key / value are of type string)
// The function must return an object, e.g. {"temperature": 22.5}

function u82f(v, min, max) {
    return ((v * ( 1.0 / 255.0 ) * (max - min) ) + min);
}

function Decode(fPort, bytes, variables) {
  var c = 0;
  if (fPort == 1) {
    return {
        "systemTime" : ((bytes[c++]<<0) | (bytes[c++]<<8)),
        "effectN" : (bytes[c++]),
        "temperature" : (u82f(bytes[c++], 0.0, 50.0)),
        "humidity" : (u82f(bytes[c++], 0.0, 1.0)),
        "batteryVoltage" : (u82f(bytes[c++], 2.7, 4.2))
    };
  } else if (fPort == 2) {
    return {
        "systemTime" : ((bytes[c++]<<0) | (bytes[c++]<<8)),
        "status" : (bytes[c++]),
        "effectN" : (bytes[c++]),
        "brightness" : (u82f(bytes[c++], 0.0, 1.0)),
        "batteryVoltage" : (bytes[c] ? u82f(bytes[c++], 2.7, 4.2) : 0),
        "systemVoltage" : (bytes[c] ? u82f(bytes[c++], 2.7, 4.2) : 0),
        "vbusVoltage" : (u82f(bytes[c++], 0.0, 5.5)),
        "chargeCurrent" : (u82f(bytes[c++], 0.0, 1000.0)),
        "temperature" : (u82f(bytes[c++], 0.0, 50.0)),
        "humidity" : (u82f(bytes[c++], 0.0, 1.0))
      };
  } else if (fPort == 3) {
    return {
        "systemTime" : ((bytes[c++]<<0) | (bytes[c++]<<8)),
        "status" : (bytes[c++]),
        "effectN" : (bytes[c++]),
        "brightness" : (u82f(bytes[c++], 0.0, 1.0)),
        "batteryVoltage" : (bytes[c] ? u82f(bytes[c++], 2.7, 4.2) : 0),
        "systemVoltage" : (bytes[c] ? u82f(bytes[c++], 2.7, 4.2) : 0),
        "vbusVoltage" : (u82f(bytes[c++], 0.0, 5.5)),
        "chargeCurrent" : (u82f(bytes[c++], 0.0, 1000.0)),
        "temperature" : (u82f(bytes[c++], 0.0, 50.0)),
        "humidity" : (u82f(bytes[c++], 0.0, 1.0)),
        "ringColor" : { "r" : (bytes[c++]),
                        "g" : (bytes[c++]),
                        "b" : (bytes[c++]),
                        "a" : (bytes[c++]) },
        "birdColor" : { "r" : (bytes[c++]),
                        "g" : (bytes[c++]),
                        "b" : (bytes[c++]),
                        "a" : (bytes[c++]) },
        "switch1Count" : ((bytes[c++]<<0) | (bytes[c++]<<8)),
        "switch2Count" : ((bytes[c++]<<0) | (bytes[c++]<<8)),
        "switch3Count" : ((bytes[c++]<<0) | (bytes[c++]<<8)),
        "bootCount" : ((bytes[c++]<<0) | (bytes[c++]<<8)),
        "intCount" : ((bytes[c++]<<0) | (bytes[c++]<<8)),
        "dselCount" : ((bytes[c++]<<0) | (bytes[c++]<<8))
    };
  }
  return {};
}

console.log(Decode(2,[0x30, 0x01, 0x36, 0x00, 0x19, 0xdd, 0xdd, 0xe7, 0x72, 0x8c, 0x4c]));

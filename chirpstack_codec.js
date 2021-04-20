// Decode decodes an array of bytes into an object.
//  - fPort contains the LoRaWAN fPort number
//  - bytes is an array of bytes, e.g. [225, 230, 255, 0]
//  - variables contains the device variables e.g. {"calibration": "3.5"} (both the key / value are of type string)
// The function must return an object, e.g. {"temperature": 22.5}

function u82f(v, min, max) {
    return ((v * ( 1.0 / 255.0 ) * (max - min) ) + min);
}

function Decode(fPort, bytes, variables) {
  if (fPort == 1) {
    var systemTime = (bytes[0]<<0) | (bytes[1]<<8);
    var effectN = bytes[2];
    var temperature = u82f(bytes[3], 0.0, 50.0);
    var humidity = u82f(bytes[4], 0.0, 1.0);
    var batteryVoltage = u82f(bytes[5], 2.7, 4.2);
    return {
      "systemTime" : systemTime,
      "effectN" : effectN,
      "temperature" : temperature,
      "humidity" : humidity,
      "batteryVoltage" : batteryVoltage
    };
  }
  if (fPort == 2) {
    var systemTime = (bytes[0]<<0) | (bytes[1]<<8);
    var status = bytes[2];
    var effectN = bytes[3];
    var brightness = u82f(bytes[4], 0.0, 1.0);
    var batteryVoltage = u82f(bytes[5], 2.7, 4.2);
    var systemVoltage = u82f(bytes[6], 2.7, 4.2);
    var vbusVoltage = u82f(bytes[7], 0.0, 5.5);
    var chargeCurrent = u82f(bytes[8], 0.0, 1000.0);
    var temperature = u82f(bytes[9], 0.0, 50.0);
    var humidity = u82f(bytes[10], 0.0, 1.0);
    return {
      "systemTime" : systemTime,
      "status" : status,
      "effectN" : effectN,
      "brightness" : brightness,
      "batteryVoltage" : batteryVoltage,
      "systemVoltage" : systemVoltage,
      "vbusVoltage" : vbusVoltage,
      "chargeCurrent" : chargeCurrent,
      "temperature" : temperature,
      "humidity" : humidity
    };
  }
  return {};
}

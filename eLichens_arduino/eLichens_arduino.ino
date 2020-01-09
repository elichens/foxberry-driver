#include <SoftwareSerial.h>

#include "ELICHENS_driver.h"


#define SENSOR_SERIAL_TX_PIN (6)
#define SENSOR_SERIAL_RX_PIN (7)


SoftwareSerial sensorSerial (SENSOR_SERIAL_RX_PIN, SENSOR_SERIAL_TX_PIN);

// Prototype for our UART communication
ELCOM_errorCode_t el_uartTransmit(uint8_t *data, uint16_t size);
ELCOM_errorCode_t el_uartReceive(uint8_t *data);
ELCOM_errorCode_t el_uartWaitUntilReceived(void);
void el_uartAbortReceive(void);

// Define our sensor
ELICHENS_Sensor_t sensor;


void setup() {
  ELCOM_errorCode_t error_code;
  char str[24];
  uint32_t sn;
  
  // Logging
  Serial.begin(115200);

  // Sensor
  sensorSerial.begin(57600);

  sensor.uartTransmit = &el_uartTransmit;
  sensor.uartReceive = &el_uartReceive;
  sensor.uartWaitUntilReceived = &el_uartWaitUntilReceived;
  sensor.uartAbortReceive = &el_uartAbortReceive;

  // Init

  delay(EL_STARTUP_DELAY_MS);

  ELCOM_getSysModelName(&sensor, str);
  Serial.print("Model name: ");
  Serial.println(str);
  delay(200);

  ELCOM_getSysProdName(&sensor, str);
  Serial.print("Product name: ");
  Serial.println(str);
  delay(200);

  ELCOM_getSysFwVer(&sensor, str);
  Serial.print("Firmware version: ");
  Serial.println(str);
  delay(200);

  ELCOM_getSysSn(&sensor, &sn);
  Serial.print("Serial number: ");
  Serial.println(sn);
  delay(200);

  ELCOM_getSenName(&sensor, str);
  Serial.print("Sensor's name: ");
  Serial.println(str);
  delay(200);

  // Load sensor's format
  ELCOM_getSenDataFmt(&sensor, &sensor.dataFormat);
  delay(200);
}

void loop() {
  ELCOM_errorCode_t error_code;
  uint32_t runtime;
  ELICHENS_SensorData_t data;
  float temperature;

  error_code = ELCOM_getSysRunTime(&sensor, &runtime);
  error_code |= ELCOM_getSenData(&sensor, &data);
  error_code |= ELCOM_getSenTemp(&sensor, &temperature);

  if (ELCOM_NO_ERROR == error_code) {
     Serial.print("time = ");
     Serial.print(runtime);
     Serial.print(" ; ppm = ");
     Serial.print(data.value);
     Serial.print(" ; degC = ");
     Serial.println(temperature);
  }
  else {
    Serial.println("Failed to read sensor value");
  }

  delay(1000);
}


ELCOM_errorCode_t el_uartTransmit(uint8_t *data, uint16_t size)
{
  sensorSerial.write(data, size);
  return ELCOM_NO_ERROR;
}


ELCOM_errorCode_t el_uartReceive(uint8_t *data)
{
  // Here we have to clear the bufferRx otherwise el_uartWaitUntilReceived() would return instantly
  memset(sensor.bufferRx, 0, ELCOM_DATA_BUFFER_SIZE);

  // Clear any incoming data in serial
  while (sensorSerial.available()) {
    sensorSerial.read();
  }
  
  // Enables the selected software serial port to listen
  sensorSerial.listen();
}


ELCOM_errorCode_t el_uartWaitUntilReceived(void)
{
  uint32_t start = millis();
  uint8_t i = 0;

  while (millis() - start < 250) {
    while (sensorSerial.available()) {
      sensor.bufferRx[i++] = sensorSerial.read();
    }
    
    if (ELCOM_isResponseComplete(sensor.bufferRx)) {
      return ELCOM_NO_ERROR;
    }
    // Continue waiting
  }

  Serial.println("Receive data timed out");
  return ELCOM_SLAVE_TIMEOUT;
}


void el_uartAbortReceive(void)
{
  // Just clear the serial
  while (sensorSerial.available()) {
    sensorSerial.read();
  }
}

/*******************************************************************************
  * COPYRIGHT(c) 2019 Elichens
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
#ifndef __ELICHENS_DRIVER_H__
#define __ELICHENS_DRIVER_H__

#include <stdint.h>
#include "elCom.h"


#define EL_STARTUP_DELAY_MS				5000	// Time before we can send commands to the sensor


/********************************************************************
 * Sensor definition
 ********************************************************************/

typedef struct {
	ELCOM_DataFormat_t	dataFormat;										// Format used in the sensor data
	ELCOM_packet_t 		packet;											// Static packet to send and receive data with the sensor
	uint8_t 			bufferTx[ELCOM_DATA_BUFFER_SIZE];               // Transmit buffer
	uint8_t 			bufferRx[ELCOM_DATA_BUFFER_SIZE];				// Receive buffer
	ELCOM_errorCode_t 	(*uartTransmit)(uint8_t *data, uint16_t size);  // Callback to send some data to the sensor's UART
	ELCOM_errorCode_t 	(*uartReceive)(uint8_t *data);					// Callback to start listening to the sensor's UART
	ELCOM_errorCode_t   (*uartWaitUntilReceived)(void);                 // Callback to block the code until a response is received
	void				(*uartAbortReceive)(void);                      // Callback to stop listening to the sensor's UART
} ELICHENS_Sensor_t;


/********************************************************************
 * Basic information
 ********************************************************************/

ELCOM_errorCode_t ELCOM_getSysModelName(ELICHENS_Sensor_t *sensor, char modelName[24]);	// Model name
ELCOM_errorCode_t ELCOM_getSysProdName(ELICHENS_Sensor_t *sensor, char prodName[24]);  	// Product name
ELCOM_errorCode_t ELCOM_getSysFwVer(ELICHENS_Sensor_t *sensor, char version[7]);  		// Firmware version
ELCOM_errorCode_t ELCOM_getSysSn(ELICHENS_Sensor_t *sensor, uint32_t *sn);				// Serial number
ELCOM_errorCode_t ELCOM_getSysRunTime(ELICHENS_Sensor_t *sensor, uint32_t *runtime);	// Run time in seconds


/********************************************************************
 * Sensor's data
 ********************************************************************/

typedef struct {
	uint8_t 			status;		// Status code
	uint8_t 			error;		// Error code
	int32_t 			value;		// Concentration value in PPM
} ELICHENS_SensorData_t;

ELCOM_errorCode_t ELCOM_getSenData(ELICHENS_Sensor_t *sensor, ELICHENS_SensorData_t *data);		// Measure
ELCOM_errorCode_t ELCOM_getSenTemp(ELICHENS_Sensor_t *sensor, float *temperature);				// Internal temperature
ELCOM_errorCode_t ELCOM_getSenDataFmt(ELICHENS_Sensor_t *sensor, ELCOM_DataFormat_t	*format);	// Data format
ELCOM_errorCode_t ELCOM_getSenName(ELICHENS_Sensor_t *sensor, char name[8]);						// Sensor name (CO2, CH4, CH4NB)


#endif // __ELICHENS_DRIVER_H__

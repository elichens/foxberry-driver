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
#include "ELICHENS_driver.h"

#include <string.h>


/********************************************************************
 * Internal
 ********************************************************************/

/**
 * Divide positive or negative dividend by positive divisor and round
 * to closest integer. Result is undefined for negative divisors and
 * for negative dividends if the divisor variable type is unsigned.
 */
#define DIV_ROUND_CLOSEST(x, divisor)(          \
{                                               \
    typeof(x) __x = x;                          \
    typeof(divisor) __d = divisor;              \
    (((typeof(x))-1) > 0 ||                     \
     ((typeof(divisor))-1) > 0 || (__x) > 0) ?  \
        (((__x) + ((__d) / 2)) / (__d)) :       \
        (((__x) - ((__d) / 2)) / (__d));        \
}                                               \
)


/**
 * Generic function to send an ELCOM packet to the sensor, process its response and
 * handle any error that could occur.
 */
static ELCOM_errorCode_t EL_sendAndReceivePacket(ELICHENS_Sensor_t *sensor)
{
	ELCOM_errorCode_t err_code;
	uint8_t size;

	size = ELCOM_prepareSendPacket(&sensor->packet, sensor->bufferTx);

	// Start listening
	err_code = sensor->uartReceive(sensor->bufferRx);
	if (ELCOM_NO_ERROR != err_code) {
		return err_code;
	}

	// Send the packet
	err_code = sensor->uartTransmit(sensor->bufferTx, size);
	if (ELCOM_NO_ERROR != err_code) {
		sensor->uartAbortReceive();
		return err_code;
	}

	// Wait for response
	err_code = sensor->uartWaitUntilReceived();
	if (ELCOM_NO_ERROR != err_code) {
		sensor->uartAbortReceive();
		return err_code;
	}

	// Parse response
	err_code = ELCOM_parseReceivedPacket(sensor->bufferRx, &sensor->packet);

	return err_code;
}


/********************************************************************
 * Basic information
 ********************************************************************/

ELCOM_errorCode_t ELCOM_getSysModelName(ELICHENS_Sensor_t *sensor, char modelName[24])
{
	ELCOM_errorCode_t err_code;

	sensor->packet.cmd = ELCOM_CMD_GET_MODEL_NAME;
	sensor->packet.dataLength = 0;

	err_code = EL_sendAndReceivePacket(sensor);

	if (ELCOM_NO_ERROR != err_code) {
		modelName[0] = 0; // Empty name
		return err_code;
	}

	memcpy(modelName, sensor->packet.data, sensor->packet.dataLength);
	modelName[sensor->packet.dataLength] = 0;

	return err_code; // OK
}


ELCOM_errorCode_t ELCOM_getSysProdName(ELICHENS_Sensor_t *sensor, char prodName[24])
{
	ELCOM_errorCode_t err_code;

	sensor->packet.cmd = ELCOM_CMD_GET_PROD_NAME;
	sensor->packet.dataLength = 0;

	err_code = EL_sendAndReceivePacket(sensor);

	if (ELCOM_NO_ERROR != err_code) {
		prodName[0] = 0; // Empty name
		return err_code;
	}

	memcpy(prodName, sensor->packet.data, sensor->packet.dataLength);
	prodName[sensor->packet.dataLength] = 0;

	return err_code; // OK
}


ELCOM_errorCode_t ELCOM_getSysFwVer(ELICHENS_Sensor_t *sensor, char version[7])
{
	ELCOM_errorCode_t err_code;

	sensor->packet.cmd = ELCOM_CMD_GET_FW_VER;
	sensor->packet.dataLength = 0;

	err_code = EL_sendAndReceivePacket(sensor);

	if (ELCOM_NO_ERROR != err_code) {
		version[0] = 0; // Invalid version
		return err_code;
	}

	if (sensor->packet.dataLength != 6) {
		version[0] = 0; // Invalid version
		return ELCOM_SLAVE_ERROR; // Unexpected length
	}

	memcpy(version, sensor->packet.data, sensor->packet.dataLength);
	version[sensor->packet.dataLength] = 0;

	return err_code; // OK
}


ELCOM_errorCode_t ELCOM_getSysSn(ELICHENS_Sensor_t *sensor, uint32_t *sn)
{
	ELCOM_errorCode_t err_code;
	uint8_t c;

	*sn = 0;

	sensor->packet.cmd = ELCOM_CMD_GET_SEN_SN;
	sensor->packet.dataLength = 0;

	err_code = EL_sendAndReceivePacket(sensor);

	if (ELCOM_NO_ERROR != err_code) {
		return err_code; // Invalid sensor id
	}

	for (uint8_t i = 0; i < sensor->packet.dataLength; i++) {
		// We get the serial number in ASCII
		// sometimes as "SN00123456789", sometimes as "00123456" depending on the sensor version
		*sn = 10 * *sn;
		c = sensor->packet.data[i];
		if (c >= '0' && c <= '9') {
			*sn += c - '0';
		}
	}

	return err_code; // OK
}


ELCOM_errorCode_t ELCOM_getSysRunTime(ELICHENS_Sensor_t *sensor, uint32_t *runtime)
{
	ELCOM_errorCode_t err_code;

	sensor->packet.cmd = ELCOM_CMD_GET_RUN_TIME;
	sensor->packet.dataLength = 0;

	err_code = EL_sendAndReceivePacket(sensor);

	if (ELCOM_NO_ERROR != err_code) {
		return err_code;
	}

	memcpy(runtime, &sensor->packet.data[0], 4);

	return err_code; // OK
}


/********************************************************************
 * Sensor's data
 ********************************************************************/

ELCOM_errorCode_t ELCOM_getSenData(ELICHENS_Sensor_t *sensor, ELICHENS_SensorData_t *data)
{
	ELCOM_errorCode_t err_code;
	int32_t tmp;

	sensor->packet.cmd = ELCOM_CMD_GET_SEN_DATA;
	sensor->packet.dataLength = 1;
	sensor->packet.data[0] = 0;

	err_code = EL_sendAndReceivePacket(sensor);

	if (ELCOM_NO_ERROR != err_code) {
		data->status = 0xFF;
		data->error = 0xFF;
		data->value = 0;
		return err_code;
	}

	// byte 0 is the sensor index
	data->status = sensor->packet.data[1];
	data->error = sensor->packet.data[2];

	memcpy(&tmp, &sensor->packet.data[3], 4);
	data->value = DIV_ROUND_CLOSEST(tmp, 100);

	return err_code;
}


ELCOM_errorCode_t ELCOM_getSenTemp(ELICHENS_Sensor_t *sensor, float *temperature)
{
	ELCOM_errorCode_t err_code;
	int32_t tmp;

	sensor->packet.cmd = ELCOM_CMD_GET_SEN_TEMP;
	sensor->packet.dataLength = 1;
	sensor->packet.data[0] = 0;

	err_code = EL_sendAndReceivePacket(sensor);

	if (ELCOM_NO_ERROR != err_code) {
		return err_code;
	}

	// byte 0 is the sensor index
	memcpy(&tmp, &sensor->packet.data[1], 4);
	*temperature = tmp / 100.;

	return err_code;
}


ELCOM_errorCode_t ELCOM_getSenDataFmt(ELICHENS_Sensor_t *sensor, ELCOM_DataFormat_t	*format)
{
	ELCOM_errorCode_t err_code;

	sensor->packet.cmd = ELCOM_CMD_GET_SEN_DATA_FMT;
	sensor->packet.dataLength = 1;
	sensor->packet.data[0] = 0;

	err_code = EL_sendAndReceivePacket(sensor);

	if (ELCOM_NO_ERROR != err_code) {
		return err_code;
	}

	// byte 0 is the sensor index
	format->decimalPoint = sensor->packet.data[1];
	format->unitCode = sensor->packet.data[2];
	format->resInt = sensor->packet.data[3];
	format->resExp = sensor->packet.data[4];

	return err_code;
}


ELCOM_errorCode_t ELCOM_getSenName(ELICHENS_Sensor_t *sensor, char name[8])
{
	ELCOM_errorCode_t err_code;

	sensor->packet.cmd = ELCOM_CMD_GET_SEN_NAME;
	sensor->packet.dataLength = 1;
	sensor->packet.data[0] = 0;

	err_code = EL_sendAndReceivePacket(sensor);

	if (ELCOM_NO_ERROR != err_code) {
		name[0] = 0; // Empty name
		return err_code;
	}

	// byte 0 is the sensor index
	memcpy(name, &sensor->packet.data[1], sensor->packet.dataLength - 1);
	name[sensor->packet.dataLength - 1] = 0;

	return err_code;
}

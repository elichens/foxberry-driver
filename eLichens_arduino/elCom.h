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
#ifndef __ELCOM_H
#define __ELCOM_H

#include <stdint.h>


#define ELCOM_DATA_BUFFER_SIZE			(255u)


/********************************************************************
 * ELCOM Command codes
 ********************************************************************/

/* Basic system informations commands */
#define ELCOM_CMD_GET_MODEL_NAME         (0x10)
#define ELCOM_CMD_GET_PROD_NAME          (0x11)
#define ELCOM_CMD_GET_FW_VER             (0x12)
#define ELCOM_CMD_GET_SEN_SN             (0x13)
#define ELCOM_CMD_GET_RUN_TIME           (0x14)
#define ELCOM_CMD_GET_PROD_DATE          (0x16)

/* Sensor informations commands */
#define ELCOM_CMD_GET_SEN_DATA				 (0x21)
#define ELCOM_CMD_GET_SEN_TEMP				 (0x22)
#define ELCOM_CMD_GET_SEN_DATA_FMT			 (0x23)
#define ELCOM_CMD_GET_SEN_NAME				 (0x26)

/* Error commands */
#define ELCOM_CMD_ERROR_SLAVE		      (0x40)


/********************************************************************
 * ELCOM fields positions
 ********************************************************************/
#define ELCOM_FIELD_START_OF_PACKET_POS      (0)
#define ELCOM_FIELD_VER_POS                  (1)
#define ELCOM_FIELD_CMD_POS                  (2)
#define ELCOM_FIELD_LEN_POS                  (3)
#define ELCOM_FIELD_DATA_POS                 (4)
//note that checksum and EOP doesn't have a fix position because of variable data length


/********************************************************************
 * ELCOM fields sizes
 ********************************************************************/
#define ELCOM_FIELD_START_OF_PACKET_SIZE     (1)
#define ELCOM_FIELD_VER_SIZE                 (1)
#define ELCOM_FIELD_CMD_SIZE                 (1)
#define ELCOM_FIELD_LEN_SIZE                 (1)
#define ELCOM_FIELD_CRC_SIZE	             (2)
#define ELCOM_FIELD_END_OF_PACKET_SIZE       (1)
#define ELCOM_FIELD_HEADER_SIZE				 (ELCOM_FIELD_START_OF_PACKET_SIZE + ELCOM_FIELD_VER_SIZE + ELCOM_FIELD_CMD_SIZE + ELCOM_FIELD_LEN_SIZE)
#define ELCOM_FIELD_FOOTER_SIZE			     (ELCOM_FIELD_CRC_SIZE + ELCOM_FIELD_END_OF_PACKET_SIZE)
#define ELCOM_FIELD_DATA_MAX_SIZE            (ELCOM_DATA_BUFFER_SIZE-ELCOM_FIELD_CMD_SIZE-ELCOM_FIELD_CRC_SIZE-ELCOM_FIELD_END_OF_PACKET_SIZE)


/********************************************************************
 * ELCOM fields statics values
 ********************************************************************/
#define ELCOM_FIELD_START_OF_PACKET_VALUE    (0x5B)
#define ELCOM_FIELD_VER_VALUE                (0x01)
#define ELCOM_FIELD_END_OF_PACKET_VALUE      (0x5D)


/********************************************************************
 * ELCOM GET_DATA status masks
 ********************************************************************/
#define ELCOM_STATUS_WARMUP        		(1<<1)
#define ELCOM_STATUS_CALIBRATION     	(1<<3)
#define ELCOM_STATUS_LAMP				(1<<2)
#define ELCOM_STATUS_DATA_NOT_RELIABLE 	(1<<0)


/********************************************************************
 * ELCOM data structures and enum
 ********************************************************************/


/**
 *   @struct ELCOM_packet Buffer structure to hold a parsed ELCOM packet
 **/
typedef struct {
	uint8_t cmd;
	uint8_t dataLength;
	uint8_t data[ELCOM_FIELD_DATA_MAX_SIZE];
} ELCOM_packet_t;


/**
 *   @enum   ELCOM_error_code Error code returned by the ELCOM functions
 *           Differs from the error code sent by the slave error command
 **/
typedef enum {
	ELCOM_NO_ERROR				= 0x00,
	ELCOM_INVALID_SOP			= 0x01,
	ELCOM_INVALID_VER			= 0x02,
	ELCOM_INVALID_CRC			= 0x03,
	ELCOM_INVALID_EOP			= 0x04,
	ELCOM_COMMAND_UNKNOW		= 0x05,
	ELCOM_SLAVE_TIMEOUT			= 0x06,
	ELCOM_SLAVE_ERROR			= 0x07,
} ELCOM_errorCode_t;


/**
 *   @enum  ELCOM_slave_error_code Error code sent by the error slave command
 **/
typedef enum {
	ELCOM_FAIL_UNKNOW			= 0x01,
	ELCOM_FAIL_INVALID_CMD		= 0x02,
	ELCOM_FAIL_DATASIZE			= 0x03,
	ELCOM_FAIL_INVALIDVALUE		= 0x04,
	ELCOM_FAIL_NO_RIGHT			= 0x05,
	ELCOM_FAIL_OPERATION		= 0x06,
} ELCOM_slaveErrorCode_t;


typedef struct {
	uint8_t decimalPoint;
	uint8_t unitCode;
	uint8_t resInt;
	uint8_t resExp;
} ELCOM_DataFormat_t;


/********************************************************************
 * ELCOM function prototype
 ********************************************************************/

ELCOM_errorCode_t ELCOM_parseReceivedPacket(uint8_t *dataBufferIn, ELCOM_packet_t *packetOut);
uint8_t ELCOM_prepareSendPacket(ELCOM_packet_t *packetToSend, uint8_t *dataBufferOut);
ELCOM_slaveErrorCode_t ELCOM_handleError(ELCOM_errorCode_t errorCode, ELCOM_packet_t *packetOut);
uint8_t ELCOM_isResponseComplete(uint8_t *buffer);


#endif

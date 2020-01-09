/* ========================================
    *   @file       elcom.c  
    *   @brief      
    *   @author		eLichens
    *   @version	1.0
    *   @date		2017/03
    
 * Copyright eLichens, 2017
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF ELICHENS.
 * ========================================
*/

#include "elcom.h"

#include <stdio.h>
#include <string.h>
#include "crc_el.h"


/* External variables --------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private callback function -----------------------------------------------*/

static ELCOM_errorCode_t ELCOM_validateHeader(uint8_t *packetData);
static ELCOM_errorCode_t ELCOM_validateEOP(uint8_t *packetData);
static ELCOM_errorCode_t ELCOM_validateCheksum(uint8_t *packetData);
static uint8_t ELCOM_extractDataLength(uint8_t *packetData);
static uint8_t ELCOM_extractCommand(uint8_t *packetData);
static void ELCOM_extractData(uint8_t *packetData, uint8_t *dataOut, uint8_t dataLength);

/* End private callback function -------------------------------------------*/


/**
 *   @brief  Check if a received packet is valid and parse the fields into a structure
 *   @param  dataBufferIn    The buffer containing the packet to parse
 *   @param  packetOut       Structure pointer parse data destination
 *   @return ELCOM_INVALID_CHECKSUM : if checksum invalid
 *           ELCOM_INVALID_SOP :  if start of packet is invalid
 *           ELCOM_INVALID_VER :  if invalid version field
 *           ELCOM_INVALID_EOP :  if invalid end of packet
 *           ELCOM_INVALID_CHECKSUM : if invalid checksum
 *           ELCOM_NO_ERROR otherwise
 **/
ELCOM_errorCode_t ELCOM_parseReceivedPacket(uint8_t *dataBufferIn, ELCOM_packet_t *packetOut)
{
	ELCOM_errorCode_t err_code;

	packetOut->cmd = 0;
	packetOut->dataLength = 0;
	memset(packetOut->data, 0, ELCOM_FIELD_DATA_MAX_SIZE);

	/* Verify the header */
	err_code = ELCOM_validateHeader(dataBufferIn);
	if (err_code != ELCOM_NO_ERROR) {
		return err_code;
	}

	/* Verify the footer */
	err_code = ELCOM_validateEOP(dataBufferIn);
	if (err_code != ELCOM_NO_ERROR) {
		return err_code;
	}

	/* Verify checksum */
	err_code = ELCOM_validateCheksum(dataBufferIn);
	if (err_code != ELCOM_NO_ERROR) {
		return err_code;
	}

	/* Extract data length */
	packetOut->dataLength = ELCOM_extractDataLength(dataBufferIn);

	/* Extract command */
	packetOut->cmd = ELCOM_extractCommand(dataBufferIn);

	/* Extract data */
	ELCOM_extractData(dataBufferIn, packetOut->data, packetOut->dataLength);

	if (ELCOM_CMD_ERROR_SLAVE == packetOut->cmd) {
		return ELCOM_SLAVE_ERROR;
	}

	return ELCOM_NO_ERROR;
}


/**
 *   @brief : Check whether the packet header is valid or not
 *   @param  packetData :    buffer containing the header (start of message at index 0)
 *   @return ELCOM_INVALID_VER, if version is not 0x01, ELCOM_INVALID_SOP if the first byte is not 0x5B, ELCOM_NO_ERROR otherwise
 **/
static ELCOM_errorCode_t ELCOM_validateHeader(uint8_t *packetData)
{
    if (packetData[ELCOM_FIELD_START_OF_PACKET_POS] != ELCOM_FIELD_START_OF_PACKET_VALUE
    		|| packetData[ELCOM_FIELD_VER_POS] != ELCOM_FIELD_VER_VALUE) {
    	return ELCOM_INVALID_SOP;
    }

	if (packetData[ELCOM_FIELD_VER_POS] != ELCOM_FIELD_VER_VALUE) {
		return ELCOM_INVALID_VER;
	}

	return ELCOM_NO_ERROR;
}


/**
 *   @brief  Check whether the packet footer is valid or not
 *   @param  packetData :    buffer containing the packet bytes (start of message at index 0)
 *   @return ELCOM_INVALID_EOP if the first byte is not 0x5D, ELCOM_NO_ERROR otherwise
 **/
static ELCOM_errorCode_t ELCOM_validateEOP(uint8_t *packetData)
{
    if (packetData[packetData[ELCOM_FIELD_LEN_POS] + ELCOM_FIELD_HEADER_SIZE + ELCOM_FIELD_CRC_SIZE] != ELCOM_FIELD_END_OF_PACKET_VALUE) {
    	return ELCOM_INVALID_EOP;
    }

    return ELCOM_NO_ERROR;
}


/**
 *   @brief  Check whether the checksum contained in a received packet
 *   @param  packetData  buffer containing the packet bytes (start of packet at index 0)
 *   @return ELCOM_INVALID_CHECKSUM if there is a checksum error, ELCOM_NO_ERROR otherwise
 **/
static ELCOM_errorCode_t ELCOM_validateCheksum(uint8_t *packetData)
{
	uint8_t dataLength = packetData[ELCOM_FIELD_LEN_POS];

    uint8_t crc_data_length = ELCOM_FIELD_START_OF_PACKET_SIZE +
                              ELCOM_FIELD_VER_SIZE +
                              ELCOM_FIELD_LEN_SIZE +
                              ELCOM_FIELD_CMD_SIZE +
                              dataLength;

    uint16_t packetCrc = 0;
    uint16_t computeCrc = 0;

    memcpy(&packetCrc, &packetData[crc_data_length], ELCOM_FIELD_CRC_SIZE);

    computeCrc = CRC_computeCRC(packetData, crc_data_length);

    if (packetCrc != computeCrc) {
    	return ELCOM_INVALID_CRC;
    }

    return ELCOM_NO_ERROR;
}

/**
 *   @brief  Extract the data length in the packet.
 *           Note that the data length differs from the data field value. (It includes also the size of command, cheksum and end of packet field size)
 *   @param  packetData  buffer containing the packet bytes (start of packet at index 0)
 *   @return the data length
 **/
static uint8_t ELCOM_extractDataLength(uint8_t *packetData)
{
    return packetData[ELCOM_FIELD_LEN_POS];
}

/**
 *   @brief  Extract the command field value
 *   @param  packetData  buffer containing the packet bytes (start of packet at index 0)
 *   @return the command field value
 **/
static uint8_t ELCOM_extractCommand(uint8_t *packetData)
{
	return packetData[ELCOM_FIELD_CMD_POS];
}


/**
 *   @brief  Extract the data from a given packet and place it in a buffer
 *   @param  packetData  buffer containing the packet bytes (start of packet at index 0)
 *   @param  dataOut     buffer to place the extracted data
 *   @param  dataLength  the length of data NOTE : It's not the data field value but the effective data length
 **/
static void ELCOM_extractData(uint8_t *packetData, uint8_t *dataOut, uint8_t dataLength)
{
    if (dataLength > ELCOM_FIELD_DATA_MAX_SIZE) {
        return;
    }
	memcpy(dataOut, &packetData[ELCOM_FIELD_DATA_POS], dataLength);
}


/**
 *   @brief  Construct a ELCOM packet with the values passed in the structured.
 *           The UART buffer is then filled with this packet
 *   @param  packetToSend    Structure containing the packet to send fields value
 *   @param  dataBufferOut   Buffer that will be ready to be transmitted
 *   @return size of the buffer size
 **/
uint8_t ELCOM_prepareSendPacket(ELCOM_packet_t *packetToSend, uint8_t *dataBufferOut)
{
    uint16_t crc = 0;
    uint8_t* bufferIndex = dataBufferOut;
    uint8_t dataLength;

	memset(dataBufferOut, 0, ELCOM_DATA_BUFFER_SIZE);

    //Start constructing packet
    //SOP
    *bufferIndex = ELCOM_FIELD_START_OF_PACKET_VALUE;
    bufferIndex += ELCOM_FIELD_START_OF_PACKET_SIZE;
    
    //VER
    *bufferIndex = ELCOM_FIELD_VER_VALUE;
    bufferIndex += ELCOM_FIELD_VER_SIZE;

    //CMD
    *bufferIndex = packetToSend->cmd;
    bufferIndex += ELCOM_FIELD_CMD_SIZE;
    
    //LEN
    dataLength = packetToSend->dataLength;
    *bufferIndex = dataLength;
    bufferIndex += ELCOM_FIELD_LEN_SIZE;
    
    //Data
    memcpy(bufferIndex, packetToSend->data, dataLength);
    bufferIndex += dataLength;
    
    //CRC
    crc = CRC_computeCRC(dataBufferOut, bufferIndex - dataBufferOut);
    
    //Copy the CRC
    memcpy(bufferIndex, (uint8_t*)&crc, ELCOM_FIELD_CRC_SIZE);
    bufferIndex += ELCOM_FIELD_CRC_SIZE;
    
    //EOP
    *bufferIndex = ELCOM_FIELD_END_OF_PACKET_VALUE;
    bufferIndex += ELCOM_FIELD_END_OF_PACKET_SIZE;
    
    return (bufferIndex - dataBufferOut); // Size of the output buffer
}


/**
 * @brief ELCOM error handler, build the ELCOM_CMD_ERROR_SLAVE response on an error.
 * @param ELCOM_errorCode_t errorCode Any error code met (should not be ELCOM_NO_ERROR)
 * @param ELCOM_packet_t *packetOut
 * @retval ELCOM_slaveErrorCode_t set in the packetOut
 */
ELCOM_slaveErrorCode_t ELCOM_handleError(ELCOM_errorCode_t errorCode, ELCOM_packet_t *packetOut)
{
	ELCOM_slaveErrorCode_t slaveErrorCode;

	memset(packetOut->data, 0, ELCOM_FIELD_DATA_MAX_SIZE);

	switch (errorCode)
	{
	case ELCOM_COMMAND_UNKNOW:
		slaveErrorCode = ELCOM_FAIL_INVALID_CMD;
		break;

	case ELCOM_NO_ERROR:
		// Should not happen !

	default:
		// Any validation error
		slaveErrorCode = ELCOM_FAIL_OPERATION;
	}

	packetOut->cmd = ELCOM_CMD_ERROR_SLAVE;
	packetOut->data[0] = slaveErrorCode;
	packetOut->dataLength = 1;

	return slaveErrorCode;
}


uint8_t ELCOM_isResponseComplete(uint8_t *buffer)
{
	uint8_t dataLength;
	return (dataLength = buffer[ELCOM_FIELD_LEN_POS]) != 0
			&& buffer[dataLength + 6] != 0;
}

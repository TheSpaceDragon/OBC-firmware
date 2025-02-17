#include "cc1120_txrx.h"
#include "obc_logging.h"
#include "cc1120_mcu.h"
#include "cc1120_spi.h"
#include "cc1120_defs.h"
#include "obc_math.h"
#include "obc_board_config.h"

// TODO: Ideally, we shouldn't need to include this in the driver
#include "uplink_decoder.h"

#include <FreeRTOS.h>
#include <os_semphr.h>
#include <sys_common.h>
#include <FreeRTOSConfig.h>

#include <stdbool.h>

#define COMMS_MAX_UPLINK_BYTES \
  1000U  // Maximum amount of bytes we will currently be uplinking at a time (should be updated in the future)

#define TX_SEMAPHORE_TIMEOUT pdMS_TO_TICKS(5000)
#define RX_SEMAPHORE_TIMEOUT pdMS_TO_TICKS(100)
#define TX_FIFO_EMPTY_SEMAPHORE_TIMEOUT pdMS_TO_TICKS(5000)
#define SYNC_EVENT_SEMAPHORE_TIMEOUT pdMS_TO_TICKS(30000)

static SemaphoreHandle_t rxSemaphore = NULL;
static StaticSemaphore_t rxSemaphoreBuffer;
static SemaphoreHandle_t txSemaphore = NULL;
static StaticSemaphore_t txSemaphoreBuffer;
static SemaphoreHandle_t txFifoEmptySemaphore = NULL;
static StaticSemaphore_t txFifoEmptySemaphoreBuffer;
static SemaphoreHandle_t syncReceivedSemaphore = NULL;
static StaticSemaphore_t syncReceivedSemaphoreBuffer;

static obc_error_code_t cc1120SendVariablePktMode(uint8_t *data, uint32_t len);

static obc_error_code_t cc1120SendInifinitePktMode(uint8_t *data, uint32_t len);

static obc_error_code_t writeFifoBlocking(uint8_t *data, uint32_t len);

static register_setting_t cc1120SettingsStd[] = {
    // Set GPIO 0 to RXFIFO_THR_PKT
    {CC1120_REGS_IOCFG0, 0x01U},
    // Set GPIO 1 to HighZ
    {CC1120_REGS_IOCFG1, 0x30U},
    // Set GPIO 2 to PKT_SYNC_RXTX
    {CC1120_REGS_IOCFG2, 0x06U},
    // Set GPIO 3 to TXFIFO_THR_PKT
    {CC1120_REGS_IOCFG3, 0x03U},
    // Set the sync word as 16 bits and allow for < 2 bit error on sync word
    {CC1120_REGS_SYNC_CFG0, 0x09U},
    // Set sync word qualifier value threshold similar to the one talked about for preamble in section 6.8
    {CC1120_REGS_SYNC_CFG1, 0x08U},
    // Set first 8 bits of the sync word to 0x55U (arbitrary value)
    {CC1120_REGS_SYNC0, 0x55U},
    // Set next 8 bits of the sync word to 0x57U (arbitrary value)
    {CC1120_REGS_SYNC1, 0x57U},
    // Set cc1120 to switch to FSTXON state after a packet is received
    {CC1120_REGS_RFEND_CFG1, 0x1F},
    {CC1120_REGS_DEVIATION_M, 0x3AU},
    {CC1120_REGS_MODCFG_DEV_E, 0x0AU},
    {CC1120_REGS_DCFILT_CFG, 0x1CU},
    // Set the preamble as 4 bytes of 10101010
    {CC1120_REGS_PREAMBLE_CFG1, 0x18U},
    // enable preamble and set the error threshold
    {CC1120_REGS_PREAMBLE_CFG0, 0x2AU},
    {CC1120_REGS_IQIC, 0xC6U},
    {CC1120_REGS_CHAN_BW, 0x08U},
    {CC1120_REGS_MDMCFG0, 0x05U},
    {CC1120_REGS_SYMBOL_RATE2, 0x73U},
    {CC1120_REGS_AGC_REF, 0x20U},
    {CC1120_REGS_AGC_CS_THR, 0x19U},
    {CC1120_REGS_AGC_CFG2, 0x20U},
    {CC1120_REGS_AGC_CFG1, 0xA9U},
    {CC1120_REGS_AGC_CFG0, 0xCFU},
    {CC1120_REGS_FIFO_CFG, TXRX_INTERRUPT_THRESHOLD},
    {CC1120_REGS_FS_CFG, 0x14U},
    {CC1120_REGS_PKT_CFG0, 0x00U},
    {CC1120_REGS_PA_CFG0, 0x7DU},
    {CC1120_REGS_PKT_LEN, 0x0CU}};

static register_setting_t cc1120SettingsExt[] = {
    {CC1120_REGS_EXT_IF_MIX_CFG, 0x00U}, {CC1120_REGS_EXT_FREQOFF_CFG, 0x34U}, {CC1120_REGS_EXT_FREQ2, 0x6CU},
    {CC1120_REGS_EXT_FREQ1, 0x7AU},      {CC1120_REGS_EXT_FREQ0, 0xE1U},       {CC1120_REGS_EXT_FS_DIG1, 0x00U},
    {CC1120_REGS_EXT_FS_DIG0, 0x5FU},    {CC1120_REGS_EXT_FS_CAL1, 0x40U},     {CC1120_REGS_EXT_FS_CAL0, 0x0EU},
    {CC1120_REGS_EXT_FS_DIVTWO, 0x03U},  {CC1120_REGS_EXT_FS_DSM0, 0x33U},     {CC1120_REGS_EXT_FS_DVC0, 0x17U},
    {CC1120_REGS_EXT_FS_PFD, 0x50U},     {CC1120_REGS_EXT_FS_PRE, 0x6EU},      {CC1120_REGS_EXT_FS_REG_DIV_CML, 0x14U},
    {CC1120_REGS_EXT_FS_SPARE, 0xACU},   {CC1120_REGS_EXT_FS_VCO0, 0xB4U},     {CC1120_REGS_EXT_XOSC5, 0x0EU},
    {CC1120_REGS_EXT_XOSC1, 0x03U},      {CC1120_REGS_EXT_TOC_CFG, 0x89U}};

void initAllTxRxSemaphores(void) {
  if (txSemaphore == NULL) {
    txSemaphore = xSemaphoreCreateBinaryStatic(&txSemaphoreBuffer);
    // Initialize semaphore with count of 1
    xSemaphoreGive(txSemaphore);
  }
  if (rxSemaphore == NULL) {
    rxSemaphore = xSemaphoreCreateBinaryStatic(&rxSemaphoreBuffer);
  }
  if (txFifoEmptySemaphore == NULL) {
    txFifoEmptySemaphore = xSemaphoreCreateBinaryStatic(&txFifoEmptySemaphoreBuffer);
    // Initialize semaphore with count of 1
    xSemaphoreGive(txFifoEmptySemaphore);
  }
  if (syncReceivedSemaphore == NULL) {
    syncReceivedSemaphore = xSemaphoreCreateBinaryStatic(&syncReceivedSemaphoreBuffer);
  }
}

/**
 * @brief Gets the number of bytes queued in the TX FIFO
 *
 * @param numBytes - A pointer to an 8-bit integer to store the number of bytes in
 * @return obc_error_code_t - Whether or not the registe read was successful
 */
obc_error_code_t cc1120GetBytesInTxFifo(uint8_t *numBytes) {
  if (numBytes == NULL) {
    return OBC_ERR_CODE_INVALID_ARG;
  }
  obc_error_code_t errCode;
  RETURN_IF_ERROR_CODE(cc1120ReadExtAddrSpi(CC1120_REGS_EXT_NUM_TXBYTES, numBytes, 1));
  return OBC_ERR_CODE_SUCCESS;
}

/**
 * @brief Gets the state of the CC1120 from the MARCSTATE register
 *
 * @param stateNum - A pointer to an 8-bit integer to store the state in
 * @return obc_error_code_t - Whether or not the register read was successful
 */
obc_error_code_t cc1120GetState(cc1120_state_t *stateNum) {
  if (stateNum == NULL) {
    return OBC_ERR_CODE_INVALID_ARG;
  }
  obc_error_code_t errCode;
  RETURN_IF_ERROR_CODE(cc1120ReadExtAddrSpi(CC1120_REGS_EXT_MARCSTATE, stateNum, 1));
  *stateNum &= 0b11111;
  return OBC_ERR_CODE_SUCCESS;
}

/**
 * @brief Resets CC1120 & initializes transmit mode
 *
 * @return obc_error_code_t - Whether or not the setup was a success
 */
obc_error_code_t cc1120Init(void) {
  obc_error_code_t errCode;

  // When changing which signals are sent by each gpio, the output will be unstable so interrupts should be disabled
  // see chapter 3.4 in the datasheet for more info
  gioDisableNotification(gioPORTB, CC1120_RX_THR_PKT_gioPORTB_PIN);
  gioDisableNotification(gioPORTB, CC1120_TX_THR_PKT_hetPORT1_PIN);
  gioDisableNotification(gioPORTA, CC1120_PKT_SYNC_RXTX_hetPORT1_PIN);

  for (uint8_t i = 0; i < sizeof(cc1120SettingsStd) / sizeof(register_setting_t); i++) {
    RETURN_IF_ERROR_CODE(cc1120WriteSpi(cc1120SettingsStd[i].addr, &cc1120SettingsStd[i].val, 1));
  }

  // enable interrupts again now that the gpio signals are set
  gioEnableNotification(gioPORTB, CC1120_RX_THR_PKT_gioPORTB_PIN);
  gioEnableNotification(gioPORTB, CC1120_TX_THR_PKT_hetPORT1_PIN);
  gioEnableNotification(gioPORTA, CC1120_PKT_SYNC_RXTX_hetPORT1_PIN);
  for (uint8_t i = 0; i < sizeof(cc1120SettingsExt) / sizeof(register_setting_t); i++) {
    RETURN_IF_ERROR_CODE(cc1120WriteExtAddrSpi(cc1120SettingsExt[i].addr, &cc1120SettingsExt[i].val, 1));
  }

  initAllTxRxSemaphores();

  RETURN_IF_ERROR_CODE(cc1120StrobeSpi(CC1120_STROBE_SFSTXON));
  return OBC_ERR_CODE_SUCCESS;
}

/**
 * @brief Adds the given data to the CC1120 FIFO buffer and transmits
 *
 * @param data - The packet to transmit
 * @param len - The size of the provided packet in bytes
 * @return obc_error_code_t
 */
obc_error_code_t cc1120Send(uint8_t *data, uint32_t len) {
  obc_error_code_t errCode;

  if (txSemaphore == NULL) {
    return OBC_ERR_CODE_INVALID_STATE;
  }

  if (len < 1) {
    return OBC_ERR_CODE_INVALID_ARG;
  }

  if (data == NULL) {
    return OBC_ERR_CODE_INVALID_ARG;
  }

  // wait on the semaphore to make sure tx fifo is empty
  if (xSemaphoreTake(txFifoEmptySemaphore, TX_FIFO_EMPTY_SEMAPHORE_TIMEOUT) != pdPASS) {
    LOG_ERROR_CODE(OBC_ERR_CODE_SEMAPHORE_TIMEOUT);
    return OBC_ERR_CODE_SEMAPHORE_TIMEOUT;
  }

  // See section 8.1.5
  if (len > CC1120_MAX_PACKET_LEN) {
    RETURN_IF_ERROR_CODE(cc1120SendInifinitePktMode(data, len));
  } else {  // If packet size <= 255, use variable packet length mode
    RETURN_IF_ERROR_CODE(cc1120SendVariablePktMode(data, len));
  }

  return OBC_ERR_CODE_SUCCESS;
}

/**
 * @brief Transmits data with length less than 256
 *
 * @param data - The packet to transmit
 * @param len - The size of the provided packet in bytes
 * @return obc_error_code_t
 */
static obc_error_code_t cc1120SendVariablePktMode(uint8_t *data, uint32_t len) {
  obc_error_code_t errCode;

  // Set to variable packet length mode
  uint8_t spiTransferData = VARIABLE_PACKET_LENGTH_MODE;
  RETURN_IF_ERROR_CODE(cc1120WriteSpi(CC1120_REGS_PKT_CFG0, &spiTransferData, 1));

  // Set max packet size
  spiTransferData = CC1120_MAX_PACKET_LEN;
  RETURN_IF_ERROR_CODE(cc1120WriteSpi(CC1120_REGS_PKT_LEN, &spiTransferData, 1));

  // Write current packet size
  uint8_t variableDataLen = (uint8_t)len;
  RETURN_IF_ERROR_CODE(cc1120WriteFifo(&variableDataLen, 1));  // Write packet size

  // Write TXRX_INTERRUPT_THRESHOLD bytes to TX fifo and activate TX mode
  RETURN_IF_ERROR_CODE(writeFifoBlocking(data, uint32Min(len, (uint32_t)TXRX_INTERRUPT_THRESHOLD)));
  RETURN_IF_ERROR_CODE(cc1120StrobeSpi(CC1120_STROBE_STX));

  // Continously wait for the tx fifo to drop below (128 - TXRX_INTERRUPT_THRESHOLD) bytes before writing
  // TXRX_INTERRUPT_THRESHOLD more bytes
  uint32_t groupsOfBytesWritten;
  for (groupsOfBytesWritten = 1; groupsOfBytesWritten < len / TXRX_INTERRUPT_THRESHOLD; groupsOfBytesWritten++) {
    RETURN_IF_ERROR_CODE(
        writeFifoBlocking(data + groupsOfBytesWritten * TXRX_INTERRUPT_THRESHOLD, TXRX_INTERRUPT_THRESHOLD));
  }

  // If not all bytes have been sent, write the remaining bytes to TX FIFO
  uint32_t bytesSent = (groupsOfBytesWritten - 1) * TXRX_INTERRUPT_THRESHOLD + uint32Min(len, TXRX_INTERRUPT_THRESHOLD);
  if (bytesSent < len) {
    RETURN_IF_ERROR_CODE(writeFifoBlocking(data + groupsOfBytesWritten * TXRX_INTERRUPT_THRESHOLD,
                                           len - groupsOfBytesWritten * TXRX_INTERRUPT_THRESHOLD));
  }
  return OBC_ERR_CODE_SUCCESS;
}

/**
 * @brief Transmits data with length greater than 255
 *
 * @param data - The packet to transmit
 * @param len - The size of the provided packet in bytes
 * @return obc_error_code_t
 */
static obc_error_code_t cc1120SendInifinitePktMode(uint8_t *data, uint32_t len) {
  obc_error_code_t errCode;

  // temporarily set packet size to infinite
  uint8_t spiTransferData = INFINITE_PACKET_LENGTH_MODE;
  RETURN_IF_ERROR_CODE(cc1120WriteSpi(CC1120_REGS_PKT_CFG0, &spiTransferData, 1));

  // Set packet length to mod(len, 256) so that the correct number of bits
  // are sent when fixed packet mode gets reactivated
  spiTransferData = len % (CC1120_MAX_PACKET_LEN + 1);
  RETURN_IF_ERROR_CODE(cc1120WriteSpi(CC1120_REGS_PKT_LEN, &spiTransferData, 1));

  // Write TXRX_INTERRUPT_THRESHOLD bytes to TX fifo and activate TX mode
  RETURN_IF_ERROR_CODE(writeFifoBlocking(data, TXRX_INTERRUPT_THRESHOLD));
  RETURN_IF_ERROR_CODE(cc1120StrobeSpi(CC1120_STROBE_STX));

  // Continously wait for the tx fifo to drop below (128 - TXRX_INTERRUPT_THRESHOLD) bytes before writing
  // TXRX_INTERRUPT_THRESHOLD more bytes need to also make sure that we do not send all of the remaining bytes if len is
  // a multiple of TXRX_INTERRUPT_THRESHOLD to ensure this, we subtract 1 from len in the loop bounds so there is always
  // 1 to TXRX_INTERRUPT_THRESHOLD Bytes left after the loop
  uint32_t groupsOfBytesWritten;
  for (groupsOfBytesWritten = 1; groupsOfBytesWritten < (len - 1) / TXRX_INTERRUPT_THRESHOLD; groupsOfBytesWritten++) {
    RETURN_IF_ERROR_CODE(
        writeFifoBlocking(data + groupsOfBytesWritten * TXRX_INTERRUPT_THRESHOLD, TXRX_INTERRUPT_THRESHOLD));
  }

  // switch back to fixed packet length mode so that transmission is able to properly end once the remaining bytes are
  // sent
  spiTransferData = FIXED_PACKET_LENGTH_MODE;
  RETURN_IF_ERROR_CODE(cc1120WriteSpi(CC1120_REGS_PKT_CFG0, &spiTransferData, 1));

  // write the remaining bytes to TX FIFO
  RETURN_IF_ERROR_CODE(writeFifoBlocking(data + groupsOfBytesWritten * TXRX_INTERRUPT_THRESHOLD,
                                         len - groupsOfBytesWritten * TXRX_INTERRUPT_THRESHOLD));

  return OBC_ERR_CODE_SUCCESS;
}

/**
 * @brief helper function for the cc1120Send functions to take the txSemaphore and then write data to TX FIFO
 *
 * @param data - The packet to transmit
 * @param len - The size of the provided packet in bytes
 * @return obc_error_code_t
 */
static obc_error_code_t writeFifoBlocking(uint8_t *data, uint32_t len) {
  obc_error_code_t errCode;
  if (xSemaphoreTake(txSemaphore, TX_SEMAPHORE_TIMEOUT) != pdPASS) {
    LOG_ERROR_CODE(OBC_ERR_CODE_SEMAPHORE_TIMEOUT);
    return OBC_ERR_CODE_SEMAPHORE_TIMEOUT;
  }
  errCode = cc1120WriteFifo(data, len);
  if (errCode != OBC_ERR_CODE_SUCCESS) {
    LOG_ERROR_CODE(errCode);
    xSemaphoreGive(txSemaphore);
    return errCode;
  }
  return OBC_ERR_CODE_SUCCESS;
}
/* RX functions */

/**
 * @brief Gets the number of bytes queued in the RX FIFO
 *
 * @param numBytes - A pointer to an 8-bit integer to store the number of bytes in
 * @return obc_error_code_t - Whether or not the register read was successful
 */
obc_error_code_t cc1120GetBytesInRxFifo(uint8_t *numBytes) {
  if (numBytes == NULL) {
    return OBC_ERR_CODE_INVALID_ARG;
  }
  obc_error_code_t errCode;
  RETURN_IF_ERROR_CODE(cc1120ReadExtAddrSpi(CC1120_REGS_EXT_NUM_RXBYTES, numBytes, 1));
  return OBC_ERR_CODE_SUCCESS;
}

/**
 * @brief Switches the cc1120 to RX mode to continuously receive bytes and send them to the decode task
 *
 * @return obc_error_code_t
 */
obc_error_code_t cc1120Receive(void) {
  obc_error_code_t errCode = OBC_ERR_CODE_SUCCESS;
  if (rxSemaphore == NULL) {
    return OBC_ERR_CODE_INVALID_STATE;
  }

  // poll the semaphore to clear whatever value it has (do not block and wait on it)
  xSemaphoreTake(syncReceivedSemaphore, (TickType_t)0);

  // When changing which signals are sent by each gpio, the output will be unstable so interrupts should be disabled
  // see chapter 3.4 in the datasheet for more info
  gioDisableNotification(gioPORTA, CC1120_PKT_SYNC_RXTX_hetPORT1_PIN);

  // switch gpio 2 to be a SYNC_EVENT signal instead of CC1120_PKT_SYNC_RXTX_PIN
  uint8_t spiTransferData = SYNC_EVENT_SIGNAL_NUM;
  RETURN_IF_ERROR_CODE(cc1120WriteSpi(CC1120_REGS_IOCFG2, &spiTransferData, 1));

  // enable interrupts again now that the gpio signals are set
  gioEnableNotification(gioPORTA, (uint32_t)CC1120_SYNC_EVENT_PIN);

  // Temporarily set packet size to infinite
  spiTransferData = INFINITE_PACKET_LENGTH_MODE;
  RETURN_IF_ERROR_CODE(cc1120WriteSpi(CC1120_REGS_PKT_CFG0, &spiTransferData, 1));

  // Switch cc1120 to receive mode
  RETURN_IF_ERROR_CODE(cc1120StrobeSpi(CC1120_STROBE_SRX));

  uint8_t dataBuffer[TXRX_INTERRUPT_THRESHOLD];

  // wait to receive sync word before continuing
  if (xSemaphoreTake(syncReceivedSemaphore, SYNC_EVENT_SEMAPHORE_TIMEOUT) != pdPASS) {
    LOG_ERROR_CODE(OBC_ERR_CODE_SEMAPHORE_TIMEOUT);
    return OBC_ERR_CODE_SEMAPHORE_TIMEOUT;
  }
  // See chapters 8.1, 8.4, 8.5
  // If we do not stop receiving data, continue looping until COMMS_MAX_UPLINK_BYTES rounded up to the nearest multiple
  // of TXRX_INTERRUPT_THRESHOLD bytes are received
  uint8_t rxFifoReadCycles;  // number of times we receive TXRX_INTERRUPT_THRESHOLD bytes and read them out
  for (rxFifoReadCycles = 0;
       rxFifoReadCycles < (COMMS_MAX_UPLINK_BYTES + TXRX_INTERRUPT_THRESHOLD - 1) / TXRX_INTERRUPT_THRESHOLD;
       ++rxFifoReadCycles) {
    // wait until we have not received more than TXRX_INTERRUPT_THRESHOLD bytes for more than RX_SEMAPHORE_TIMEOUT
    // before exiting this loop since that means we are no longer transmitting
    if (xSemaphoreTake(rxSemaphore, RX_SEMAPHORE_TIMEOUT) != pdPASS) {
      break;
    }
    RETURN_IF_ERROR_CODE(cc1120ReadFifo(dataBuffer, TXRX_INTERRUPT_THRESHOLD));
    for (uint8_t i = 0; i < TXRX_INTERRUPT_THRESHOLD; ++i) {
      sendToDecodeDataQueue(&dataBuffer[i]);
    }
  }

  uint8_t numBytesInRxFifo;

  // check the number of bytes remaining in the RX FIFO
  RETURN_IF_ERROR_CODE(cc1120GetBytesInRxFifo(&numBytesInRxFifo));

  if (numBytesInRxFifo != 0) {
    // if there are still bytes in the RX FIFO, read them out
    RETURN_IF_ERROR_CODE(cc1120ReadFifo(dataBuffer, numBytesInRxFifo));
  }

  // send the bytes read (if any) to decode data queue
  for (uint8_t i = 0; i < numBytesInRxFifo; ++i) {
    sendToDecodeDataQueue(&dataBuffer[i]);
  }

  if (rxFifoReadCycles == (COMMS_MAX_UPLINK_BYTES + TXRX_INTERRUPT_THRESHOLD - 1) / TXRX_INTERRUPT_THRESHOLD) {
    // if recv was terminated by the cubesat due to us receiving the max number of bytes return an error
    return OBC_ERR_CODE_CC1120_RECEIVE_TERMINATED;
  }

  return OBC_ERR_CODE_SUCCESS;
}

/**
 * @brief block until the tx fifo is empty without decrementing the semaphore
 *
 * @return obc_error_code_t - whether the tx fifo empty semaphore became available without timing out or not
 */
obc_error_code_t txFifoEmptyCheckBlocking(void) {
  if (xSemaphoreTake(txFifoEmptySemaphore, TX_FIFO_EMPTY_SEMAPHORE_TIMEOUT) != pdPASS) {
    return OBC_ERR_CODE_SEMAPHORE_TIMEOUT;
  }
  xSemaphoreGive(txFifoEmptySemaphore);
  return OBC_ERR_CODE_SUCCESS;
}

void txFifoReadyCallback(void) {
  BaseType_t xHigherPriorityTaskAwoken = pdFALSE;
  // give semaphore and set xHigherPriorityTaskAwoken to pdTRUE if this unblocks a higher priority task than the current
  // one
  if (xSemaphoreGiveFromISR(txSemaphore, &xHigherPriorityTaskAwoken) != pdPASS) {
    /* TODO: figure out how to log from ISR */
  }
  // if xHigherPriorityTaskAwoken == pdTRUE then request a context switch since this means a higher priority task has
  // been unblocked
  portYIELD_FROM_ISR(xHigherPriorityTaskAwoken);
}

void rxFifoReadyCallback(void) {
  BaseType_t xHigherPriorityTaskAwoken = pdFALSE;
  // give semaphore and set xHigherPriorityTaskAwoken to pdTRUE if this unblocks a higher priority task than the current
  // one
  if (xSemaphoreGiveFromISR(rxSemaphore, &xHigherPriorityTaskAwoken) != pdPASS) {
    /* TODO: figure out how to log from ISR */
  }
  // if xHigherPriorityTaskAwoken == pdTRUE then request a context switch since this means a higher priority task has
  // been unblocked
  portYIELD_FROM_ISR(xHigherPriorityTaskAwoken);
}

void txFifoEmptyCallback(void) {
  BaseType_t xHigherPriorityTaskAwoken = pdFALSE;
  // give semaphore and set xHigherPriorityTaskAwoken to pdTRUE if this unblocks a higher priority task than the current
  // one
  if (xSemaphoreGiveFromISR(txFifoEmptySemaphore, &xHigherPriorityTaskAwoken) != pdPASS) {
    /* TODO: figure out how to log from ISR */
  }
  // if xHigherPriorityTaskAwoken == pdTRUE then request a context switch since this means a higher priority task has
  // been unblocked
  portYIELD_FROM_ISR(xHigherPriorityTaskAwoken);
}

void syncEventCallback(void) {
  BaseType_t xHigherPriorityTaskAwoken = pdFALSE;
  // give semaphore and set xHigherPriorityTaskAwoken to pdTRUE if this unblocks a higher priority task than the current
  // one
  if (xSemaphoreGiveFromISR(syncReceivedSemaphore, &xHigherPriorityTaskAwoken) != pdPASS) {
    /* TODO: figure out how to log from ISR */
  }
  // if xHigherPriorityTaskAwoken == pdTRUE then request a context switch since this means a higher priority task has
  // been unblocked
  portYIELD_FROM_ISR(xHigherPriorityTaskAwoken);
}

SemaphoreHandle_t getCC1120RxSemaphoreHandle(void) { return rxSemaphore; }

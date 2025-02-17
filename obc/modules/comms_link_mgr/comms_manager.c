#include "comms_manager.h"
#include "uplink_decoder.h"
#include "downlink_encoder.h"
#include "comms_uplink_receiver.h"
#include "comms_downlink_transmitter.h"
#include "obc_gs_aes128.h"
#include "obc_gs_fec.h"

#include "obc_errors.h"
#include "obc_logging.h"
#include "obc_task_config.h"
#include "telemetry_manager.h"
#include "telemetry_fs_utils.h"
#include "obc_gs_telemetry_pack.h"
#include "obc_reliance_fs.h"

#include <FreeRTOS.h>
#include <os_portmacro.h>
#include <os_queue.h>
#include <os_task.h>

#include <redposix.h>

#include <sys_common.h>
#include <gio.h>

/* Comms Manager event queue config */
#define COMMS_MANAGER_QUEUE_LENGTH 10U
#define COMMS_MANAGER_QUEUE_ITEM_SIZE sizeof(comms_event_t)
#define COMMS_MANAGER_QUEUE_RX_WAIT_PERIOD pdMS_TO_TICKS(10)
#define COMMS_MANAGER_QUEUE_TX_WAIT_PERIOD pdMS_TO_TICKS(10)

static SemaphoreHandle_t cc1120Mutex = NULL;
static StaticSemaphore_t cc1120MutexBuffer;

static TaskHandle_t commsTaskHandle = NULL;
static StaticTask_t commsTaskBuffer;
static StackType_t commsTaskStack[COMMS_MANAGER_STACK_SIZE];

static QueueHandle_t commsQueueHandle = NULL;
static StaticQueue_t commsQueue;
static uint8_t commsQueueStack[COMMS_MANAGER_QUEUE_LENGTH * COMMS_MANAGER_QUEUE_ITEM_SIZE];

static const uint8_t TEMP_STATIC_KEY[AES_KEY_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                                      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

/**
 * @brief	Comms Manager task.
 * @param	pvParameters	Task parameters.
 */
static void vCommsManagerTask(void *pvParameters);

void initCommsManager(void) {
  ASSERT((commsTaskStack != NULL) && (&commsTaskBuffer != NULL));
  if (commsTaskHandle == NULL) {
    commsTaskHandle = xTaskCreateStatic(vCommsManagerTask, COMMS_MANAGER_NAME, COMMS_MANAGER_STACK_SIZE, NULL,
                                        COMMS_MANAGER_PRIORITY, commsTaskStack, &commsTaskBuffer);
  }

  ASSERT((commsQueueStack != NULL) && (&commsQueue != NULL));
  if (commsQueueHandle == NULL) {
    commsQueueHandle =
        xQueueCreateStatic(COMMS_MANAGER_QUEUE_LENGTH, COMMS_MANAGER_QUEUE_ITEM_SIZE, commsQueueStack, &commsQueue);
  }

  if (cc1120Mutex == NULL) {
    cc1120Mutex = xSemaphoreCreateMutexStatic(&cc1120MutexBuffer);
  }
  // TODO: Implement a key exchange algorithm instead of using Pre-Shared/static key
  initializeAesCtx(TEMP_STATIC_KEY);
  initRs();

  initRecvTask(&cc1120Mutex);
  initDecodeTask();

  initTelemEncodeTask(&cc1120Mutex);
  initCC1120TransmitTask();
}

obc_error_code_t sendToCommsQueue(comms_event_t *event) {
  ASSERT(commsQueueHandle != NULL);

  if (event == NULL) {
    return OBC_ERR_CODE_INVALID_ARG;
  }

  if (xQueueSend(commsQueueHandle, (void *)event, COMMS_MANAGER_QUEUE_TX_WAIT_PERIOD) == pdPASS) {
    return OBC_ERR_CODE_SUCCESS;
  }

  return OBC_ERR_CODE_QUEUE_FULL;
}

static void vCommsManagerTask(void *pvParameters) {
  obc_error_code_t errCode;

  while (1) {
    comms_event_t queueMsg;

    if (xQueueReceive(commsQueueHandle, &queueMsg, COMMS_MANAGER_QUEUE_RX_WAIT_PERIOD) != pdPASS) {
      continue;
    }

    switch (queueMsg.eventID) {
      case DOWNLINK_TELEMETRY_FILE:
        LOG_IF_ERROR_CODE(sendToDownlinkQueue(&queueMsg));
        break;
      case DOWNLINK_DATA_BUFFER:
        LOG_IF_ERROR_CODE(sendToDownlinkQueue(&queueMsg));
        break;
      case BEGIN_UPLINK:
        LOG_IF_ERROR_CODE(startUplink());
        break;
    }
  }
}

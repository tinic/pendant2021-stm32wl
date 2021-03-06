/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lora_app.c
  * @author  MCD Application Team
  * @brief   Application of the LRWAN Middleware
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "Region.h" /* Needed for LORAWAN_DEFAULT_DATA_RATE */
#include "sys_app.h"
#include "lora_app.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "lora_app_version.h"
#include "lorawan_version.h"
#include "subghz_phy_version.h"
#include "lora_info.h"
#include "LmHandler.h"
#include "stm32_lpm.h"
#include "adc_if.h"
#include "sys_conf.h"
#include "CayenneLpp.h"
#include "sys_sensors.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "stm32_lpm.h"
#include "utilities_def.h"
#include "lora_info.h"

#ifdef USER_APP_BUILD
#include "user_app.h"
#endif  // #ifdef USER_APP_BUILD

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief LoRa State Machine states
  */
typedef enum TxEventType_e
{
  /**
    * @brief Appdata Transmission issue based on timer every TxDutyCycleTime
    */
  TX_ON_TIMER,
  /**
    * @brief Appdata Transmission external event plugged on OnSendEvent( )
    */
  TX_ON_EVENT
  /* USER CODE BEGIN TxEventType_t */

  /* USER CODE END TxEventType_t */
} TxEventType_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define DEBUG_MSG
/* USER CODE END PM */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  LoRa End Node send request
  */
static void SendTxData(void);

/**
  * @brief  TX timer callback function
  * @param  context ptr of timer context
  */
static void OnTxTimerEvent(void *context);

/**
  * @brief  join event callback function
  * @param  joinParams status of join
  */
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams);

/**
  * @brief  tx event callback function
  * @param  params status of last Tx
  */
static void OnTxData(LmHandlerTxParams_t *params);

/**
  * @brief callback when LoRa application has received a frame
  * @param appData data received in the last Rx
  * @param params status of last Rx
  */
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);

/*!
 * Will be called each time a Radio IRQ is handled by the MAC layer
 *
 */
static void OnMacProcessNotify(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private variables ---------------------------------------------------------*/
static ActivationType_t ActivationType = LORAWAN_DEFAULT_ACTIVATION_TYPE;

/**
  * @brief LoRaWAN handler Callbacks
  */
static LmHandlerCallbacks_t LmHandlerCallbacks =
{
  .GetBatteryLevel =           GetBatteryLevel,
  .GetTemperature =            GetTemperatureLevel,
  .GetUniqueId =               GetUniqueId,
  .GetDevAddr =                GetDevAddr,
  .OnMacProcess =              OnMacProcessNotify,
  .OnJoinRequest =             OnJoinRequest,
  .OnTxData =                  OnTxData,
  .OnRxData =                  OnRxData
};

/**
  * @brief LoRaWAN handler parameters
  */
static LmHandlerParams_t LmHandlerParams =
{
  .ActiveRegion =             ACTIVE_REGION,
  .DefaultClass =             LORAWAN_DEFAULT_CLASS,
  .AdrEnable =                LORAWAN_ADR_STATE,
  .TxDatarate =               LORAWAN_DEFAULT_DATA_RATE,
  .PingPeriodicity =          LORAWAN_DEFAULT_PING_SLOT_PERIODICITY
};

/**
  * @brief Type of Event to generate application Tx
  */
static TxEventType_t EventType = TX_ON_TIMER;

/**
  * @brief Timer to handle the application Tx
  */
static UTIL_TIMER_Object_t TxTimer;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Exported functions ---------------------------------------------------------*/
/* USER CODE BEGIN EF */

/* USER CODE END EF */

void LoRaWAN_Init(void)
{
  /* USER CODE BEGIN LoRaWAN_Init_1 */

  /* USER CODE END LoRaWAN_Init_1 */

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LmHandlerProcess), UTIL_SEQ_RFU, LmHandlerProcess);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), UTIL_SEQ_RFU, SendTxData);
  /* Init Info table used by LmHandler*/
  LoraInfo_Init();

  /* Init the Lora Stack*/
  LmHandlerInit(&LmHandlerCallbacks);

  LmHandlerConfigure(&LmHandlerParams);

  /* USER CODE BEGIN LoRaWAN_Init_2 */

#ifdef USER_APP_BUILD
  MibRequestConfirm_t mibReq;
  memset(&mibReq, 0, sizeof(mibReq));

  static uint8_t key[16];
  memcpy(key, lora_app_key(), 16);

  mibReq.Type = MIB_NWK_KEY;
  mibReq.Param.NwkKey = key;
  LoRaMacMibSetRequestConfirm(&mibReq);

  mibReq.Type = MIB_APP_KEY;
  mibReq.Param.NwkKey = key;
  LoRaMacMibSetRequestConfirm(&mibReq);

  static uint8_t joinEui[8];
  memset(&joinEui[0], 0, sizeof(joinEui));
  mibReq.Type = MIB_JOIN_EUI;
  mibReq.Param.JoinEui = joinEui;
  LoRaMacMibSetRequestConfirm(&mibReq);
#endif  // #ifdef USER_APP_BUILD

  /* USER CODE END LoRaWAN_Init_2 */

  LmHandlerJoin(ActivationType);

  if (EventType == TX_ON_TIMER)
  {
    /* send every time timer elapses */
    UTIL_TIMER_Create(&TxTimer,  0xFFFFFFFFU, UTIL_TIMER_ONESHOT, OnTxTimerEvent, NULL);
    UTIL_TIMER_SetPeriod(&TxTimer,  APP_TX_DUTYCYCLE);
    UTIL_TIMER_Start(&TxTimer);
  }
  else
  {
    /* USER CODE BEGIN LoRaWAN_Init_3 */
    /* USER CODE END LoRaWAN_Init_3 */
  }

  /* USER CODE BEGIN LoRaWAN_Init_Last */

  /* USER CODE END LoRaWAN_Init_Last */
}

/* USER CODE BEGIN PB_Callbacks */

/* USER CODE END PB_Callbacks */

/* Private functions ---------------------------------------------------------*/
/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
  /* USER CODE BEGIN OnRxData_1 */
	if ((appData != NULL) && (params != NULL)) {
		switch (appData->Port) {
			case LORAWAN_SWITCH_CLASS_PORT: {
				if (appData->BufferSize == 1) {
					switch (appData->Buffer[0]) {
						case 0: {
							LmHandlerRequestClass(CLASS_A);
#ifdef DEBUG_MSG
							printf("CLASS_A!\r\n");
#endif  // #ifdef DEBUG_MSG
						}
						break;
						case 1: {
							LmHandlerRequestClass(CLASS_B);
#ifdef DEBUG_MSG
							printf("CLASS_B!\r\n");
#endif  // #ifdef DEBUG_MSG
						}
						break;
						case 2: {
							LmHandlerRequestClass(CLASS_C);
#ifdef DEBUG_MSG
							printf("CLASS_C!\r\n");
#endif  // #ifdef DEBUG_MSG
						}
						break;
						default: {
							break;
						}
					}
				}
			}
			break;
			case LORAWAN_USER_APP_PORT: {
#ifdef USER_APP_BUILD
				lora_decode_packet(appData->Buffer, appData->BufferSize);
#endif  // #ifdef USER_APP_BUILD
			}
			break;
			default: {
			} break;
		}
	}
  /* USER CODE END OnRxData_1 */
}

static void SendTxData(void)
{
  /* USER CODE BEGIN SendTxData_1 */
	static LmHandlerAppData_t TxData = { LORAWAN_USER_APP_PORT, 0, 0 };
#ifdef USER_APP_BUILD
	TxData.Buffer = (uint8_t *)lora_encode_packet(&TxData.BufferSize, &TxData.Port);
#else  //#ifdef USER_APP_BUILD
	TxData.Buffer = 0;
	TxData.BufferSize = 0;
#endif  // #ifdef PENDANT_BUILD
	UTIL_TIMER_Time_t nextTxIn = 0;
	if (LORAMAC_HANDLER_SUCCESS == LmHandlerSend(&TxData, LORAWAN_DEFAULT_CONFIRMED_MSG_STATE, &nextTxIn, false)) {
#ifdef DEBUG_MSG
		printf("SendTxData Success!\r\n");
	} else if (nextTxIn > 0) {
		printf("SendTxData Early!\r\n");
	} else {
		printf("SendTxData Fail!\r\n");
#endif // #ifdef DEBUG_MSG
	}
  /* USER CODE END SendTxData_1 */
}

static void OnTxTimerEvent(void *context)
{
  /* USER CODE BEGIN OnTxTimerEvent_1 */

  /* USER CODE END OnTxTimerEvent_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);

  /*Wait for next tx slot*/
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxTimerEvent_2 */

  /* USER CODE END OnTxTimerEvent_2 */
}

/* USER CODE BEGIN PrFD_LedEvents */

/* USER CODE END PrFD_LedEvents */

static void OnTxData(LmHandlerTxParams_t *params)
{
  /* USER CODE BEGIN OnTxData_1 */
	if ((params != NULL) && (params->IsMcpsConfirm != 0)) {
		if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG) {
#ifdef DEBUG_MSG
			printf("OnTxData confirmed!!\r\n");
		} else {
			printf("OnTxData unconfirmed!\r\n");
#endif // #ifdef DEBUG_MSG
		}
	}
  /* USER CODE END OnTxData_1 */
}

static void OnJoinRequest(LmHandlerJoinParams_t *joinParams)
{
  /* USER CODE BEGIN OnJoinRequest_1 */
	if (joinParams != NULL) {
		if (joinParams->Status == LORAMAC_HANDLER_SUCCESS) {
			if (joinParams->Mode == ACTIVATION_TYPE_OTAA) {
#ifdef DEBUG_MSG
				printf("OnJoinRequest confirmed!!\r\n");
#endif  // #ifdef DEBUG_MSG
			}
		} else {
#ifdef DEBUG_MSG
			printf("OnJoinRequest retry\r\n");
#endif  // #ifdef DEBUG_MSG
		}
	}
  /* USER CODE END OnJoinRequest_1 */
}

static void OnMacProcessNotify(void)
{
  /* USER CODE BEGIN OnMacProcessNotify_1 */

  /* USER CODE END OnMacProcessNotify_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LmHandlerProcess), CFG_SEQ_Prio_0);

  /* USER CODE BEGIN OnMacProcessNotify_2 */

  /* USER CODE END OnMacProcessNotify_2 */
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

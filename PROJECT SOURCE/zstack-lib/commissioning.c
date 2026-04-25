//==============================================================================
// commissioning.c
//==============================================================================
// ZigBee device commissioning and network recovery implementation
//==============================================================================

#include "commissioning.h"
#include "OSAL_PwrMgr.h"
#include "ZDApp.h"
#include "bdb_interface.h"
#include "hal_key.h"
#include "zcl_app.h"

//==============================================================================
// Static Function Prototypes
//==============================================================================
static void zclCommissioning_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);
static void zclCommissioning_ResetBackoffRetry(void);
static void zclCommissioning_OnConnect(void);
static void zclCommissioning_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg);
static void zclCommissioning_BindNotification(bdbBindNotificationData_t *data);

//==============================================================================
// External Variables
//==============================================================================
extern bool requestNewTrustCenterLinkKey;

//==============================================================================
// Global Variables
//==============================================================================
byte rejoinsLeft = APP_COMMISSIONING_END_DEVICE_REJOIN_TRIES;
uint32 rejoinDelay = APP_COMMISSIONING_END_DEVICE_REJOIN_START_DELAY;
uint8 zclCommissioning_TaskId = 0;

//==============================================================================
// Default Configuration
//==============================================================================
#ifndef APP_TX_POWER
    #define APP_TX_POWER TX_PWR_PLUS_4
#endif

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Initialize commissioning module
 * @param task_id Task ID for event handling
 */
void zclCommissioning_Init(uint8 task_id)
{
    zclCommissioning_TaskId = task_id;

    bdb_RegisterCommissioningStatusCB(zclCommissioning_ProcessCommissioningStatus);
    bdb_RegisterBindNotificationCB(zclCommissioning_BindNotification);

    ZMacSetTransmitPower(APP_TX_POWER);

    // Important to allow connections through routers.
    // Coordinator should be compiled with #define TP2_LEGACY_ZC
    requestNewTrustCenterLinkKey = FALSE;

    bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING);
}

/**
 * @brief Control sleep/polling behavior for power saving
 * @param allow TRUE to enable power saving, FALSE to disable
 */
void zclCommissioning_Sleep(uint8 allow)
{
#if defined(POWER_SAVING)
    if (allow) {
        NLME_SetPollRate(0);      // Disable polling to save power
    } else {
        NLME_SetPollRate(POLL_RATE);  // Restore normal polling rate
    }
#endif
}

/**
 * @brief Main event loop handler
 * @param task_id Task ID
 * @param events  Pending events
 * @return Remaining events
 */
uint16 zclCommissioning_event_loop(uint8 task_id, uint16 events)
{
    if (events & SYS_EVENT_MSG) {
        devStates_t zclApp_NwkState;
        afIncomingMSGPacket_t *MSGpkt;

        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclCommissioning_TaskId))) {

            switch (MSGpkt->hdr.event) {
            case ZDO_STATE_CHANGE:
                zclApp_NwkState = (devStates_t)(MSGpkt->hdr.status);

                if (zclApp_NwkState == DEV_ROUTER ||
                    zclApp_NwkState == DEV_END_DEVICE ||
                    zclApp_NwkState == DEV_END_DEVICE_UNAUTH)
                {
                    // Successfully connected to network
                    zclCommissioning_ResetBackoffRetry();
                    HAL_TURN_OFF_LED1();
                }
                else if (zclApp_NwkState == DEV_NWK_ORPHAN ||
                         zclApp_NwkState == DEV_NWK_DISC ||
                         zclApp_NwkState == DEV_NWK_BACKOFF)
                {
                    // Lost connection - attempt to recover
                    if (rejoinsLeft > 0 && zclApp_NwkState != DEV_NWK_BACKOFF) {
                        osal_start_timerEx(zclCommissioning_TaskId,
                                         APP_COMMISSIONING_END_DEVICE_REJOIN_EVT,
                                         rejoinDelay);
                    }
                }
                break;

            case ZCL_INCOMING_MSG:
                zclCommissioning_ProcessIncomingMsg((zclIncomingMsg_t *)MSGpkt);
                break;

            default:
                break;
            }

            osal_msg_deallocate((uint8 *)MSGpkt);
        }

        return (events ^ SYS_EVENT_MSG);
    }

    if (events & APP_COMMISSIONING_END_DEVICE_REJOIN_EVT) {
#if ZG_BUILD_ENDDEVICE_TYPE
        bdb_ZedAttemptRecoverNwk();
        osal_start_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_CLOCK_DOWN_POLING_RATE_EVT, 60000);
#endif
        return (events ^ APP_COMMISSIONING_END_DEVICE_REJOIN_EVT);
    }

    if (events & APP_COMMISSIONING_CLOCK_DOWN_POLING_RATE_EVT) {
        zclCommissioning_Sleep(TRUE);
        return (events ^ APP_COMMISSIONING_CLOCK_DOWN_POLING_RATE_EVT);
    }

#if APP_COMMISSIONING_BY_LONG_PRESS == TRUE
    if (events & APP_COMMISSIONING_BY_LONG_PRESS_EVT) {
        HAL_TURN_ON_LED1();
        osal_start_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_OFF_EVT, 14000);
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING);
        return (events ^ APP_COMMISSIONING_BY_LONG_PRESS_EVT);
    }

    if (events & APP_COMMISSIONING_OFF_EVT) {
        HAL_TURN_OFF_LED1();
        return (events ^ APP_COMMISSIONING_OFF_EVT);
    }
#endif

    return 0;
}

/**
 * @brief Handle key press events for commissioning
 * @param portAndAction Port and action mask
 * @param keyCode       Key code (unused)
 */
void zclCommissioning_HandleKeys(uint8 portAndAction, uint8 keyCode)
{
    // If already on network - ignore commissioning
    if (devState == DEV_ROUTER ||
        devState == DEV_END_DEVICE ||
        devState == DEV_END_DEVICE_UNAUTH)
    {
        osal_stop_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_BY_LONG_PRESS_EVT);
        return;
    }

    if (portAndAction & HAL_KEY_PRESS) {
#if ZG_BUILD_ENDDEVICE_TYPE
        if (devState == DEV_NWK_ORPHAN) {
            bdb_ZedAttemptRecoverNwk();
        }
#else
        if (devState == DEV_NWK_ORPHAN || devState == DEV_NWK_DISC) {
            bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING);
        }
#endif
    }

#if APP_COMMISSIONING_BY_LONG_PRESS == TRUE
    if (portAndAction & HAL_KEY_RELEASE) {
        osal_stop_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_BY_LONG_PRESS_EVT);
    } else {
        bool statTimer = TRUE;

#if APP_COMMISSIONING_BY_LONG_PRESS_PORT
        statTimer = (APP_COMMISSIONING_BY_LONG_PRESS_PORT & portAndAction) != 0;
#endif

        if (statTimer &&
            devState != DEV_ROUTER &&
            devState != DEV_END_DEVICE &&
            devState != DEV_END_DEVICE_UNAUTH &&
            devState != DEV_NWK_JOINING &&
            devState != DEV_NWK_SEC_REJOIN_CURR_CHANNEL &&
            devState != DEV_NWK_SEC_REJOIN_ALL_CHANNEL)
        {
            uint32 timeout = APP_COMMISSIONING_HOLD_TIME_FAST;
            osal_start_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_BY_LONG_PRESS_EVT, timeout);
        }
    }
#endif
}

//==============================================================================
// Static Functions
//==============================================================================

/**
 * @brief Reset backoff retry counters to default values
 */
static void zclCommissioning_ResetBackoffRetry(void)
{
    rejoinsLeft = APP_COMMISSIONING_END_DEVICE_REJOIN_TRIES;
    rejoinDelay = APP_COMMISSIONING_END_DEVICE_REJOIN_START_DELAY;
}

/**
 * @brief Callback when successfully connected to network
 */
static void zclCommissioning_OnConnect(void)
{
    zclCommissioning_ResetBackoffRetry();
    osal_start_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_CLOCK_DOWN_POLING_RATE_EVT, 10 * 1000);
}

/**
 * @brief Process commissioning status callbacks from BDB
 * @param bdbCommissioningModeMsg Commissioning status message
 */
static void zclCommissioning_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg)
{
    switch (bdbCommissioningModeMsg->bdbCommissioningMode) {
    case BDB_COMMISSIONING_INITIALIZATION:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_NO_NETWORK:
            break;
        case BDB_COMMISSIONING_NETWORK_RESTORED:
            zclCommissioning_OnConnect();
            break;
        default:
            break;
        }
        break;

    case BDB_COMMISSIONING_NWK_STEERING:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_SUCCESS:
            zclCommissioning_OnConnect();
            HAL_TURN_OFF_LED1();
            break;
        default:
            break;
        }
        break;

    case BDB_COMMISSIONING_PARENT_LOST:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_NETWORK_RESTORED:
            zclCommissioning_ResetBackoffRetry();
            break;
        default:
            if (rejoinsLeft > 0) {
                rejoinDelay = (uint32)((float)rejoinDelay * APP_COMMISSIONING_END_DEVICE_REJOIN_BACKOFF);
                rejoinsLeft = rejoinsLeft - 1;
            } else {
                rejoinDelay = APP_COMMISSIONING_END_DEVICE_REJOIN_MAX_DELAY;
                zclCommissioning_ResetBackoffRetry();
            }
            osal_start_timerEx(zclCommissioning_TaskId, APP_COMMISSIONING_END_DEVICE_REJOIN_EVT, rejoinDelay);
            break;
        }
        break;

    default:
        break;
    }
}

/**
 * @brief Process incoming ZCL message and free attribute command memory
 * @param pInMsg Incoming ZCL message
 */
static void zclCommissioning_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg)
{
    if (pInMsg->attrCmd) {
        osal_mem_free(pInMsg->attrCmd);
    }
}

/**
 * @brief Bind notification callback
 * @param data Bind notification data
 */
static void zclCommissioning_BindNotification(bdbBindNotificationData_t *data)
{
    uint16 maxEntries = 0, usedEntries = 0;
    bindCapacity(&maxEntries, &usedEntries);
}
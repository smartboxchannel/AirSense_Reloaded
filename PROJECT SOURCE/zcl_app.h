//==============================================================================
// zcl_app.h
//==============================================================================
// ZCL Application definitions and configuration
//==============================================================================

#ifndef ZCL_APP_H
#define ZCL_APP_H

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// Includes
//==============================================================================

#include "zcl.h"

//==============================================================================
// Constants
//==============================================================================

// Application Events
#define APP_REPORT_EVT          0x0001
#define APP_SAVE_ATTRS_EVT      0x0002
#define APP_READ_SENSORS_EVT    0x0004
#define APP_REQ_SENSORS_EVT     0x0008
#define APP_CO2_EVT             0x0010
#define APP_REP_SENSORS_EVT     0x0020
#define APP_LED_EVT             0x0040
#define APP_IDENT_EVT           0x0080

//==============================================================================
// Macros
//==============================================================================

#define NW_APP_CONFIG   0x0402

// Access control macros for ZCL attributes
#define R   ACCESS_CONTROL_READ
#define RW  (R | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_WRITE)
#define RR  (R | ACCESS_REPORTABLE)

// Cluster ID aliases
#define BASIC           ZCL_CLUSTER_ID_GEN_BASIC
#define GEN_ON_OFF      ZCL_CLUSTER_ID_GEN_ON_OFF
#define POWER_CFG       ZCL_CLUSTER_ID_GEN_ON
#define TEMP            ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT
#define HUMIDITY        ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY
#define PRESSURE        ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT
#define IDENTIFY        ZCL_CLUSTER_ID_GEN_IDENTIFY

// Custom cluster IDs
#define ZCL_CO2             0x040D
#define ZCL_FORMALDEHYDE    0x042B

//==============================================================================
// CO2 Cluster Attribute IDs
//==============================================================================

#define ATTRID_CO2_MEASURED_VALUE           0x0000
#define ATTRID_CO2_ACCURATE_MEASURED_VALUE  0xF111
#define ATTRID_ENABLE_ABC                   0x0202
#define ATTRID_LED_FEEDBACK                 0x0203
#define ATTRID_THRESHOLD1                   0x0204
#define ATTRID_THRESHOLD2                   0x0205

// Offset attributes (shared ID across clusters)
#define ATTRID_TemperatureOffset    0x0210
#define ATTRID_PressureOffset       0x0210
#define ATTRID_HumidityOffset       0x0210
#define ATTRID_FORMALDEHYDE_Offset  0x0210

// Formaldehyde Cluster Attribute IDs
#define ATTRID_FORMALDEHYDE_MEASURED_VALUE   0x0000


//==============================================================================
// ZCL Data Type Aliases
//==============================================================================

#define ZCL_BOOL    ZCL_DATATYPE_BOOLEAN
#define ZCL_UINT8   ZCL_DATATYPE_UINT8
#define ZCL_UINT16  ZCL_DATATYPE_UINT16
#define ZCL_INT16   ZCL_DATATYPE_INT16
#define ZCL_INT8    ZCL_DATATYPE_INT8
#define ZCL_INT32   ZCL_DATATYPE_INT32
#define ZCL_UINT32  ZCL_DATATYPE_UINT32
#define ZCL_SINGLE  ZCL_DATATYPE_SINGLE_PREC

//==============================================================================
// Constants
//==============================================================================

#define DEFAULT_IDENTIFY_TIME   0

//==============================================================================
// Typedefs
//==============================================================================

/**
 * @brief Application configuration structure
 */
typedef struct {
    uint8   LedFeedback;
    uint8   EnableABC;
    uint16  Threshold1_PPM;
    uint16  Threshold2_PPM;
    uint16  Threshold1_CH2O_ppb;
    uint16  Threshold2_CH2O_ppb;
    int16   TemperatureOffset;
    int32   PressureOffset;
    int16   HumidityOffset;
    bool    accurate_co2;
    int8    ch2oOffset;
} application_config_t;

/**
 * @brief Sensor state structure
 */
typedef struct {
    float   CO2;
    int16   CO2_PPM;
    int16   Temperature;
    int16   BME280_Temperature_Sensor_MeasuredValue;
    uint16  BME280_HumiditySensor_MeasuredValue;
    int16   BME280_PressureSensor_MeasuredValue;
    int16   BME280_PressureSensor_ScaledValue;
    int8    BME280_PressureSensor_Scale;
    int16   BME280_PressureSensor_raw;
    float   ch2o;
    float   ch2o2;
} sensors_state_t;

//==============================================================================
// External Variables
//==============================================================================

extern uint8 zclApp_StateText[];
extern uint16 tempCH20;

extern SimpleDescriptionFormat_t zclApp_FirstEP;
extern CONST zclAttrRec_t zclApp_AttrsFirstEP[];
extern CONST uint8 zclApp_AttrsCount;

extern uint16 zclApp_IdentifyTime;
extern const uint8 zclApp_ManufacturerName[];
extern const uint8 zclApp_ModelId[];
extern const uint8 zclApp_DateCode[];
extern const uint8 zclApp_SWBuildID[];
extern const uint8 zclApp_PowerSource;

extern application_config_t zclApp_Config;
extern sensors_state_t zclApp_Sensors;
extern int16 sendInitReportCount;

//==============================================================================
// Function Prototypes
//==============================================================================

/**
 * @brief Initialize ZCL application task
 * @param task_id Task ID for event handling
 */
extern void zclApp_Init(byte task_id);

/**
 * @brief Main event loop for ZCL application
 * @param task_id Task ID
 * @param events  Pending events
 * @return Remaining events
 */
extern UINT16 zclApp_event_loop(byte task_id, UINT16 events);

/**
 * @brief Reset all configurable attributes to default values
 */
extern void zclApp_ResetAttributesToDefaultValues(void);

//==============================================================================

#ifdef __cplusplus
}
#endif

#endif /* ZCL_APP_H */
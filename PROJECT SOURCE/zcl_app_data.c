//==============================================================================
// zcl_app_data.c
//==============================================================================
// ZCL application data structures and attributes
//==============================================================================

#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_ms.h"

#include "zcl_app.h"

#include "bdb_touchlink.h"
#include "bdb_touchlink_target.h"
#include "stub_aps.h"

//==============================================================================
// Constants
//==============================================================================

#define APP_DEVICE_VERSION    2
#define APP_FLAGS             0

#define APP_HWVERSION         1
#define APP_ZCLVERSION        1

//==============================================================================
// Global Variables
//==============================================================================

// Cluster revision
const uint16 zclApp_clusterRevision_all = 0x0002;

// Basic Cluster attributes
const uint8 zclApp_HWRevision         = APP_HWVERSION;
const uint8 zclApp_ZCLVersion         = APP_ZCLVERSION;
const uint8 zclApp_ApplicationVersion = 3;
const uint8 zclApp_StackVersion       = 4;

// Basic Cluster string attributes (format: length byte followed by string data)
const uint8 zclApp_ManufacturerName[] = {9, 'E', 'f', 'e', 'k', 't', 'a', 'L', 'a', 'b'};
const uint8 zclApp_ModelId[]          = {24, 'D', 'I', 'Y', 'R', 'u', 'Z', '_', 'A', 'i', 'r', 'S', 'e', 'n', 's', 'e', '_', 'R', 'e', 'l', 'o', 'a', 'd', 'e', 'd'};
const uint8 zclApp_DateCode[]         = {15, '2', '0', '2', '6', '0', '2', '1', '8', ' ', '6', '4', '3', ' ', '7', '7'};
const uint8 zclApp_SWBuildID[]        = {5,  '4', '.', '0', '.', '6'};
const uint8 zclApp_PowerSource        = POWER_SOURCE_DC;

// Default configuration values
#define DEFAULT_LedFeedback     TRUE
#define DEFAULT_EnableABC       TRUE
// Reference: https://www.kane.co.uk/knowledge-centre/what-are-safe-levels-of-co-and-co2-in-rooms
#define DEFAULT_Threshold1_PPM  1000
#define DEFAULT_Threshold2_PPM  2000
#define DEFAULT_TemperatureOffset 0
#define DEFAULT_PressureOffset    0
#define DEFAULT_HumidityOffset    0

// State buffer
uint8 zclApp_StateText[15];

// Temporary formaldehyde variables
uint16 tempCH20   = 0;
float  tempCH20_2 = 0;

// Application configuration
application_config_t zclApp_Config = {
    .LedFeedback        = DEFAULT_LedFeedback,
    .EnableABC          = DEFAULT_EnableABC,
    .Threshold1_PPM     = DEFAULT_Threshold1_PPM,
    .Threshold2_PPM     = DEFAULT_Threshold2_PPM,
    .Threshold1_CH2O_ppb = 50,
    .Threshold2_CH2O_ppb = 100,
    .TemperatureOffset  = DEFAULT_TemperatureOffset,
    .PressureOffset     = DEFAULT_PressureOffset,
    .HumidityOffset     = DEFAULT_HumidityOffset,
    .accurate_co2       = 0,
    .ch2oOffset         = 0,
};

// Sensor state
sensors_state_t zclApp_Sensors = {
    .CO2                                   = 0.0,
    .CO2_PPM                               = 0,
    .Temperature                           = 0,
    .BME280_PressureSensor_MeasuredValue   = 0,
    .BME280_HumiditySensor_MeasuredValue   = 0,
    .BME280_PressureSensor_ScaledValue     = 0,
    .BME280_PressureSensor_Scale           = -1,
    .BME280_PressureSensor_raw             = 0,
    .ch2o                                  = 0,
    .ch2o2                                 = 0
};

// Identify time (seconds)
uint16 zclApp_IdentifyTime = DEFAULT_IDENTIFY_TIME;

//==============================================================================
// ZCL Attribute Definitions
//==============================================================================

CONST zclAttrRec_t zclApp_AttrsFirstEP[] = {
    // Basic Cluster
    {BASIC, {ATTRID_BASIC_ZCL_VERSION,         ZCL_UINT8,      R,  (void *)&zclApp_ZCLVersion}},
    {BASIC, {ATTRID_BASIC_APPL_VERSION,        ZCL_UINT8,      R,  (void *)&zclApp_ApplicationVersion}},
    {BASIC, {ATTRID_BASIC_STACK_VERSION,       ZCL_UINT8,      R,  (void *)&zclApp_StackVersion}},
    {BASIC, {ATTRID_BASIC_HW_VERSION,          ZCL_UINT8,      R,  (void *)&zclApp_HWRevision}},
    {BASIC, {ATTRID_BASIC_MANUFACTURER_NAME,   ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ManufacturerName}},
    {BASIC, {ATTRID_BASIC_MODEL_ID,            ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ModelId}},
    {BASIC, {ATTRID_BASIC_DATE_CODE,           ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_BASIC_POWER_SOURCE,        ZCL_DATATYPE_ENUM8, R, (void *)&zclApp_PowerSource}},
    {BASIC, {ATTRID_BASIC_SW_BUILD_ID,         ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_SWBuildID}},
    {BASIC, {ATTRID_CLUSTER_REVISION,          ZCL_UINT16,     R,  (void *)&zclApp_clusterRevision_all}},

    // Identify Cluster
    {IDENTIFY, {ATTRID_IDENTIFY_TIME,          ZCL_DATATYPE_UINT16, RW, (void *)&zclApp_IdentifyTime}},
    {IDENTIFY, {ATTRID_CLUSTER_REVISION,       ZCL_DATATYPE_UINT16, R,  (void *)&zclApp_clusterRevision_all}},

    // Temperature Measurement Cluster
    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_INT16,   RR, (void *)&zclApp_Sensors.Temperature}},
    {TEMP, {ATTRID_TemperatureOffset,             ZCL_INT16,   RW, (void *)&zclApp_Config.TemperatureOffset}},

    // Pressure Measurement Cluster
    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE, ZCL_INT16, RR, (void *)&zclApp_Sensors.BME280_PressureSensor_MeasuredValue}},
    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_SCALED_VALUE,   ZCL_INT16, RR, (void *)&zclApp_Sensors.BME280_PressureSensor_ScaledValue}},
    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_SCALE,          ZCL_INT8,  R,  (void *)&zclApp_Sensors.BME280_PressureSensor_Scale}},
    {PRESSURE, {ATTRID_PressureOffset,                         ZCL_INT32, RW, (void *)&zclApp_Config.PressureOffset}},

    // Humidity Measurement Cluster
    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclApp_Sensors.BME280_HumiditySensor_MeasuredValue}},
    {HUMIDITY, {ATTRID_HumidityOffset,                      ZCL_INT16,  RW, (void *)&zclApp_Config.HumidityOffset}},

    // CO2 Cluster (custom)
    {ZCL_CO2, {ATTRID_CO2_MEASURED_VALUE,           ZCL_SINGLE, RR, (void *)&zclApp_Sensors.CO2}},
    {ZCL_CO2, {ATTRID_ENABLE_ABC,                   ZCL_BOOL,   RW, (void *)&zclApp_Config.EnableABC}},
    {ZCL_CO2, {ATTRID_LED_FEEDBACK,                 ZCL_BOOL,   RW, (void *)&zclApp_Config.LedFeedback}},
    {ZCL_CO2, {ATTRID_THRESHOLD1,                   ZCL_UINT16, RW, (void *)&zclApp_Config.Threshold1_PPM}},
    {ZCL_CO2, {ATTRID_THRESHOLD2,                   ZCL_UINT16, RW, (void *)&zclApp_Config.Threshold2_PPM}},
    {ZCL_CO2, {ATTRID_CO2_ACCURATE_MEASURED_VALUE,  ZCL_BOOL,   RW, (void *)&zclApp_Config.accurate_co2}},

    // Formaldehyde Cluster (custom)
    {ZCL_FORMALDEHYDE, {ATTRID_FORMALDEHYDE_MEASURED_VALUE, ZCL_SINGLE, RR, (void *)&zclApp_Sensors.ch2o}},
    {ZCL_FORMALDEHYDE, {ATTRID_FORMALDEHYDE_Offset,         ZCL_INT8,   RW, (void *)&zclApp_Config.ch2oOffset}},
    {ZCL_FORMALDEHYDE, {ATTRID_THRESHOLD1,                  ZCL_UINT16, RW, (void *)&zclApp_Config.Threshold1_CH2O_ppb}},
    {ZCL_FORMALDEHYDE, {ATTRID_THRESHOLD2,                  ZCL_UINT16, RW, (void *)&zclApp_Config.Threshold2_CH2O_ppb}},
};

// Number of attributes
uint8 CONST zclApp_AttrsCount = (sizeof(zclApp_AttrsFirstEP) / sizeof(zclApp_AttrsFirstEP[0]));

//==============================================================================
// Endpoint Configuration
//==============================================================================

// Input cluster list
const cId_t zclApp_InClusterList[] = {
    ZCL_CLUSTER_ID_GEN_BASIC,
    IDENTIFY,
    TEMP,
    HUMIDITY,
    PRESSURE,
    ZCL_CO2,
    ZCL_FORMALDEHYDE
};

#define APP_MAX_INCLUSTERS (sizeof(zclApp_InClusterList) / sizeof(zclApp_InClusterList[0]))

// Output cluster list
const cId_t zclApp_OutClusterList[] = {
    ZCL_CO2,
    ZCL_FORMALDEHYDE
};

#define APP_MAX_OUT_CLUSTERS (sizeof(zclApp_OutClusterList) / sizeof(zclApp_OutClusterList[0]))

// Simple descriptor for endpoint 1
SimpleDescriptionFormat_t zclApp_FirstEP = {
    1,                                 // Endpoint
    ZCL_HA_PROFILE_ID,                 // Application profile ID
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,     // Device ID
    APP_DEVICE_VERSION,                // Device version
    APP_FLAGS,                         // Flags
    APP_MAX_INCLUSTERS,                // Number of input clusters
    (cId_t *)zclApp_InClusterList,     // Input cluster list
    APP_MAX_OUT_CLUSTERS,              // Number of output clusters
    (cId_t *)zclApp_OutClusterList     // Output cluster list
};

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Reset all configurable attributes to their default values
 */
void zclApp_ResetAttributesToDefaultValues(void)
{
    zclApp_Config.LedFeedback        = DEFAULT_LedFeedback;
    zclApp_Config.EnableABC          = DEFAULT_EnableABC;
    zclApp_Config.Threshold1_PPM     = DEFAULT_Threshold1_PPM;
    zclApp_Config.Threshold2_PPM     = DEFAULT_Threshold2_PPM;
    zclApp_Config.TemperatureOffset  = DEFAULT_TemperatureOffset;
    zclApp_Config.HumidityOffset     = DEFAULT_HumidityOffset;
    zclApp_Config.PressureOffset     = DEFAULT_PressureOffset;
    zclApp_IdentifyTime              = DEFAULT_IDENTIFY_TIME;
}
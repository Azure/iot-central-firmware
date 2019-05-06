#ifndef iotc_sample_h
#define iotc_sample_h

extern "C" {
    int isBG96Online();
    uint16_t VL53L0X_PROXIMITY_GetDistance(void);
    void VL53L0X_PROXIMITY_Init(void);
    void iotc_create_task();

    typedef enum
    {
    LED2 = 0,
    LED_GREEN = LED2,
    }Led_TypeDef;

    typedef enum
    {
    BL_LED1       = 0xFFFF,
    BL_LED2       = LED2,
    BL_LED_ORANGE = 0xFFFF,
    BL_LED_GREEN  = LED2
    } BL_Led_t;
    void BL_LED_Init(BL_Led_t led);
    void BL_LED_Off(BL_Led_t led);
    void BL_LED_On(BL_Led_t led);

    typedef enum
    {
        ACCELERO_OK = 0,
        ACCELERO_ERROR = 1,
        ACCELERO_TIMEOUT = 2
    }
    ACCELERO_StatusTypeDef;

    typedef enum
    {
        MAGNETO_OK = 0,
        MAGNETO_ERROR = 1,
        MAGNETO_TIMEOUT = 2
    }
    MAGNETO_StatusTypeDef;

    ACCELERO_StatusTypeDef BSP_ACCELERO_Init(void);
    void BSP_ACCELERO_AccGetXYZ(int16_t *pDataXYZ);

    MAGNETO_StatusTypeDef BSP_MAGNETO_Init(void);
    void BSP_MAGNETO_GetXYZ(int16_t *pDataXYZ);

    uint32_t BSP_PSENSOR_Init(void);
    float    BSP_PSENSOR_ReadPressure(void);
    uint8_t BSP_GYRO_Init(void);
    void BSP_GYRO_GetXYZ(float* pfData);

    typedef enum
    {
    TSENSOR_OK = 0,
    TSENSOR_ERROR
    }TSENSOR_Status_TypDef;

    uint32_t BSP_TSENSOR_Init(void);
    float BSP_TSENSOR_ReadTemp(void);

    typedef enum
    {
    HSENSOR_OK = 0,
    HSENSOR_ERROR
    }HSENSOR_Status_TypDef;

    uint32_t BSP_HSENSOR_Init(void);
    float    BSP_HSENSOR_ReadHumidity(void);
}

#endif

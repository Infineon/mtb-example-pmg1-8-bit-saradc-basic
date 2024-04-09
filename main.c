/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the PMG1 8-bit SAR ADC basic Example
*              for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2022-2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "cybsp.h"
#include "cy_pdl.h"
#include "stdio.h"
#include <inttypes.h>

/*******************************************************************************
* Macros
*******************************************************************************/
#define CY_ASSERT_FAILED          (0u)
#define DELAY_MS                  (500u)
#define GPIO_HSIOM                HSIOM_SEL_AMUXA
#define HSIOM_STATE_GPIO          HSIOM_SEL_GPIO
#define ADC_VREF                  CY_USBPD_ADC_VREF_PROG /* Replace with CY_USBPD_ADC_VREF_VDDD to use VDDD (3.3V) as the Vref for the 8-bit ADC*/

/* Debug print macro to enable the UART print */
#define DEBUG_PRINT               (0u)

/* Part of USBPD driver initialization */
cy_stc_pd_dpm_config_t* get_dpm_connect_stat()
{
    return NULL;    /* This value is not required here, hence NULL is returned */
}

/* Structure for UART context */
cy_stc_scb_uart_context_t UART_context;

#if DEBUG_PRINT
/* Variable used for tracking the print status */
volatile bool ENTER_LOOP = true;

/*******************************************************************************
* Function Name: check_status
********************************************************************************
* Summary:
*  Prints the error message.
*
* Parameters:
*  error_msg - message to print if any error encountered.
*  status - status obtained after evaluation.
*
* Return:
*  void
*
*******************************************************************************/
void check_status(char *message, cy_rslt_t status)
{
    char error_msg[50];

    sprintf(error_msg, "Error Code: 0x%08" PRIX32 "\n", status);

    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\n=====================================================\r\n");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\nFAIL: ");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, message);
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\n");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, error_msg);
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\n=====================================================\r\n");
}
#endif


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  System entrance point. This function performs
*  - initial setup of device
*  - initialize UART_SCB, USBPD driver
*  - read ADC value and convert to corresponding voltage based on the PMG1 device and Vref value chosen
*  - send the ADC data through UART_SCB
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    cy_stc_usbpd_context_t USBPD;              /* USBPD context */
    cy_en_usbpd_status_t usbpd_result;

    /* Variables to store ADC result */
    uint8_t ADC_val;
    int ADC_voltage;

    /* Variables to store ADC data for UART display */
    char_t ADC_string[80];

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution */
    if(result != CY_RSLT_SUCCESS)
    {
        /* Insert error handler here */
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* Configure and enable the UART peripheral */
    Cy_SCB_UART_Init(CYBSP_UART_HW, &CYBSP_UART_config, &UART_context);
    Cy_SCB_UART_Enable(CYBSP_UART_HW);

#if DEBUG_PRINT
    /* Sequence to clear screen */
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\x1b[2J\x1b[;H");

    /* Print "8-bit SAR ADC basic" */
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "****************** ");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "PMG1 MCU: 8-bit SAR ADC basic");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "****************** \r\n\n");
#endif

    /* To set data field in USBPD context structure to NULL.
     * Required for uninterrupted USBPD driver initialization */
    memset((void *)&USBPD, 0, sizeof (cy_stc_usbpd_context_t));

    /* Initialize the USBPD driver */
#if defined(CY_DEVICE_CCG3)
    usbpd_result = Cy_USBPD_Init(&USBPD, 0, mtb_usbpd_port0_HW, NULL,
                 (cy_stc_usbpd_config_t *)&mtb_usbpd_port0_config, get_dpm_connect_stat);
#else
    usbpd_result = Cy_USBPD_Init(&USBPD, 0, mtb_usbpd_port0_HW, mtb_usbpd_port0_HW_TRIM,
                 (cy_stc_usbpd_config_t *)&mtb_usbpd_port0_config, get_dpm_connect_stat);
#if PMG1_PD_DUALPORT_ENABLE
    usbpd_result = Cy_USBPD_Init(&USBPD, 1, mtb_usbpd_port1_HW, mtb_usbpd_port1_HW_TRIM,
                 (cy_stc_usbpd_config_t *)&mtb_usbpd_port1_config, get_dpm_port1_connect_stat);
#endif /* PMG1_PD_DUALPORT_ENABLE */
#endif

    if (usbpd_result != CY_USBPD_STAT_SUCCESS )
    {
#if DEBUG_PRINT
        check_status("API Cy_USBPD_Init failed with error code", usbpd_result);
#endif
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Retrieve the HSIOM connection to GPIO port */
    if(HSIOM_STATE_GPIO == Cy_GPIO_GetHSIOM(CYBSP_AMUX_A_PORT, CYBSP_AMUX_A_NUM))
    {
        /* Connect GPIO to AMUXA */
        Cy_GPIO_SetHSIOM(CYBSP_AMUX_A_PORT, CYBSP_AMUX_A_NUM, GPIO_HSIOM);
    }

    /* Enables the PD block and the registers required for ADC operation */
    usbpd_result = Cy_USBPD_Adc_Init(&USBPD, CY_USBPD_ADC_ID_0);
    if (usbpd_result != CY_USBPD_STAT_SUCCESS)
    {
#if DEBUG_PRINT
        check_status("API Cy_USBPD_Adc_Init failed with error code", usbpd_result);
#endif
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* PMG1-S2 device has only VDDD (3.3V) as the default reference voltage */
#if (defined(CY_DEVICE_CCG3PA) || defined(CY_DEVICE_CCG6) || defined(CY_DEVICE_PMG1S3))
    usbpd_result = Cy_USBPD_Adc_SelectVref(&USBPD, CY_USBPD_ADC_ID_0, ADC_VREF);
    /* PMG1-S0,S1,S3 devices have both VDDD (3.3V) and 2.0V from RefGen block as the reference voltages */
    /* Vref = 2.0V from RefGen block is the default value of reference voltage used */
#endif
    if (usbpd_result != CY_USBPD_STAT_SUCCESS)
    {
#if DEBUG_PRINT
        check_status("API Cy_USBPD_Adc_SelectVref failed with error code", usbpd_result);
#endif
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* Send a string over serial terminal */
    Cy_SCB_UART_PutString(CYBSP_UART_HW,"\n\n\nDisplaying the measured voltage & 8-bit SAR ADC result:\r\n");
    Cy_SCB_UART_PutString(CYBSP_UART_HW,"--------------------------------------------------------\r\n\n");

    for(;;)
    {
        /* Calibrate ADC volts per division value before taking any ADC readings; VDDD supply level could vary across time */
        Cy_USBPD_Adc_Calibrate(&USBPD, CY_USBPD_ADC_ID_0);        

        /* Enables the ADC block to function as an ADC and returns the sample value in ADC units */
        /* AMUXA line is used as the ADC input source */
        ADC_val = Cy_USBPD_Adc_Sample(&USBPD, CY_USBPD_ADC_ID_0, CY_USBPD_ADC_INPUT_AMUX_A);

#if defined(CY_DEVICE_CCG3)
        ADC_voltage = (int)(3.3 * ADC_val/255 * 1000);
            /* ADC code to voltage conversion formula for PMG1-S2; Vref = 3.3V */
#else
        if(ADC_VREF)
            /* ADC code to voltage conversion formula for PMG1-S0,S1,S3; Vref = 3.3V */
            ADC_voltage = (int)(3.3 * ADC_val/255 * 1000);
        else
            /* ADC code to voltage conversion formula for PMG1-S0,S1,S3; Vref = 2.0V */
            ADC_voltage = (int)(2.0 * ADC_val/255 * 1000);
#endif

        /* Conversion from int to char_t for UART transmit */
        sprintf(ADC_string, "ADC result = %d ; Voltage measured = %d mV\r\n\n", ADC_val, ADC_voltage);

        /* Send ADC data string over UART serial terminal */
        Cy_SCB_UART_PutString(CYBSP_UART_HW, ADC_string);

        /* Toggle the user LED state */
        Cy_GPIO_Inv(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);

        /* Delay for UART terminal display of ADC data */
        Cy_SysLib_Delay(DELAY_MS);
#if DEBUG_PRINT
        if (ENTER_LOOP)
        {
            Cy_SCB_UART_PutString(CYBSP_UART_HW, "Entered for loop\r\n");
            ENTER_LOOP = false;
        }
#endif
    }
} 
/* [] END OF FILE */

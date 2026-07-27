/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for MOTOR_PWM */
#define MOTOR_PWM_INST                                                     TIMG6
#define MOTOR_PWM_INST_IRQHandler                               TIMG6_IRQHandler
#define MOTOR_PWM_INST_INT_IRQN                                 (TIMG6_INT_IRQn)
#define MOTOR_PWM_INST_CLK_FREQ                                           500000
/* GPIO defines for channel 0 */
#define GPIO_MOTOR_PWM_C0_PORT                                             GPIOB
#define GPIO_MOTOR_PWM_C0_PIN                                      DL_GPIO_PIN_2
#define GPIO_MOTOR_PWM_C0_IOMUX                                  (IOMUX_PINCM15)
#define GPIO_MOTOR_PWM_C0_IOMUX_FUNC                 IOMUX_PINCM15_PF_TIMG6_CCP0
#define GPIO_MOTOR_PWM_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_MOTOR_PWM_C1_PORT                                             GPIOB
#define GPIO_MOTOR_PWM_C1_PIN                                      DL_GPIO_PIN_3
#define GPIO_MOTOR_PWM_C1_IOMUX                                  (IOMUX_PINCM16)
#define GPIO_MOTOR_PWM_C1_IOMUX_FUNC                 IOMUX_PINCM16_PF_TIMG6_CCP1
#define GPIO_MOTOR_PWM_C1_IDX                                DL_TIMER_CC_1_INDEX




/* Defines for I2C_0 */
#define I2C_0_INST                                                          I2C0
#define I2C_0_INST_IRQHandler                                    I2C0_IRQHandler
#define I2C_0_INST_INT_IRQN                                        I2C0_INT_IRQn
#define I2C_0_BUS_SPEED_HZ                                                400000
#define GPIO_I2C_0_SDA_PORT                                                GPIOA
#define GPIO_I2C_0_SDA_PIN                                         DL_GPIO_PIN_0
#define GPIO_I2C_0_IOMUX_SDA                                      (IOMUX_PINCM1)
#define GPIO_I2C_0_IOMUX_SDA_FUNC                       IOMUX_PINCM1_PF_I2C0_SDA
#define GPIO_I2C_0_SCL_PORT                                                GPIOA
#define GPIO_I2C_0_SCL_PIN                                         DL_GPIO_PIN_1
#define GPIO_I2C_0_IOMUX_SCL                                      (IOMUX_PINCM2)
#define GPIO_I2C_0_IOMUX_SCL_FUNC                       IOMUX_PINCM2_PF_I2C0_SCL


/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           32000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                  (9600)
#define UART_0_IBRD_32_MHZ_9600_BAUD                                       (208)
#define UART_0_FBRD_32_MHZ_9600_BAUD                                        (21)





/* Defines for ADC12_0 */
#define ADC12_0_INST                                                        ADC0
#define ADC12_0_INST_IRQHandler                                  ADC0_IRQHandler
#define ADC12_0_INST_INT_IRQN                                    (ADC0_INT_IRQn)
#define ADC12_0_ADCMEM_ADC_CH0                                DL_ADC12_MEM_IDX_0
#define ADC12_0_ADCMEM_ADC_CH0_REF               DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_0_ADCMEM_ADC_CH0_REF_VOLTAGE_V                                     3.3
#define GPIO_ADC12_0_C0_PORT                                               GPIOA
#define GPIO_ADC12_0_C0_PIN                                       DL_GPIO_PIN_27
#define GPIO_ADC12_0_IOMUX_C0                                    (IOMUX_PINCM60)
#define GPIO_ADC12_0_IOMUX_C0_FUNC                (IOMUX_PINCM60_PF_UNCONNECTED)



/* Port definition for Pin Group MOTOR_DIR */
#define MOTOR_DIR_PORT                                                   (GPIOA)

/* Defines for LEFT_IN1: GPIOA.13 with pinCMx 35 on package pin 28 */
#define MOTOR_DIR_LEFT_IN1_PIN                                  (DL_GPIO_PIN_13)
#define MOTOR_DIR_LEFT_IN1_IOMUX                                 (IOMUX_PINCM35)
/* Defines for LEFT_IN2: GPIOA.14 with pinCMx 36 on package pin 29 */
#define MOTOR_DIR_LEFT_IN2_PIN                                  (DL_GPIO_PIN_14)
#define MOTOR_DIR_LEFT_IN2_IOMUX                                 (IOMUX_PINCM36)
/* Defines for RIGHT_IN1: GPIOA.17 with pinCMx 39 on package pin 32 */
#define MOTOR_DIR_RIGHT_IN1_PIN                                 (DL_GPIO_PIN_17)
#define MOTOR_DIR_RIGHT_IN1_IOMUX                                (IOMUX_PINCM39)
/* Defines for RIGHT_IN2: GPIOA.16 with pinCMx 38 on package pin 31 */
#define MOTOR_DIR_RIGHT_IN2_PIN                                 (DL_GPIO_PIN_16)
#define MOTOR_DIR_RIGHT_IN2_IOMUX                                (IOMUX_PINCM38)
/* Defines for A0: GPIOA.12 with pinCMx 34 on package pin 27 */
#define GRAY_ADDR_A0_PORT                                                (GPIOA)
#define GRAY_ADDR_A0_PIN                                        (DL_GPIO_PIN_12)
#define GRAY_ADDR_A0_IOMUX                                       (IOMUX_PINCM34)
/* Defines for A1: GPIOB.16 with pinCMx 33 on package pin 26 */
#define GRAY_ADDR_A1_PORT                                                (GPIOB)
#define GRAY_ADDR_A1_PIN                                        (DL_GPIO_PIN_16)
#define GRAY_ADDR_A1_IOMUX                                       (IOMUX_PINCM33)
/* Defines for A2: GPIOB.17 with pinCMx 43 on package pin 36 */
#define GRAY_ADDR_A2_PORT                                                (GPIOB)
#define GRAY_ADDR_A2_PIN                                        (DL_GPIO_PIN_17)
#define GRAY_ADDR_A2_IOMUX                                       (IOMUX_PINCM43)
/* Defines for SCL: GPIOA.28 with pinCMx 3 on package pin 3 */
#define OLED_SCL_PORT                                                    (GPIOA)
#define OLED_SCL_PIN                                            (DL_GPIO_PIN_28)
#define OLED_SCL_IOMUX                                            (IOMUX_PINCM3)
/* Defines for SDA: GPIOA.31 with pinCMx 6 on package pin 5 */
#define OLED_SDA_PORT                                                    (GPIOA)
#define OLED_SDA_PIN                                            (DL_GPIO_PIN_31)
#define OLED_SDA_IOMUX                                            (IOMUX_PINCM6)
/* Defines for RES: GPIOB.14 with pinCMx 31 on package pin 24 */
#define OLED_RES_PORT                                                    (GPIOB)
#define OLED_RES_PIN                                            (DL_GPIO_PIN_14)
#define OLED_RES_IOMUX                                           (IOMUX_PINCM31)
/* Defines for DC: GPIOB.15 with pinCMx 32 on package pin 25 */
#define OLED_DC_PORT                                                     (GPIOB)
#define OLED_DC_PIN                                             (DL_GPIO_PIN_15)
#define OLED_DC_IOMUX                                            (IOMUX_PINCM32)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_MOTOR_PWM_init(void);
void SYSCFG_DL_I2C_0_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_ADC12_0_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */

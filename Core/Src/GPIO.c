/*
 * GPIO.c
 *
 *  Created on: Jul 19, 2025
 *      Author: HouseY2K
 */

#include "main.h"        // Inclui HAL + pinos
#include <stdbool.h>     // Pra usar bool, se for necessário
#include "GPIO.h"        // Prototipagem das funções (você pode criar esse .h)

extern UART_HandleTypeDef huart3;  // Declara que huart3 vem de outro arquivo

void resetPulse(void) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_Delay(25);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
}

void sendBypassCommand(void) {
    uint8_t data[3] = {0x80, 0xF0, 0x04};
    HAL_UART_Transmit(&huart3, data, 3, HAL_MAX_DELAY);
}

void dbgSetLow(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Configura o pino DBG como GPIO saída push-pull LOW
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);  // Ajuste porta e pino
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
}


void dbgSetUART(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();  // Liga clock do GPIOB, só pra garantir

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;     // Alternativa Push-Pull
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; // Velocidade alta pra UART
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // No pull-up/down (se quiser, pode colocar pull-up)

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    huart3.Instance = USART3;
    huart3.Init.BaudRate = 9600;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_HalfDuplex_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
}


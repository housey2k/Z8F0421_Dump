#include "firmware.h"
#include "usbd_cdc_if.h"
#include <string.h>

// By HouseY2K
// This code was mostly written by ChatGPT. I'm a terrible coder, but I understand what's going on and can answer most questions.


/* Definições rápidas para usar pinos do CubeMX */
#define DIO_PORT GPIOA
static const uint16_t DIO_PINS[8] = {
    A0_Pin, A1_Pin, A2_Pin, A3_Pin,
    A4_Pin, A5_Pin, A6_Pin, A7_Pin
};

#define SEL_PORT GPIOA
#define SEL_B0_PIN B0_Pin
#define SEL_B1_PIN B1_Pin
#define SEL_C0_PIN C0_Pin

#define CLK_PORT GPIOB
#define CLK_PIN TARGET_CLK_Pin

#define RESET_PORT GPIOB
#define RESET_PIN TARGET_RESET_Pin

extern UART_HandleTypeDef huart3;

static void sel_write(uint8_t b1, uint8_t b0, uint8_t c0) {
    HAL_GPIO_WritePin(SEL_PORT, SEL_B1_PIN, b1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEL_PORT, SEL_B0_PIN, b0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEL_PORT, SEL_C0_PIN, c0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void dio_set_output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = A0_Pin|A1_Pin|A2_Pin|A3_Pin|
                          A4_Pin|A5_Pin|A6_Pin|A7_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DIO_PORT, &GPIO_InitStruct);
}

static void dio_set_input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = A0_Pin|A1_Pin|A2_Pin|A3_Pin|
                          A4_Pin|A5_Pin|A6_Pin|A7_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DIO_PORT, &GPIO_InitStruct);
}


static uint8_t dio_read(void) {
    uint8_t val = 0;
    for (int i = 0; i < 8; i++) {
        if (HAL_GPIO_ReadPin(DIO_PORT, DIO_PINS[i]) == GPIO_PIN_SET)
            val |= (1 << i);
    }
    return val;
}

static void send_bypass_sequence(void) {
    uint8_t seq[] = {0x80, 0xF0, 0x04};
    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_SET);
    for (int i = 0; i < 3; i++) {
        HAL_UART_Transmit(&huart3, &seq[i], 1, HAL_MAX_DELAY);
        HAL_Delay(2);
    }
}

static void clk_pulse(void) {
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
    __NOP(); __NOP();
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
    __NOP(); __NOP();
}

static void dio_write(uint8_t val) {
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(DIO_PORT, DIO_PINS[i], (val & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

static void initialize_bypass(void)
{
    send_bypass_sequence();

    // set TMR & TEST1
    sel_write(1, 0, 0); // 0x04h

    dio_set_output();
    dio_write(0x80); // 0x80

    clk_pulse();
}

static void read(uint16_t begin, uint16_t end)
{
    uint8_t val;

    // === 1. enable read mode once ===
    sel_write(0, 1, 1);          // SEL=0x03 (CTRL)
    dio_set_output();
    dio_write(0xF0);             // XE + YE + SE + OE
    clk_pulse();

    // === 2. loop through address range ===
    for (uint16_t addr = begin; addr <= end; addr++) {

        // --- high byte ---
        sel_write(0, 0, 0);                  // SEL=0x00 (ADDRH)
        dio_set_output();
        dio_write((addr >> 8) & 0xFF);
        clk_pulse();

        // --- low byte ---
        sel_write(0, 0, 1);                  // SEL=0x01 (ADDRL)
        dio_set_output();
        dio_write(addr & 0xFF);
        clk_pulse();

        // --- switch to array readout ---
        sel_write(1, 0, 1);                  // SEL=0x05 (DATA OUT)
        dio_set_input();
        clk_pulse();
        __NOP(); __NOP(); __NOP();           // ~small wait (~50 ns)

        // --- sample ---
        val = dio_read();

        // send out over USB CDC
        CDC_Transmit_FS(&val, 1);

        // --- recovery ---
        sel_write(1, 1, 1);                  // SEL=0x07 (Hi-Z)
        dio_set_input();
        HAL_Delay(1);                        // 1 µs recovery
    }
}



void fwsetup(void) {
	initialize_bypass(); // reset, BYPASS sequence, TMT&TEST1

	read(0x0, 0x0FFF); // full flash dump
}

void fwloop(void) {

}

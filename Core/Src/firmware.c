#include "firmware.h"
#include "usbd_cdc_if.h"
#include <string.h>

// By HouseY2K
// This code was mostly written by ChatGPT. I'm a terrible coder, but I understand what's going on and can ask most questions.

// HouseY2K: you know i hate being formal. i hate order, i hate hierarchy. i'm freedom last boss, any formal shit was master gpt
// HouseY2K: i would never be able to write this shit myself
// HouseY2K: thank you, gpt 4.1-mini

// HouseY2K: long live all the hardware hackers
// HouseY2K: everyone against Landis+Gyr

// HouseY2K: fym unprofessional? I'm hacking this chip on femboy outfits, I'm not a pentester in a meeting room
// HouseY2K: Let's all take fursuit pics for #FursuitFriday after we dump this shit right here



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

/* ===================== Funções de controle ===================== */
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

static void dio_write(uint8_t val) {
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(DIO_PORT, DIO_PINS[i], (val & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

static uint8_t dio_read(void) {
    uint8_t val = 0;
    for (int i = 0; i < 8; i++) {
        if (HAL_GPIO_ReadPin(DIO_PORT, DIO_PINS[i]) == GPIO_PIN_SET)
            val |= (1 << i);
    }
    return val;
}

static void sel_write(uint8_t b1, uint8_t b0, uint8_t c0) {
    HAL_GPIO_WritePin(SEL_PORT, SEL_B1_PIN, b1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEL_PORT, SEL_B0_PIN, b0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEL_PORT, SEL_C0_PIN, c0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void clk_pulse(void) {
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
    __NOP(); __NOP();
    HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
    __NOP(); __NOP();
}

static void target_reset_assert(void) {
    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_RESET);
}

static void target_reset_release(void) {
    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_SET);
}

/* ===================== Sequência de bypass ===================== */
static void send_bypass_sequence(void) {
    uint8_t seq[] = {0x80, 0xF0, 0x04};
    target_reset_assert();
    HAL_Delay(5);
    target_reset_release();
    HAL_Delay(5);
    for (int i = 0; i < 3; i++) {
        HAL_UART_Transmit(&huart3, &seq[i], 1, HAL_MAX_DELAY);
        HAL_Delay(2);
    }
}

/* ===================== Seletores modo BYPASS ===================== */
#define SEL_ADDRL()  sel_write(0,0,0)
#define SEL_ADDRH()  sel_write(0,0,1)
#define SEL_DIN()    sel_write(0,1,0)
#define SEL_DOUT()   sel_write(0,1,1)
#define SEL_CTRL()   sel_write(1,0,0)
#define SEL_STATUS() sel_write(1,0,1)

/* ===================== Leitura de 1 byte ===================== */
static uint8_t read_byte(uint16_t addr) {
    uint8_t lo = addr & 0xFF;
    uint8_t hi = (addr >> 8) & 0xFF;
    uint8_t data;

    SEL_ADDRL();
    dio_set_output();
    dio_write(lo);
    for (int i = 0; i < 8; i++) clk_pulse();

    SEL_ADDRH();
    dio_write(hi);
    for (int i = 0; i < 8; i++) clk_pulse();

    SEL_DOUT();
    dio_set_input();
    for (int i = 0; i < 8; i++) clk_pulse();
    data = dio_read();

    return data;
}

/* ===================== Dump ===================== */
static void dump_flash(uint16_t start, uint16_t end) {
    uint8_t buf[64];
    uint16_t ptr = 0;

    for (uint16_t addr = start; addr <= end; addr++) {
        buf[ptr++] = read_byte(addr);
        if (ptr >= sizeof(buf)) {
            while (CDC_Transmit_FS(buf, ptr) == USBD_BUSY) {}
            ptr = 0;
        }
    }
    if (ptr > 0) {
        while (CDC_Transmit_FS(buf, ptr) == USBD_BUSY) {}
    }
}

/* ===================== Entradas públicas ===================== */
void fwsetup(void) {
    HAL_Delay(10);
    send_bypass_sequence();
    HAL_Delay(20);
}

void fwloop(void) {
    dump_flash(0x0000, 0x07FF); // exemplo: 2KB
    while (1) {
        HAL_Delay(1000);
    }
}

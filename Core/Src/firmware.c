// firmware.c - Z8F0421 core dump using Flash Bypass Mode
// HouseY2K: let's all unite and rape the Landis+Gyr engineer that thought it was a great idea to put read protection on the MCU

#include "main.h"
#include "stm32f1xx_hal.h"

// === ChatGPT: Pin definitions ===
#define OCD_TX_Pin     GPIO_PIN_10   // ChatGPT: PB10 (USART3 TX -> Z8 DBG)
#define OCD_TX_Port    GPIOB

#define PORTA_Pins     GPIOA         // ChatGPT: PA0–PA7: Data/Addr/Control bus
#define PORTA_MASK     0xFF

#define SEL_B0_Pin     GPIO_PIN_8    // ChatGPT: PA8
#define SEL_B1_Pin     GPIO_PIN_9    // ChatGPT: PA9
#define SEL_C0_Pin     GPIO_PIN_10   // ChatGPT: PA10
#define SEL_Port       GPIOA

#define XIN_Pin        GPIO_PIN_4    // ChatGPT: PB4 -> XIN pin on target
#define XIN_Port       GPIOB

extern UART_HandleTypeDef huart3;

// HouseY2K: you know i hate being formal. i hate order, i hate hierarchy. i'm freedom last boss, any formal shit was master gpt
// HouseY2K: i would never be able to write this shit myself
// HouseY2K: thank you, gpt 4.1-mini

// HouseY2K: long live all the hardware hackers
// HouseY2K: fuck Landis+Gyr

// HouseY2K: fym unprofessional? I'm hacking this chip on femboy outfits, I'm not a pentester in a meeting room
// HouseY2K: Let's all take fursuit pics for #FursuitFriday after we dump this shit right here

// === ChatGPT: DWT Microsecond Delay ===
void delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

// === ChatGPT: Toggle XIN clock ===
void toggleXIN() {
    HAL_GPIO_TogglePin(XIN_Port, XIN_Pin);
    delay_us(2);  // ChatGPT: short delay for edge detect
    HAL_GPIO_TogglePin(XIN_Port, XIN_Pin);
    delay_us(2);
}

// === ChatGPT: Selector set ===
void setSelector(uint8_t sel) {
    HAL_GPIO_WritePin(SEL_Port, SEL_B0_Pin, (sel & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEL_Port, SEL_B1_Pin, (sel & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEL_Port, SEL_C0_Pin, (sel & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// === ChatGPT: Port A I/O direction control ===
void setPortADirectionInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = 0xFF; // ChatGPT: PA0-PA7
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(PORTA_Pins, &GPIO_InitStruct);
}

void setPortADirectionOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = 0xFF; // ChatGPT: PA0-PA7
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PORTA_Pins, &GPIO_InitStruct);
}

// === ChatGPT: Write to bus ===
void writePortA(uint8_t data) {
    setPortADirectionOutput();
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(PORTA_Pins, (1 << i), (data & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    toggleXIN();
}

// === ChatGPT: Read from bus ===
uint8_t readPortA(void) {
    setPortADirectionInput();
    uint8_t val = 0;
    for (int i = 0; i < 8; i++) {
        val |= (HAL_GPIO_ReadPin(PORTA_Pins, (1 << i)) ? 1 : 0) << i;
    }
    return val;
}

// === ChatGPT: Send OCD command ===
void sendOCD(uint8_t b) {
    HAL_UART_Transmit(&huart3, &b, 1, HAL_MAX_DELAY);
    delay_us(100); // ChatGPT: Allow sync
}

// === ChatGPT: Enter Flash Bypass Mode ===
void enterBypassMode() {
    sendOCD(0x80); // ChatGPT: Autobaud
    sendOCD(0xF0); // ChatGPT: Write Testmode
    sendOCD(0x04); // ChatGPT: Enable bypass
}

// === ChatGPT: Read byte from Flash ===
uint8_t readByte(uint16_t addr) {
    setSelector(0x00);  // ChatGPT: XADDR (hi)
    writePortA(addr >> 8);

    setSelector(0x01);  // ChatGPT: YADDR (lo)
    writePortA(addr & 0xFF);

    setSelector(0x03);  // ChatGPT: Control signals
    writePortA(0xF0);   // ChatGPT: XE | YE | SE | OE | TEST1

    delay_us(1);        // ChatGPT: settle
    setSelector(0x05);  // ChatGPT: DOUT
    uint8_t val = readPortA();

    setSelector(0x03);
    writePortA(0x00);   // ChatGPT: clear control

    return val;
}

// === ChatGPT: Main firmware logic ===
void fwmain(void) {
    // ChatGPT: Enable DWT cycle counter for delay_us
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    enterBypassMode();

    for (uint16_t addr = 0x0000; addr < 0x1000; addr++) {
        uint8_t val = readByte(addr);
        // ChatGPT: Save to RAM, USB, or EEPROM instead of USART3
        // ChatGPT: Avoid writing to huart3 since it's shared with OCD line
    }
}

/*
(Throw your hands up)
(Throw your, your hands up)
Ladies and gentlemen! (Throw your, throw, throw your)
(Throw your, your, your hands, your, your hands up)
(Throw your hands up)
Chocolate Starfish!
(Your hands up)
Wanna keep on rollin', baby!
(Throw, your hands up)
(Throw your hands up)
(Throw your hands up)

I move in, now move out!
Hands up, now hands down!
Back up, back up!
Tell me what you're gonna do now!
Breathe in, now breathe out!
Hands up, now hands down!
Back up, back up!
Tell me what you're gonna do now!
Keep rollin', rollin', rollin', rollin' (what?)
Keep rollin', rollin', rollin', rollin' (come on!)
Keep rollin', rollin', rollin', rollin' (yeah!)
Keep rollin', rollin', rollin', rollin'

Now I know y'all be lovin' this shit right here
L.I.M.P. bizkit is right here
People in the house put them hands in the air
'Cause if you don't care, then we don't care (yeah!)
One, two, three times, two to the six
Jonesin' your fix of that Limp Bizkit mix
So where the fuck you at, punk? Shut the fuck up!
And back the fuck up while we fuck this track up

(Throw your hands up)
(Throw, your hands up)
(Throw, throw your hands up)
(Throw your hands up)
(Throw your hands up)

You wanna mess with Limp Bizkit? (Yeah)
You can't mess with Limp Bizkit (why?)
Because we get it on (when?)
Everyday and every night, oh
And this platinum thing right here (uh-huh?)
Yo, we're doin' it all the time (what?)
So you better get some better beats and, ah
Get some better rhymes (d'oh!)
We got the gang set, so don't complain yet
Twenty-four-seven, never beggin' for a rain check
Old school soldiers blastin' out the hot shit
That rock shit, puttin' bounce in the mosh pit

(Throw your hands up)
(Throw, your hands up)
(Throw, throw your hands up)
(Throw your hands up)
(Throw your hands up)

Hey, ladies! (Where you at?)
Hey, fellas! (Where you at?)
And the people that don't give a fuck! (Where you at?)
All the lovers! (Where you at?)
All the haters! (Where you at?)
And all the people that call themselves players (where you at?)
Hot mamas! (Where you at?)
Pimp daddies! (Where you at?)
And the people rollin' up in caddies! (Where you at?)
Hey, rockers! (Where you at?)
Hip-hoppers! (Where you at?)
And everybody all around the world!

Move in, now move out!
Hands up, now hands down!
Back up, back up!
Tell me what you're gonna do now!
Breathe in, now breathe out!
Hands up, now hands down!
Back up, back up!
Tell me what you're gonna do now!
Keep rollin', rollin', rollin', rollin' (yeah!)
Keep rollin', rollin', rollin', rollin' (what!)
Keep rollin', rollin', rollin', rollin' (come on!)
Keep rollin', rollin', rollin', rollin'

Move in, now move out!
Hands up, now hands down!
Back up, back up!
Tell me what you're gonna do now!
Breathe in, now breathe out!
Hands up, now hands down!
Back up, back up!
Tell me what you're gonna do now!
Keep rollin', rollin', rollin', rollin' (what?)
Keep rollin', rollin', rollin', rollin' (come on!)
Keep rollin', rollin', rollin', rollin' (yeah!)
Keep rollin', rollin', rollin', rollin'
 */

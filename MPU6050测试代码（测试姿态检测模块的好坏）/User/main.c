#include "stm32f10x.h"
#include "oled.h"           // 包含 oled.h 即可，无需 oledfont.h
#include "mpu6050.h"
#include <stdio.h>
#include <string.h>

// 简单的延时（不依赖 SysTick）
static void delay_us(uint32_t us)
{
    uint32_t i;
    for (; us > 0; us--)
    {
        i = 10;
        while (i--);
    }
}
static void delay_ms(uint32_t ms)
{
    while (ms--) delay_us(1000);
}

// LED 初始化（PC13 低电平点亮）
static void LED_Init(void)
{
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_13;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}
static void LED_On(void)  { GPIO_ResetBits(GPIOC, GPIO_Pin_13); }
static void LED_Off(void) { GPIO_SetBits(GPIOC, GPIO_Pin_13); }

int main(void)
{
    LED_Init();
    OLED_Init();
    delay_ms(100);

    // 针对 1.3 寸 OLED (SH1106) 的偏移修正（如果需要）
    // OLED_WR_Byte(0x02, OLED_CMD);  // 默认列起始地址，可尝试 0x00

    // 初始化 MPU6050 (PB8/PB9)
    SI2C_TypeDef mpu_i2c;
    mpu_i2c.SCL_GPIOx = GPIOB;
    mpu_i2c.SCL_GPIO_Pin = GPIO_Pin_8;
    mpu_i2c.SDA_GPIOx = GPIOB;
    mpu_i2c.SDA_GPIO_Pin = GPIO_Pin_9;
    MPU6050_Init(&mpu_i2c);
    delay_ms(100);

    // LED 闪烁 3 次表示启动成功
    for (int i = 0; i < 3; i++)
    {
        LED_On();
        delay_ms(200);
        LED_Off();
        delay_ms(200);
    }

    // 清屏并绘制固定标签（一次性）
    OLED_Clear();
    DrawStringToGRAM(0, 0, (u8*)"AccX:", 16, 1);
    DrawStringToGRAM(0, 16, (u8*)"AccY:", 16, 1);
    DrawStringToGRAM(0, 32, (u8*)"Pitch:", 16, 1);
    DrawStringToGRAM(0, 48, (u8*)"Roll:", 16, 1);
    OLED_Refresh();

    MPU6050_Data data;
    char line[20];

    while (1)
    {
        MPU6050_ReadData(&data);

        // 清除旧数值（12个空格足够）
        DrawStringToGRAM(50, 0, (u8*)"            ", 16, 1);
        DrawStringToGRAM(50, 16, (u8*)"            ", 16, 1);
        DrawStringToGRAM(50, 32, (u8*)"            ", 16, 1);
        DrawStringToGRAM(50, 48, (u8*)"            ", 16, 1);

        // 写入新数值
        snprintf(line, sizeof(line), "% 5.2f g", data.Accel_X);
        DrawStringToGRAM(50, 0, (u8*)line, 16, 1);
        snprintf(line, sizeof(line), "% 5.2f g", data.Accel_Y);
        DrawStringToGRAM(50, 16, (u8*)line, 16, 1);
        snprintf(line, sizeof(line), "% 6.1f deg", data.Angle_X);
        DrawStringToGRAM(50, 32, (u8*)line, 16, 1);
        snprintf(line, sizeof(line), "% 6.1f deg", data.Angle_Y);
        DrawStringToGRAM(50, 48, (u8*)line, 16, 1);

        // 一次性刷新整个屏幕
        OLED_Refresh();

        // LED 慢闪指示
        static uint32_t cnt = 0;
        if (++cnt >= 5)
        {
            cnt = 0;
            static uint8_t led_state = 0;
            if (led_state) LED_Off();
            else LED_On();
            led_state = !led_state;
        }

        delay_ms(200);
    }
}

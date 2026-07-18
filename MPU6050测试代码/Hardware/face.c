#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "face.h"
#include "W25Q64.h"
#include "random.h"

u8 readBuffer1[1024] __attribute__((section(".ccmram"))); // 使用CCM RAM


void face_blink(void);
void face_smile(void);
void face_cry(void);
void face_sad(void);
void face_struggle(void);
void face_angry(void);
void face_comfortable(void);
void face_eat(void);
void face_left(void);
void face_right(void);
void face_up(void);
void face_down(void);
void face_leftup(void);
void face_rightup(void);
void face_leftdown(void);
void face_rightdown(void);
void face_shy(void);

void face_blink(void)
{
    
    // 定义动画帧序列（地址 + 延迟时间）
    typedef struct {
        uint32_t addr;
        uint32_t delay_ms;
    } AnimationFrame;
    
    // 闭眼动画序列（固定部分）
    const AnimationFrame blink_sequence[] = {
        {0x001000,   300},   // 闭眼70% (快速过渡)
        {0x002000,   200},   // 闭眼30% (更快)
        {0x003000,   100},   // 闭眼10% (最快)
		{0x006000,   200},
        {0x003000,   100},   // 闭眼10% (快速返回)
        {0x002000,   200},   // 闭眼30% 
        {0x001000,   300}    // 闭眼70%
    };
    const int frame_count = sizeof(blink_sequence) / sizeof(blink_sequence[0]);
    
    while(1) {
        // 每次眨眼获取新的随机睁眼地址
        uint32_t random_address = Get_Random_Address();
        
        // 显示睁眼帧（使用新的随机地址）
        W25Q64_ReadData(random_address, readBuffer1, sizeof(readBuffer1));
        OLED_ShowPicture(0, 0, 128, 64, readBuffer1, 1);
        // 添加随机眨眼间隔（2-5秒）
        uint32_t random_delay = 2000 + (Get_Random_Address() % 3000);
        Delay_ms(random_delay);
        
        // 播放闭眼动画序列
        for(int i = 0; i < frame_count; i++) {
            W25Q64_ReadData(blink_sequence[i].addr, readBuffer1, sizeof(readBuffer1));
            OLED_ShowPicture(0, 0, 128, 64, readBuffer1, 1);
            Delay_us(blink_sequence[i].delay_ms);
        }

    }
}
//void face_blink(void)
//{
//	Random_Init();
//    // 获取随机地址（只获取一次）
//    uint32_t address = Get_Random_Address();
//    
//    // 定义动画帧序列（地址 + 延迟时间）
//    typedef struct {
//        uint32_t addr;
//        uint32_t delay_us;
//    } AnimationFrame;
//    
//    // 完整眨眼序列（包含正向闭眼和反向睁眼）
//    const AnimationFrame blink_sequence[] = {
//        {address,    1000},  // 睁眼 (长停留)
//        {0x001000,   300},   // 闭眼70% (快速过渡)
//        {0x002000,   200},   // 闭眼30% (更快)
//        {0x003000,   100},   // 闭眼10% (最快)
//        {0x006000,   500},   // 完全闭眼 (短暂保持)
//        {0x003000,   100},   // 闭眼10% (快速返回)
//        {0x002000,   200},   // 闭眼30% 
//        {0x001000,   300}    // 闭眼70%
//    };
//    const int frame_count = sizeof(blink_sequence) / sizeof(blink_sequence[0]);
//    
//    while(1) {
//        // 播放完整动画序列
//        for(int i = 0; i < frame_count; i++) {
//            W25Q64_ReadData(blink_sequence[i].addr, readBuffer1, sizeof(readBuffer1));
//            OLED_ShowPicture(0, 0, 128, 64, readBuffer1, 1);
//            Delay_us(blink_sequence[i].delay_us);
//        }
//        
//        // 可选：添加随机眨眼间隔
//        Delay_ms(2000 + (Get_Random_Address() % 3000)); // 2-5秒随机间隔
//    }
//}
//void face_blink(void)
//{
//	// 获取随机地址
//    uint32_t address = Get_Random_Address();
//	while(1){
//	W25Q64_ReadData(address, readBuffer1, sizeof(readBuffer1)); // 随机三个地址读取图片//睁眼
//	OLED_ShowPicture(0,0,128,64,readBuffer1,1);
//	Delay_us(1000); 
//	W25Q64_ReadData(0x001000, readBuffer1, sizeof(readBuffer1));//闭眼百分之七十
//	OLED_ShowPicture(0,0,128,64,readBuffer1,1);
//	Delay_us(1000);
//	W25Q64_ReadData(0x002000, readBuffer1, sizeof(readBuffer1));//闭眼百分之三十
//	OLED_ShowPicture(0,0,128,64,readBuffer1,1);
//	Delay_us(1000);
//	W25Q64_ReadData(0x003000, readBuffer1, sizeof(readBuffer1));//闭眼百分之十
//	OLED_ShowPicture(0,0,128,64,readBuffer1,1);
//	Delay_us(1000);
//	W25Q64_ReadData(0x006000, readBuffer1, sizeof(readBuffer1));//完全闭眼
//	OLED_ShowPicture(0,0,128,64,readBuffer1,1);
//	Delay_us(1000);
//	W25Q64_ReadData(0x003000, readBuffer1, sizeof(readBuffer1));//闭眼百分之十
//	OLED_ShowPicture(0,0,128,64,readBuffer1,1);
//	Delay_us(1000);
//	W25Q64_ReadData(0x002000, readBuffer1, sizeof(readBuffer1));//闭眼百分之三十
//	OLED_ShowPicture(0,0,128,64,readBuffer1,1);
//	Delay_us(1000);
//	W25Q64_ReadData(0x001000, readBuffer1, sizeof(readBuffer1));//闭眼百分之七十
//	OLED_ShowPicture(0,0,128,64,readBuffer1,1);
//	Delay_us(1000);

//	}
//}

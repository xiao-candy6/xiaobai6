#include "main.h"
#include "Delay.h"
#include "ov7670.h"
#include "DICM.h"
#include "ST7735.h"

// ⚠️ 核心修改：定义整帧缓冲区
// 160 * 128 * 2字节(RGB565) = 40960 字节 (约40KB)
uint8_t frame_buf[IMAGE_HEIGHT][LINE_SIZE]; 

// ⚠️ 只保留整帧完成标志，删除 line_ready 和 line_idx
extern volatile uint8_t frame_ready; 

// 帧同步GPIO初始化
static void FrameSync_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // PE0 和 PE5 配置为输出（帧同步脉冲用）
    GPIO_InitStruct.GPIO_Pin = FRAME_SYNC_PIN | GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(FRAME_SYNC_PORT, &GPIO_InitStruct);
    GPIO_SetBits(FRAME_SYNC_PORT, FRAME_SYNC_PIN);  // 初始高电平
    GPIO_SetBits(FRAME_SYNC_PORT, GPIO_Pin_5); 

    // 注意：PA4 和 PB7 绝不能配置为普通输出，它们是 DCMI 的 HSYNC 和 VSYNC
}

// 发送一帧完成后产生一个下降沿脉冲
static void Generate_FrameSync(void)
{
    GPIO_ResetBits(FRAME_SYNC_PORT, FRAME_SYNC_PIN);
    Delay_us(10);
    GPIO_SetBits(FRAME_SYNC_PORT, FRAME_SYNC_PIN);
}

/**
 * @brief 系统时钟配置：外部HSE 8MHz → 系统时钟168MHz
 */
void SystemClock_Config(void)
{
    ErrorStatus HSE_Status;

    RCC_HSEConfig(RCC_HSE_ON);
    HSE_Status = RCC_WaitForHSEStartUp();
    if(HSE_Status == ERROR)
    {
        while(1); 
    }

    RCC_PLLConfig(RCC_PLLSource_HSE, 8, 336, 2, 7);
    RCC_PLLCmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    RCC_HCLKConfig(RCC_CFGR_HPRE_DIV1);    // AHB = 168MHz
    RCC_PCLK1Config(RCC_CFGR_PPRE1_DIV4);   // APB1 = 42MHz
    RCC_PCLK2Config(RCC_CFGR_PPRE2_DIV2);   // APB2 = 84MHz
}

void GPIO_Config(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
                           RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
                           RCC_AHB1Periph_GPIOE, ENABLE);

    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_DCMI, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
}

void PA8_Output_8MHz(void)
{
    GPIO_InitTypeDef        GPIO_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_OCInitTypeDef       TIM_OCInitStruct;

    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_8;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);

    TIM_TimeBaseStruct.TIM_Prescaler         = 2;
    TIM_TimeBaseStruct.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_TimeBaseStruct.TIM_Period            = 6;
    TIM_TimeBaseStruct.TIM_ClockDivision     = TIM_CKD_DIV1;
    TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStruct);

    TIM_OCInitStruct.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStruct.TIM_Pulse       = 3;
    TIM_OCInitStruct.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &TIM_OCInitStruct);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

// ⚠️ 新增：整帧极速发送函数
//// 只设置一次窗口，连续发送 40KB 数据，比逐行发送快得多！
static void ST7735_DrawFullImage(const uint8_t *imgBuf) {
//    uint32_t i;
//    uint32_t total_size = IMAGE_WIDTH * IMAGE_HEIGHT * 2; // 40960 字节
//    
//    ST7735_SetWindow(0, 0, IMAGE_WIDTH - 1, IMAGE_HEIGHT - 1);
//    
//    TFT_CS(0);  // 整帧只拉低一次 CS
//    TFT_DC(1);  // 数据模式
//    
//    for (i = 0; i < total_size; i++) {
//        uint8_t data = imgBuf[i];
//        // 内联软件SPI，极速发送
//        for (uint8_t bit = 0; bit < 8; bit++) {
//            TFT_SCL(0);
//            if (data & 0x80) TFT_SDA(1);
//            else TFT_SDA(0);
//            data <<= 1;
//            TFT_SCL(1);
//        }
//    }
//    TFT_SCL(0);
//    TFT_CS(1);  // 整帧发完才拉高 CS

// 改后：逐行发送
for(uint16_t row = 0; row < IMAGE_HEIGHT; row++)
{
    ST7735_SetWindow(0, row, IMAGE_WIDTH - 1, row);
    TFT_CS(0);
    TFT_DC(1);
    for(uint16_t col = 0; col < LINE_SIZE; col++)
    {
        uint8_t data = frame_buf[row][col];
        for(uint8_t bit = 0; bit < 8; bit++)
        {
            TFT_SCL(0);
            if(data & 0x80) TFT_SDA(1); else TFT_SDA(0);
            data <<= 1;
            TFT_SCL(1);
        }
    }
    TFT_SCL(0);
    TFT_CS(1);
}
}
// 改前：



int main(void)
{
    SystemClock_Config();
    GPIO_Config();
    FrameSync_Init();
    
    PA8_Output_8MHz();
    ST7735_Init();
    
    if(OV7670_Init() == 0) {
        ST7735_FillRect(0, 0, 160, 128, 0xF800); // 初始化失败红屏提示
        while(1);
    }
    
    DCMI_Config();
    DCMI_DMA_Config();
    DCMI_Start();
      ST7735_FillRect(0, 0, 160, 128, 0xF800); 
//    while(1)
//    {
//        // 等待 DMA 整帧搬运完成中断 置位此标志
//        if(frame_ready)
//        {
//            frame_ready = 0; // 清除标志
//            
//            // 产生帧同步脉冲（PE0闪烁，证明代码跑到了这里）
//            Generate_FrameSync();
//            
//            // ⚠️ 核心逻辑：此时 DCMI 已经停止捕获，FIFO 不会溢出
//            // 一口气把整帧数据发给屏幕
//            ST7735_DrawFullImage((const uint8_t *)frame_buf);
//            
//            // 屏幕刷新完毕，重启 DCMI 捕获下一帧
//            DCMI_Start();
//        }
//    }

       while(1)
    {
        // 启动 DCMI 捕获
        DCMI_Start();
        
        // 等待 DMA 整帧搬运完成，加入超时检测
        uint32_t timeout = 0xFFFFFF; // 足够大的超时值
        while(frame_ready == 0 && timeout > 0)
        {
            timeout--;
        }
        
        if(timeout == 0) 
        {
            // ⚠️ 超时了！说明 DCMI 没收到 VSYNC，或者摄像头没出数据
            // 屏幕显示黄色警告
            ST7735_FillRect(0, 0, 160, 128, 0xFFE0);
            while(1); // 卡死在这里，方便你用万用表测量引脚
        }
        
        frame_ready = 0; // 清除标志
        Generate_FrameSync();
        
        // 检查数据是否全0
        if(frame_buf[0][0] == 0x00 && frame_buf[0][1] == 0x00 && frame_buf[0][2] == 0x00)
        {
            // 蓝屏：摄像头没出图
            ST7735_FillRect(0, 0, 160, 128, 0x001F); 
        }
        else
        {
            // 有数据，显示画面
            ST7735_DrawFullImage((const uint8_t *)frame_buf);
        }
    }

}

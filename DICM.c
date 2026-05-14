#include "main.h"
#include "Delay.h"

// ⚠️ 整帧缓冲区，在 main.c 中定义，这里声明外部引用
extern uint8_t frame_buf[IMAGE_HEIGHT][LINE_SIZE];

// 全局标志位，通知主循环一帧数据已经搬运完毕
volatile uint8_t frame_ready = 0;

// DCMI GPIO 初始化
static void DCMI_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // ================= 同步与时钟引脚 =================
    
    // --- VSYNC (PB7) --- 场同步必须接在PB7
    GPIO_InitStruct.GPIO_Pin = OV7670_VSYNC_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_DCMI);

    // --- HSYNC / HREF (PA4) --- 行同步必须接在PA4
    GPIO_InitStruct.GPIO_Pin = OV7670_HREF_PIN;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_DCMI);

    // --- PCLK (PA6) --- 像素时钟必须接在PA6
    GPIO_InitStruct.GPIO_Pin = OV7670_PCLK_PIN;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_DCMI);

    // ================= 数据引脚 =================

    // --- D0-D3 (PC6, PC7, PC8, PC9) ---
    GPIO_InitStruct.GPIO_Pin = OV7670_D0_PIN | OV7670_D1_PIN | OV7670_D2_PIN | OV7670_D3_PIN;
    GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_DCMI);

    // --- D4 (PC11) ---
    GPIO_InitStruct.GPIO_Pin = OV7670_D4_PIN;
    GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_DCMI);

    // --- D5-D7 (PB6, PB8, PB9) ---
    GPIO_InitStruct.GPIO_Pin = OV7670_D5_PIN | OV7670_D6_PIN | OV7670_D7_PIN;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_DCMI);
}

// DCMI 外设配置
void DCMI_Config(void)
{
    DCMI_InitTypeDef DCMI_InitStruct;

    DCMI_GPIO_Config();

    // ⚠️ 必须使用连续捕获模式，让 DMA 一口气搬完一整帧
    DCMI_InitStruct.DCMI_CaptureMode = DCMI_CaptureMode_Continuous; 
    DCMI_InitStruct.DCMI_SynchroMode = DCMI_SynchroMode_Hardware; 
	
    
    DCMI_InitStruct.DCMI_VSPolarity  = DCMI_VSPolarity_Low;      // OV7670标准：VSYNC低电平期间输出有效像素配ox15,0x00
    DCMI_InitStruct.DCMI_HSPolarity  = DCMI_VSPolarity_High;     // HREF高电平有效
    DCMI_InitStruct.DCMI_PCKPolarity = DCMI_PCKPolarity_Rising;  // 必须上升沿捕获！Rising

    DCMI_InitStruct.DCMI_CaptureRate = DCMI_CaptureRate_All_Frame; 
    DCMI_InitStruct.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b; 

    DCMI_Init(&DCMI_InitStruct);

    // 只开启 VSYNC 帧中断，用于捕获帧起始标志
    DCMI_ITConfig(DCMI_IT_VSYNC, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = DCMI_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

// DMA 配置 (整帧搬运模式)
void DCMI_DMA_Config(void)
{
    DMA_InitTypeDef DMA_InitStruct;

    DMA_DeInit(DMA2_Stream1);

    DMA_InitStruct.DMA_Channel = DMA_Channel_1;
    DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&DCMI->DR;
    
    // ⚠️ 核心修改：直接指向整帧数组的首地址
    DMA_InitStruct.DMA_Memory0BaseAddr = (uint32_t)frame_buf; 
    
    DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralToMemory;
    
    // ⚠️ 核心修改：一整帧的数据量 (160 * 128 * 2字节 / 4字节每字 = 10240 个字)
    DMA_InitStruct.DMA_BufferSize = (IMAGE_WIDTH * IMAGE_HEIGHT * 2) / 4; 
    
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    
    // ⚠️ 核心修改：正常模式，DMA搬完设定的 BufferSize (一整帧) 后自动停止，不再循环
    DMA_InitStruct.DMA_Mode = DMA_Mode_Normal; 
    DMA_InitStruct.DMA_Priority = DMA_Priority_High;
    DMA_InitStruct.DMA_FIFOMode = DMA_FIFOMode_Enable;
    DMA_InitStruct.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStruct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

    DMA_Init(DMA2_Stream1, &DMA_InitStruct);

    // 开启DMA传输完成中断，整帧搬完才触发
    DMA_ITConfig(DMA2_Stream1, DMA_IT_TC, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = DMA2_Stream1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0; 
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    DMA_Cmd(DMA2_Stream1, ENABLE);
}

// 启动 DCMI 采集
void DCMI_Start(void)
{
    frame_ready = 0; // 清除帧完成标志
    
    // 每次开启前，重新配置 DMA 的起始地址和数据长度
    DMA_Cmd(DMA2_Stream1, DISABLE);
    while(DMA_GetCmdStatus(DMA2_Stream1) != DISABLE); // 启动前可以等待
    
    DMA2_Stream1->M0AR = (uint32_t)frame_buf;
    DMA2_Stream1->NDTR = (IMAGE_WIDTH * IMAGE_HEIGHT * 2) / 4;
    DMA_Cmd(DMA2_Stream1, ENABLE);

    // 开启 DCMI
    DCMI_CaptureCmd(ENABLE);
    DCMI_Cmd(ENABLE);
}

// DCMI 中断服务函数
void DCMI_IRQHandler(void)
{
    if(DCMI_GetITStatus(DCMI_IT_VSYNC) != RESET)
    {
        // 捕获到帧起始信号，可以在这里做标记或清除标志
        DCMI_ClearITPendingBit(DCMI_IT_VSYNC);
    }
}

// DMA 中断服务函数
void DMA2_Stream1_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA2_Stream1, DMA_IT_TCIF1) != RESET)
    {
        DMA_ClearITPendingBit(DMA2_Stream1, DMA_IT_TCIF1);
        
        // ⚠️ 整帧数据已经全部从 DCMI 搬到了 frame_buf
        // 立刻停止 DCMI，防止摄像头持续输出导致 FIFO 溢出
        DCMI_CaptureCmd(DISABLE);
        DCMI_Cmd(DISABLE);
        
        // 通知主循环：一帧图像已经准备好，可以发给屏幕了
        frame_ready = 1; 
    }
}

#include "spi.h"

/**
 * @brief       �˿ڳ�ʼ��
 * @param       ��
 * @retval      ��
 */
void LCD_GPIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    LCD_SCK_CLK_ENABLE();
    LCD_MOSI_CLK_ENABLE();
    LCD_RES_CLK_ENABLE();
    LCD_DC_CLK_ENABLE();
    LCD_CS_CLK_ENABLE();
    
    GPIO_InitStructure.Pin=LCD_SCK_GPIO_PIN;
    GPIO_InitStructure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_SCK_GPIO_PORT,&GPIO_InitStructure);

    GPIO_InitStructure.Pin=LCD_MOSI_GPIO_PIN;
    GPIO_InitStructure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_MOSI_GPIO_PORT,&GPIO_InitStructure);

    GPIO_InitStructure.Pin=LCD_RES_GPIO_PIN;
    GPIO_InitStructure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_RES_GPIO_PORT,&GPIO_InitStructure);

    GPIO_InitStructure.Pin=LCD_DC_GPIO_PIN;
    GPIO_InitStructure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_DC_GPIO_PORT,&GPIO_InitStructure);
    
    GPIO_InitStructure.Pin=LCD_CS_GPIO_PIN;
    GPIO_InitStructure.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_CS_GPIO_PORT,&GPIO_InitStructure);
}

/**
 * @brief       IOģ��SPI����һ���ֽ�����
 * @param       dat: ��Ҫ���͵��ֽ�����
 * @retval      ��
 */
void LCD_WR_Bus(uint8_t dat)
{
    uint8_t i;
    LCD_CS_Clr();
    for (i = 0; i < 8; i++)
    {
        LCD_SCK_Clr();
        if (dat & 0x80)
        {
            LCD_MOSI_Set();
        }
        else
        {
            LCD_MOSI_Clr();
        }
        LCD_SCK_Set();
        dat <<= 1;
    }
    LCD_CS_Set();
}

/**
 * @brief       ��Һ��д�Ĵ�������
 * @param       reg: Ҫд������
 * @retval      ��
 */
void LCD_WR_REG(uint8_t reg)
{
    LCD_DC_Clr();
    LCD_WR_Bus(reg);
    LCD_DC_Set();
}

/**
 * @brief       ��Һ��дһ���ֽ�����
 * @param       dat: Ҫд������
 * @retval      ��
 */
void LCD_WR_DATA8(uint8_t dat)
{
    LCD_DC_Set();
    LCD_WR_Bus(dat);
    LCD_DC_Set();
}

/**
 * @brief       ��Һ��дһ����������
 * @param       dat: Ҫд������
 * @retval      ��
 */
void LCD_WR_DATA(uint16_t dat)
{
    LCD_DC_Set();
    LCD_WR_Bus(dat >> 8);
    LCD_WR_Bus(dat & 0xFF);
    LCD_DC_Set();
}

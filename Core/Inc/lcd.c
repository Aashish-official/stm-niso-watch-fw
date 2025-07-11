#include "lcd.h"
#include "lcdfont.h"
#include "string.h"

/**
 * @brief       ��ʾ�����ַ�
 * @param       x:�ַ���ʾλ������ʼ����
 * @param       y:�ַ���ʾλ������ʼ����
 * @param       num:��ʾ�ַ���ASCII��
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint8_t temp, sizex, t, m = 0;
    uint16_t i, TypefaceNum; // һ���ַ���ռ�ֽڴ�С
    sizex = sizey / 2;
    TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
    num = num - ' ';                                     // �õ�ƫ�ƺ��ֵ
    LCD_Address_Set(x, y, x + sizex - 1, y + sizey - 1); // ������ʾ����
    for (i = 0; i < TypefaceNum; i++)
    {
        if (sizey == 16)
            temp = ascii_1608[num][i]; // ����8x16����
        else if (sizey == 24)
            temp = ascii_2412[num][i]; // ����12x24����
        else if (sizey == 32)
            temp = ascii_3216[num][i]; // ����16x32����
        else
            return;
        for (t = 0; t < 8; t++)
        {
                if (temp & (0x01 << t))
                    LCD_WR_DATA(fc);
                else
                    LCD_WR_DATA(bc);
                m++;
                if (m % sizex == 0)
                {
                    m = 0;
                    break;
                }
        }
    }
}

/**
 * @brief       ��ʾ�ַ���
 * @param       x:�ַ�����ʾλ������ʼ����
 * @param       y:�ַ�����ʾλ������ʼ����
 * @param       *s:��ʾ���ַ�������
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_ShowString(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint16_t sizey)
{
    while ((*s <= '~') && (*s >= ' ')) // �ж��ǲ��ǷǷ��ַ�
    {
        if (x > (LCD_W - 1) || y > (LCD_H - 1))
            return;
        LCD_ShowChar(x, y, *s, fc, bc, sizey);
        x += sizey / 2;
        s++;
    }
}

/**
 * @brief       ������(�ڲ�����)
 * @param       m:����
 * @param       n:ָ��
 * @retval      result:m��n����
 */
uint32_t mypow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--)
    {
        result *= m;
    }
    return result;
}

/**
 * @brief       ��ʾ����
 * @param       x:������ʾλ������ʼ����
 * @param       y:������ʾλ������ʼ����
 * @param       num:��ʾ������(0~4294967295)
 * @param       len:��ʾ���ֵ�λ��
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint8_t t, temp, enshow = 0;
    uint8_t sizex = sizey / 2;
    for (t = 0; t < len; t++)
    {
        temp = (num / mypow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                LCD_ShowChar(x + t * sizex, y, ' ', fc, bc, sizey);
                continue;
            }
            else
            {
                enshow = 1;
            }
        }
        LCD_ShowChar(x + t * sizex, y, temp + '0', fc, bc, sizey);
    }
}

/**
 * @brief       ��ʾ������
 * @param       x:������ʾλ������ʼ����
 * @param       y:������ʾλ������ʼ����
 * @param       num:��ʾ�ĸ�����
 * @param       pre:��ʾ����������
 * @param       len:��ʾ��������λ��(������С����)
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_ShowFloatNum(uint16_t x, uint16_t y, float num, uint8_t pre, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint32_t i, temp, num1;
    uint8_t sizex = sizey / 2;
    num1 = num * mypow(10, pre);
    for (i = 0; i < len; i++)
    {
        temp = (num1 / mypow(10, len - i - 1)) % 10;
        if (i == (len - pre))
        {
            LCD_ShowChar(x + (len - pre) * sizex, y, '.', fc, bc, sizey);
            i++;
            len += 1;
        }
        LCD_ShowChar(x + i * sizex, y, temp + '0', fc, bc, sizey);
    }
}

/**
 * @brief       ��ʾ12x12����
 * @param       x:������ʾλ������ʼ����
 * @param       y:������ʾλ������ʼ����
 * @param       *s:��ʾ�����ַ���ʼ��ַ
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_ShowChinese12x12(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint8_t i, j, m = 0;
    uint16_t k, HZnum;    // ������Ŀ
    uint16_t TypefaceNum; // һ���ַ���ռ�ֽڴ�С

    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont12) / sizeof(typFONT_GB12); // ͳ�ƺ�����Ŀ
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont12[k].Index[0] == *(s)) && (tfont12[k].Index[1] == *(s + 1)))
        {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                        if (tfont12[k].Msk[i] & (0x01 << j))
                        {
                            LCD_WR_DATA(fc);
                        }
                        else
                        {
                            LCD_WR_DATA(bc);
                        }
                        m++;
                        if (m % sizey == 0)
                        {
                            m = 0;
                            break;
                        }
                }
            }
        }
        continue; // ���ҵ���Ӧ�����ֿ������˳�����ֹ��������ظ�ȡģ����Ӱ��
    }
}

/**
 * @brief       ��ʾ16x16����
 * @param       x:������ʾλ������ʼ����
 * @param       y:������ʾλ������ʼ����
 * @param       *s:��ʾ�����ַ���ʼ��ַ
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_ShowChinese16x16(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint8_t i, j, m = 0;
    uint16_t k, HZnum;    // ������Ŀ
    uint16_t TypefaceNum; // һ���ַ���ռ�ֽڴ�С

    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont16) / sizeof(typFONT_GB16); // ͳ�ƺ�����Ŀ
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont16[k].Index[0] == *(s)) && (tfont16[k].Index[1] == *(s + 1)))
        {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                        if (tfont16[k].Msk[i] & (0x01 << j))
                        {
                            LCD_WR_DATA(fc);
                        }
                        else
                        {
                            LCD_WR_DATA(bc);
                        }
                        m++;
                        if (m % sizey == 0)
                        {
                            m = 0;
                            break;
                        }
                    
                }
            }
        }
        continue; // ���ҵ���Ӧ�����ֿ������˳�����ֹ��������ظ�ȡģ����Ӱ��
    }
}

/**
 * @brief       ��ʾ24x24����
 * @param       x:������ʾλ������ʼ����
 * @param       y:������ʾλ������ʼ����
 * @param       *s:��ʾ�����ַ���ʼ��ַ
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_ShowChinese24x24(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint8_t i, j, m = 0;
    uint16_t k, HZnum;    // ������Ŀ
    uint16_t TypefaceNum; // һ���ַ���ռ�ֽڴ�С

    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont24) / sizeof(typFONT_GB24); // ͳ�ƺ�����Ŀ
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont24[k].Index[0] == *(s)) && (tfont24[k].Index[1] == *(s + 1)))
        {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                  
                        if (tfont24[k].Msk[i] & (0x01 << j))
                        {
                            LCD_WR_DATA(fc);
                        }
                        else
                        {
                            LCD_WR_DATA(bc);
                        }
                        m++;
                        if (m % sizey == 0)
                        {
                            m = 0;
                            break;
                        }
                }
            }
        }
        continue; // ���ҵ���Ӧ�����ֿ������˳�����ֹ��������ظ�ȡģ����Ӱ��
    }
}

/**
 * @brief       ��ʾ32x32����
 * @param       x:������ʾλ������ʼ����
 * @param       y:������ʾλ������ʼ����
 * @param       *s:��ʾ�����ַ���ʼ��ַ
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_ShowChinese32x32(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint8_t i, j, m = 0;
    uint16_t k, HZnum;    // ������Ŀ
    uint16_t TypefaceNum; // һ���ַ���ռ�ֽڴ�С

    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont32) / sizeof(typFONT_GB32); // ͳ�ƺ�����Ŀ
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont32[k].Index[0] == *(s)) && (tfont32[k].Index[1] == *(s + 1)))
        {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                        if (tfont32[k].Msk[i] & (0x01 << j))
                        {
                            LCD_WR_DATA(fc);
                        }
                        else
                        {
                            LCD_WR_DATA(bc);
                        }
                        m++;
                        if (m % sizey == 0)
                        {
                            m = 0;
                            break;
                        }

                }
            }
        }
        continue; // ���ҵ���Ӧ�����ֿ������˳�����ֹ��������ظ�ȡģ����Ӱ��
    }
}

/**
 * @brief       ��ʾ���ִ�
 * @param       x:������ʾλ������ʼ����
 * @param       y:������ʾλ������ʼ����
 * @param       *s:��ʾ�����ַ�
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_ShowChinese(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    while (*s != 0)
    {
        if (sizey == 12)
            LCD_ShowChinese12x12(x, y, s, fc, bc, sizey);
        else if (sizey == 16)
            LCD_ShowChinese16x16(x, y, s, fc, bc, sizey);
        else if (sizey == 24)
            LCD_ShowChinese24x24(x, y, s, fc, bc, sizey);
        else if (sizey == 32)
            LCD_ShowChinese32x32(x, y, s, fc, bc, sizey);
        else
            return;
        s += 2;
        x += sizey;
    }
}

/**
 * @brief       ��Ӣ�ַ�����
 * @param       x:��ʾλ������ʼ����
 * @param       y:��ʾλ������ʼ����
 * @param       *s:��ʾ�ַ���ʼ��ַ
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_ShowStr(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint16_t x0 = x;
    uint8_t bHz = 0; // �ַ���������
    while (*s != 0)  // ����δ����
    {
        if (!bHz) // Ӣ��
        {
            if (x > (LCD_W - sizey / 2) || y > (LCD_H - sizey))
            {
                return;
            }
            if (*s > 0x80)
            {
                bHz = 1; // ����
            }
            else // �ַ�
            {
                if (*s == 0x0D) // ���з���
                {
                    y += sizey;
                    x = x0;
                    s++;
                }
                else
                {
                    LCD_ShowChar(x, y, *s, fc, bc, sizey);
                    x += sizey / 2; // �ַ�,Ϊȫ�ֵ�һ��
                }
                s++;
            }
        }
        else // ����
        {
            if (x > (LCD_W - sizey) || y > (LCD_H - sizey))
            {
                return;
            }
            bHz = 0;
            if (sizey == 12)
                LCD_ShowChinese12x12(x, y, s, fc, bc, sizey);
            else if (sizey == 16)
                LCD_ShowChinese16x16(x, y, s, fc, bc, sizey);
            else if (sizey == 24)
                LCD_ShowChinese24x24(x, y, s, fc, bc, sizey);
            else
                LCD_ShowChinese32x32(x, y, s, fc, bc, sizey);
            s += 2;
            x += sizey;
        }
    }
}

/**
 * @brief       �ַ�������ʾ
 * @param       x:�����������Ч
 * @param       y:��ʾλ������ʼ����
 * @param       *s:��ʾ�ַ���ʼ��ַ
 * @param       fc:�ַ���ɫ
 * @param       bc:�ַ�������ɫ
 * @param       sizey:�ַ���С
 * @retval      ��
 */
void LCD_StrCenter(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint16_t len = strlen((const char *)s);
    uint16_t x1 = (LCD_W - len * 8) / 2;
    LCD_ShowStr(x1, y, s, fc, bc, sizey);
}

/**
 * @brief       ͼƬ��ʾ����
 * @param       x:ͼƬ��ʾλ������ʼ����
 * @param       y:ͼƬ��ʾλ������ʼ����
 * @param       width:ͼƬ���
 * @param       height:ͼƬ�߶�
 * @param       pic:ͼƬȡģ����
 * @retval      ��
 */
void LCD_ShowPicture(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t pic[])
{
    uint8_t picH, picL;
    uint16_t i, j;
    uint32_t k = 0;
    LCD_Address_Set(0, y, x + width - 1, y + height - 1);
    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width+x; j++)
        {
        	if(j<x)
        	{
        		picH = 0x00;
        		picL = 0x00;
        	}
        	else
        	{
        		picH = pic[k * 2];
        		picL = pic[k * 2 + 1];
        		k++;
        	}
        	LCD_WR_DATA(picH << 8 | picL);
        }
    }
}

#ifndef _LCD_INIT_H_
#define _LCD_INIT_H_

#include "spi.h"

/* ����Һ���ֱ��� */
#define USE_HORIZONTAL 0// ����������ʾ 0 ������ʾ  1��ת180����ʾ

#define LCD_W 120
#define LCD_H 240

/* ����˵�� */
void LCD_Address_Set(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);          // �������꺯��
void LCD_Fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color); // ��亯��
void LCD_Init(void);                                                               // ����LCD��ʼ��

/* ���廭����ɫ */
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define BRED 0XF81F
#define GRED 0XFFE0
#define GBLUE 0X07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define GREEN 0x07E0
#define CYAN 0x7FFF
#define YELLOW 0xFFE0
#define BROWN 0XBC40      // ��ɫ
#define BRRED 0XFC07      // �غ�ɫ
#define GRAY 0X8430       // ��ɫ
#define DARKBLUE 0X01CF   // ����ɫ
#define LIGHTBLUE 0X7D7C  // ǳ��ɫ
#define GRAYBLUE 0X5458   // ����ɫ
#define LIGHTGREEN 0X841F // ǳ��ɫ
#define LGRAY 0XC618      // ǳ��ɫ(PANNEL),���屳��ɫ
#define LGRAYBLUE 0XA651  // ǳ����ɫ(�м����ɫ)
#define LBBLUE 0X2B12     // ǳ����ɫ(ѡ����Ŀ�ķ�ɫ)

#endif

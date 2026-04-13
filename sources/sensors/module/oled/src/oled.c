/**
 * @file    oled.c
 * @brief   SSD1306 0.96-inch OLED driver (STM32F103, software I2C)
 * @pins    SCL -> PB1, SDA -> PB0
 */

#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"
#include "delay.h"

/* IIC Start */
void IIC_Start(void)
{
	OLED_SCLK_Set();
	OLED_SDIN_Set();
	OLED_SDIN_Clr();
	OLED_SCLK_Clr();
}

/* IIC Stop */
void IIC_Stop(void)
{
	OLED_SCLK_Set();
	OLED_SDIN_Clr();
	OLED_SDIN_Set();
}

/* Wait for IIC ACK (hardware ACK not checked on this bus) */
void IIC_Wait_Ack(void)
{
	OLED_SCLK_Set();
	OLED_SCLK_Clr();
}

/* Write one byte over IIC */
void Write_IIC_Byte(unsigned char IIC_Byte)
{
	unsigned char i;
	unsigned char m, da;
	da = IIC_Byte;
	OLED_SCLK_Clr();
	for (i = 0; i < 8; i++)
	{
		m = da & 0x80;
		if (m == 0x80)
			OLED_SDIN_Set();
		else
			OLED_SDIN_Clr();
		da = da << 1;
		OLED_SCLK_Set();
		OLED_SCLK_Clr();
	}
}

/* Write command byte over IIC */
void Write_IIC_Command(unsigned char IIC_Command)
{
	IIC_Start();
	Write_IIC_Byte(0x78);         /* Slave address, SA0=0 */
	IIC_Wait_Ack();
	Write_IIC_Byte(0x00);         /* Write command */
	IIC_Wait_Ack();
	Write_IIC_Byte(IIC_Command);
	IIC_Wait_Ack();
	IIC_Stop();
}

/* Write data byte over IIC */
void Write_IIC_Data(unsigned char IIC_Data)
{
	IIC_Start();
	Write_IIC_Byte(0x78);         /* D/C#=0; R/W#=0 */
	IIC_Wait_Ack();
	Write_IIC_Byte(0x40);         /* Write data */
	IIC_Wait_Ack();
	Write_IIC_Byte(IIC_Data);
	IIC_Wait_Ack();
	IIC_Stop();
}

/* Write one byte as command (cmd=0) or data (cmd=1) */
void OLED_WR_Byte(unsigned dat, unsigned cmd)
{
	if (cmd)
		Write_IIC_Data(dat);
	else
		Write_IIC_Command(dat);
}

/* Fill entire screen with fill_Data */
void fill_picture(unsigned char fill_Data)
{
	unsigned char m, n;
	for (m = 0; m < 8; m++)
	{
		OLED_WR_Byte(0xb0 + m, 0);  /* Page 0-7 */
		OLED_WR_Byte(0x00, 0);       /* Low column start */
		OLED_WR_Byte(0x10, 0);       /* High column start */
		for (n = 0; n < 128; n++)
			OLED_WR_Byte(fill_Data, 1);
	}
}

/* Busy-loop delay ~50 ms (approximate) */
void Delay_50ms(unsigned int Del_50ms)
{
	unsigned int m;
	for (; Del_50ms > 0; Del_50ms--)
		for (m = 6245; m > 0; m--);
}

/* Busy-loop delay ~1 ms (approximate) */
void Delay_1ms(unsigned int Del_1ms)
{
	unsigned char j;
	while (Del_1ms--)
		for (j = 0; j < 123; j++);
}

/* Set display cursor to column x, page y */
void OLED_Set_Pos(unsigned char x, unsigned char y)
{
	OLED_WR_Byte(0xb0 + y, OLED_CMD);
	OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
	OLED_WR_Byte((x & 0x0f), OLED_CMD);
}

/* Turn on OLED display */
void OLED_Display_On(void)
{
	OLED_WR_Byte(0x8D, OLED_CMD);  /* SET DCDC command */
	OLED_WR_Byte(0x14, OLED_CMD);  /* DCDC ON */
	OLED_WR_Byte(0xAF, OLED_CMD);  /* DISPLAY ON */
}

/* Turn off OLED display */
void OLED_Display_Off(void)
{
	OLED_WR_Byte(0x8D, OLED_CMD);  /* SET DCDC command */
	OLED_WR_Byte(0x10, OLED_CMD);  /* DCDC OFF */
	OLED_WR_Byte(0xAE, OLED_CMD);  /* DISPLAY OFF */
}

/* Clear screen (write all zeros) */
void OLED_Clear(void)
{
	u8 i, n;
	for (i = 0; i < 8; i++)
	{
		OLED_WR_Byte(0xb0 + i, OLED_CMD);  /* Page address 0-7 */
		OLED_WR_Byte(0x00, OLED_CMD);       /* Low column address */
		OLED_WR_Byte(0x10, OLED_CMD);       /* High column address */
		for (n = 0; n < 128; n++)
			OLED_WR_Byte(0, OLED_DATA);
	}
}

/* Light up all pixels */
void OLED_On(void)
{
	u8 i, n;
	for (i = 0; i < 8; i++)
	{
		OLED_WR_Byte(0xb0 + i, OLED_CMD);  /* Page address 0-7 */
		OLED_WR_Byte(0x00, OLED_CMD);       /* Low column address */
		OLED_WR_Byte(0x10, OLED_CMD);       /* High column address */
		for (n = 0; n < 128; n++)
			OLED_WR_Byte(1, OLED_DATA);
	}
}

/* Display one ASCII character at (x, y); Char_Size: 16 or 8 */
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 Char_Size)
{
	unsigned char c = 0, i = 0;
	c = chr - ' ';  /* Offset from space */
	if (x > Max_Column - 1) { x = 0; y = y + 2; }
	if (Char_Size == 16)
	{
		OLED_Set_Pos(x, y);
		for (i = 0; i < 8; i++)
			OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);
		OLED_Set_Pos(x, y + 1);
		for (i = 0; i < 8; i++)
			OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);
	}
	else
	{
		OLED_Set_Pos(x, y);
		for (i = 0; i < 6; i++)
			OLED_WR_Byte(F6x8[c][i], OLED_DATA);
	}
}

/* Compute m^n */
u32 oled_pow(u8 m, u8 n)
{
	u32 result = 1;
	while (n--) result *= m;
	return result;
}

/* Display a decimal number at (x, y); len digits, font size size2 */
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size2)
{
	u8 t, temp;
	u8 enshow = 0;
	for (t = 0; t < len; t++)
	{
		temp = (num / oled_pow(10, len - t - 1)) % 10;
		if (enshow == 0 && t < (len - 1))
		{
			if (temp == 0)
			{
				OLED_ShowChar(x + (size2 / 2) * t, y, ' ', size2);
				continue;
			}
			else
				enshow = 1;
		}
		OLED_ShowChar(x + (size2 / 2) * t, y, temp + '0', size2);
	}
}

/* Display a null-terminated string at (x, y) */
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 Char_Size)
{
	unsigned char j = 0;
	while (chr[j] != '\0')
	{
		OLED_ShowChar(x, y, chr[j], Char_Size);
		x += 8;
		if (x > 120) { x = 0; y += 2; }
		j++;
	}
}

/* Display 16x16 Chinese character at (x, y) from Hzk[no] */
void OLED_ShowCHinese(u8 x, u8 y, u8 no)
{
	u8 t, adder = 0;
	OLED_Set_Pos(x, y);
	for (t = 0; t < 16; t++)
	{
		OLED_WR_Byte(Hzk[2 * no][t], OLED_DATA);
		adder += 1;
	}
	OLED_Set_Pos(x, y + 1);
	for (t = 0; t < 16; t++)
	{
		OLED_WR_Byte(Hzk[2 * no + 1][t], OLED_DATA);
		adder += 1;
	}
}

/* Draw BMP image; x0/y0: top-left, x1/y1: bottom-right (y in pages) */
void OLED_DrawBMP(unsigned char x0, unsigned char y0,
                  unsigned char x1, unsigned char y1, unsigned char BMP[])
{
	unsigned int j = 0;
	unsigned char x, y;
	for (y = y0; y < y1; y++)
	{
		OLED_Set_Pos(x0, y);
		for (x = x0; x < x1; x++)
			OLED_WR_Byte(BMP[j++], OLED_DATA);
	}
}

/* Initialize SSD1306 */
void OLED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1);

	DelayMs(800);

	OLED_WR_Byte(0xAE, OLED_CMD);  /* Display off */
	OLED_WR_Byte(0x00, OLED_CMD);  /* Low column address */
	OLED_WR_Byte(0x10, OLED_CMD);  /* High column address */
	OLED_WR_Byte(0x40, OLED_CMD);  /* Start line address */
	OLED_WR_Byte(0xB0, OLED_CMD);  /* Page address */
	OLED_WR_Byte(0x81, OLED_CMD);  /* Contrast control */
	OLED_WR_Byte(0xFF, OLED_CMD);  /* Max contrast */
	OLED_WR_Byte(0xA1, OLED_CMD);  /* Segment remap */
	OLED_WR_Byte(0xA6, OLED_CMD);  /* Normal display */
	OLED_WR_Byte(0xA8, OLED_CMD);  /* Multiplex ratio */
	OLED_WR_Byte(0x3F, OLED_CMD);  /* 1/64 duty */
	OLED_WR_Byte(0xC8, OLED_CMD);  /* COM scan direction */
	OLED_WR_Byte(0xD3, OLED_CMD);  /* Display offset */
	OLED_WR_Byte(0x00, OLED_CMD);
	OLED_WR_Byte(0xD5, OLED_CMD);  /* OSC division */
	OLED_WR_Byte(0x80, OLED_CMD);
	OLED_WR_Byte(0xD8, OLED_CMD);  /* Area color mode off */
	OLED_WR_Byte(0x05, OLED_CMD);
	OLED_WR_Byte(0xD9, OLED_CMD);  /* Pre-charge period */
	OLED_WR_Byte(0xF1, OLED_CMD);
	OLED_WR_Byte(0xDA, OLED_CMD);  /* COM pin configuration */
	OLED_WR_Byte(0x12, OLED_CMD);
	OLED_WR_Byte(0xDB, OLED_CMD);  /* Vcomh */
	OLED_WR_Byte(0x30, OLED_CMD);
	OLED_WR_Byte(0x8D, OLED_CMD);  /* Charge pump enable */
	OLED_WR_Byte(0x14, OLED_CMD);
	OLED_WR_Byte(0xAF, OLED_CMD);  /* Turn on OLED panel */

	OLED_Clear();
}

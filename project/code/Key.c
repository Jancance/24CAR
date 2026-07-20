#include "zf_common_headfile.h"
#include "Key.h"
#include "board_pins.h"
// 由 1 ms 定时中断写入、主循环读取，必须声明为 volatile。
volatile uint8_t Num;

void Key_Init(void)
{

	  gpio_init(CAR_KEY_MODE_PIN , GPI, GPIO_HIGH, GPI_PULL_UP);
	  gpio_init(CAR_KEY_PLUS_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
	  gpio_init(CAR_KEY_MINUS_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
	  gpio_init(CAR_KEY_OK_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
	
	
}

uint8_t Key_GetNum(void)
{
	uint8_t Temp;
	if (Num)
	{
		Temp = Num;
		Num = 0;
		return Temp;
	}
	return 0;
}

uint8_t Key_GetState(void)
{
	if (gpio_get_level (CAR_KEY_MODE_PIN) == 0)
	{
		return 1;
	}
	if (gpio_get_level (CAR_KEY_PLUS_PIN) == 0)
	{
		return 2;
	}
	if (gpio_get_level (CAR_KEY_MINUS_PIN) == 0)
	{
		return 3;
	}
	if (gpio_get_level (CAR_KEY_OK_PIN) == 0)
	{
		return 4;
	}
	return 0;
}

void Key_Tick(void)
{
	static uint8_t Count;
	static uint8_t CurrState, PrevState;
	
	Count ++;
	if (Count >= 20)
	{
		Count = 0;
		
		PrevState = CurrState;
		CurrState = Key_GetState();
		
		if (CurrState == 0 && PrevState != 0)
		{
			Num = PrevState;
		}
	}
}

#include "intrins.h"

void Delay1ms(unsigned int t)    //824.000MHZ
{
	unsigned char i,j;
	for(t; t > 0;t--)
	{
		_nop_();
		i = 32;
		j = 40;
		do
		{
			while(--j);
		}	while(--i);
		
	}
}
	


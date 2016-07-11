#include "radio.h"
#include "nRF24L01.h"
#include "gpio.h"


extern struct spi_device *spi_device;
extern struct GpioRegisters *GpioRegisters_s;

/******************************
 Initializes radio
******************************/
void radio_init(void)
{
	uint8_t ret;

	GpioRegisters_s = (struct GpioRegisters *)__io_address(GPIO_BASE);
	
	//keep CE low
	SetGPIOFunction(NRF_CE,1);
	SetGPIOOutputValue(NRF_CE,0);

}


/******************************
 sets TX address (5 bytes)
******************************/
void nrf_set_tx_addr(uint8_t *val)
{
	nrf_xfer(TX_ADDR, 5, val, W);
}


/******************************
 sets address of different RX pipes
******************************/

int nrf_set_rx_addr(uint8_t *val, uint8_t pipe)
{
	uint8_t cmd;
	
	if(pipe > 5)
		return -1;

	cmd = RX_ADDR_P0 + pipe;

	switch(pipe){
		
		case 0:
			nrf_xfer(cmd, 5, val, W);
			break;
		case 1:
			nrf_xfer(cmd, 5, val, W);
			break;
		case 2:
			nrf_xfer(cmd, 1, val, W);
			break;
		case 3:
			nrf_xfer(cmd, 1, val, W);
			break;
		case 4:
			nrf_xfer(cmd, 1, val, W);
			break;
		case 5:
			nrf_xfer(cmd, 1, val, W);
			break;
	}
	return 0;
}


/******************************
 sets RF output power

 Note:- value range (0-3)
******************************/

int nrf_set_power(uint8_t power)
{
	
	uint8_t *val;

	if(power > 3)
		return -1;
	
	val = nrf_xfer(RF_SETUP, 1, NULL, R);
	
	power = power | val[0];

	nrf_xfer(RF_SETUP, 1, &power, W);

	return 0;
}


/******************************
 Sets communication speed

 Note:- 0----> 1Mbps
 	1----> 2Mbps
	2----> 250Kbps
******************************/

int nrf_set_speed(uint8_t speed)
{
	
	uint8_t *val;

	if(speed > 2)
		return -1;
	
	val = nrf_xfer(RF_SETUP, 1, NULL, R);
	
	speed = speed << 3;
	speed = speed | val[0];

	nrf_xfer(RF_SETUP, 1, &speed, W);

	return 0;
}


/******************************
 Sets nrf channel frequency

 note:- total channels 0 to 127
******************************/

int nrf_set_channel(uint8_t channel)
{
	if(channel > 127)
		return -1;

	nrf_xfer(RF_CH, 1, &channel, W);
	return 0;
}


/******************************
 Sets retransmission counts

 Note:- 0 = disabled, max value = 15
******************************/
int nrf_set_retrans_count(uint8_t count)
{
	uint8_t *val;

	if(count > 15)
		return -1;
	
	val = nrf_xfer(SETUP_RETR, 1, NULL, R);
	
	count = count | val[0];

	nrf_xfer(SETUP_RETR, 1, &count, W);
	return 0;
}


/******************************
 Sets retransmission delay

 Note:- value is between 0 to 15. each level adds 250us delay

 0 -----> 250us
 1 -----> 500us
 2 -----> 750us
 .         .
 .         .
******************************/

int nrf_set_retrans_delay(uint8_t delay)
{
	uint8_t *val;

	if(delay > 15)
		return -1;
	
	delay = delay << 4;
	val = nrf_xfer(SETUP_RETR,1,NULL,R);
	
	delay = delay | val[0];

	nrf_xfer(SETUP_RETR,1,&delay,W);
	return 0;
}


/******************************
 Sets RX/TX address width

 Note:- max value is 5 bytes and 0 bytes not allowed. If provided greater than that,
 function will return immediate and won't write to register
******************************/

int nrf_set_addr_width(uint8_t width)
{
	if(width == 0 && width > 5)
		return -1;
	
	nrf_xfer(SETUP_AW, 1, &width, W);
	return 0;
}


/******************************
 Enables RX data pipes 
 
 e.g :
 nrf_enable_pipes(DATA_PIPE_0 | DATA_PIPE_1);
 turns on data AA on pipe 0 & pipe 1
******************************/

void nrf_enable_pipes(uint8_t pipe)
{
	nrf_xfer(EN_RXADDR, 1, &pipe, W);
}


/******************************
 Sets auto acknowledgement to given pipe numbers

 e.g :
 nrf_set_AA(DATA_PIPE_0 | DATA_PIPE_1);
 turns on data AA on pipe 0 & pipe 1
******************************/

void nrf_set_AA(uint8_t pipe)
{
	nrf_xfer(EN_AA, 1, &pipe, W);
}


/******************************
 This function performs Read/Write to nrf chip.

 For write function:- 
 spi_write() is used and total number of bytes to be written 
 will be total number + 1.buffer will include command first and 
 data after that. command writing and data writing
 should be done in one function because of internal chip
 select handling in Linux.

 For read function:-
 For read, it is obvious to use spi_write_then_read() because
 we want to write command first and then read data. Due to internal
 chip select handling of Linux this function is suitable for nrf chip.
******************************/

uint8_t *nrf_xfer(uint8_t reg, size_t count, uint8_t *val, bool op)
{
	int retval;
	int loop;
	static uint8_t retarr[32];
	uint8_t len;
	
	len = count + 1;

	if(op == W){
		if(reg != W_TX_PAYLOAD)
			reg = reg + W_REGISTER;
	
		retarr[0] = reg;
		for(loop=1;loop < len;loop++){
			retarr[loop] = val[loop-1];
		}
		retval = spi_write(spi_device,retarr,len);
		if(retval != 0)
			printk(KERN_INFO "failed to write register to device\n");
		
	}

	if(op == R){

		retval = spi_write_then_read(spi_device,&reg,1,retarr,count);
		if(retval != 0)
			printk(KERN_INFO "failed to read from device\n");
	}

	return retarr;
}
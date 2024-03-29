#include "I2C.h"
#include "GPIO.h"
#include "USART.h"
#include "SYS_INIT.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t timeout = 200;

void I2C1_Config(uint8_t mode){
    //Enable I2C and GPIO
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; /*enables i2c1 peripheral*/
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    
    //Configure Pin 8,9 (GPIOB)
    GPIOB->MODER |= (GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1);
    GPIOB->OTYPER |= (GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9); /*open drain output*/
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED8 | GPIO_OSPEEDR_OSPEED9);
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPD8_0 | GPIO_PUPDR_PUPD9_0);
    GPIOB->AFR[1] |= (GPIO_AFRH_AFSEL8_2 | GPIO_AFRH_AFSEL9_2);
    
    //Configure I2C1
    
    I2C1->CR1 |= I2C_CR1_SWRST; /*software reset state*/
    I2C1->CR1 &= ~I2C_CR1_SWRST;
    
    //master mode config
    
    I2C1->CR2 |= (45 << I2C_CR2_FREQ_Pos);
    I2C1->CCR = 225 << I2C_CCR_CCR_Pos;
    I2C1->TRISE = 46 << I2C_TRISE_TRISE_Pos;

    I2C1->CR1 |= I2C_CR1_PE;/* enable i2c peripheral */
	
    I2C1->CR1 &= ~I2C_CR1_POS; /*ACK sent after each received byte*/
    
    if(mode == 0){
        /*Set Address and Enable Interrupts for Slave Receive*/
        I2C1_SetAddress(0);
        
        I2C1->CR2 |= I2C_CR2_ITEVTEN; /*Enable Event Interrupt*/
        I2C1->CR2 |= I2C_CR2_ITERREN; /*Enable Error Interrupt*/
        I2C1->CR2 |= I2C_CR2_ITBUFEN; /*Enable Buffer Interrupt*/
       
        NVIC_EnableIRQ(I2C1_EV_IRQn);  
    }
}


bool I2C1_Start(void){
    I2C1->CR1 |= I2C_CR1_START;
    TIM3->CNT = 0;
    while(!(I2C1->SR1 & I2C_SR1_SB)){
        if(TIM3->CNT > timeout)
            return false;  
    }
	return true;
}

void I2C1_Stop(void){
    I2C1->CR1 |= I2C_CR1_STOP;
}

/* for sending address of i2c slave */
bool I2C1_Address(uint8_t address){
    I2C1->DR = (uint8_t)(address << 1);
    TIM3->CNT = 0;
    
		/*check if the address matches*/
    while(!(I2C1->SR1 & I2C_SR1_ADDR)){
        if(TIM3->CNT > timeout)
            return false;
    }
    //Clear ADDR (read SR1 and SR2) 
    address = (uint8_t)(I2C1->SR1 | I2C1->SR2);
	
	return true;
}

void I2C1_SetAddress(uint8_t address){
    I2C1->OAR1 |= (uint32_t)(address << 1U);
    I2C1->OAR1 &= ~I2C_OAR1_ADDMODE; /*7-bit addressing mode*/
}

bool I2C1_Write(uint8_t data){
    TIM3->CNT = 0;
	
		/*ready to transmit data*/
    while(!(I2C1->SR1 & I2C_SR1_TXE)){
        if(TIM3->CNT > timeout){
            return false;
        }
    }
	
    I2C1->DR = data;
    TIM3->CNT = 0;
		/*byte transfer finished*/
    while(!(I2C1->SR1 & I2C_SR1_BTF)){
        if(TIM3->CNT > timeout)	return false;
    }
	  return true;
}


/*read and write functions*/
bool I2C1_TransmitMaster(char *buffer, uint32_t size){
    if(!I2C1_Start())	return false;    /* generate start */
    if(!I2C1_Address(0))	return false; /* send slave address */
    for(int idx = 0;idx < (int)size;idx++){
			if(!I2C1_Write(buffer[idx]))	return false;
		}/* send data */
		
    I2C1_Stop();  /* generate stop */
		return true;
}

char* I2C1_ReceiveSlave(uint8_t *buffer){
    uint32_t i = 0, len  = 0;
    uint8_t ch = 0;
    char res[100];

    while(!(I2C1->SR1 & I2C_SR1_ADDR)); /*wait for address to set*/
    i = I2C1->SR1 | I2C1->SR2; /*clear address flag*/
    i = 0;
	
    while(ch != '!'){
        while(!(I2C1->SR1 & I2C_SR1_RXNE)); /*wait for rxne to set*/
        ch = (uint8_t)I2C1->DR;/*read data from DR register*/
        if(ch == '!')	break;
        buffer[i++] = ch;
        len++;
    }
    buffer[i] = '\0';
    
    while(!(I2C1->SR1 & I2C_SR1_STOPF)); /*wait for stopf to set*/
    /*clear stop flag*/
    i = I2C1->SR1; 
    I2C1->CR1 |= I2C_CR1_PE;
    
    I2C1->CR1 &= ~I2C_CR1_ACK; /*disable ack*/
    
    
    for(i = 0; i < len; i++){
        res[i] = buffer[i];
    }
		
		res[i] = '\0';
    
    return res;
}











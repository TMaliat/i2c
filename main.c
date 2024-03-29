#include "CLOCK.h"
#include "GPIO.h"
#include "SYS_INIT.h"
#include "USART.h"
#include "I2C.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f4xx.h"

// Function declarations
void TIM5Config(void);
void TIM2Config(void);
void TIM3Config(void);
void tim5_delay(uint32_t ms);
void USART2_IRQHandler(void);
void I2C1_EV_IRQHandler(void);
void getString(void);
void init(void);
void mainloop(void);
void parse(char string[]);
void traffic_info(void);
void showTraffic(uint32_t light);
void showInterval(void);
void trafficDelay(char ch, uint32_t del, uint32_t light);
void resetLEDs(void);

static char in_buff[100], *rcv_str, stat[3][200], s1[20], s2[20], s3[20], ch1, ch2, ch3;
static uint32_t G_NS = 6, G_EW = 5, R_EW = 4, R_NS = 7, _interval = 5000, interval;
static uint32_t extendT = 0, g_delayNS = 5, r_delayNS = 2, y_delayNS = 1, trf;
static uint32_t g_delayEW = 5, r_delayEW = 2, y_delayEW = 1, tottime = 0;
static uint8_t rcv_ptr;

void traffic_info(void){
  char temp[600];
	char ss0[50],ss1[50],ss2[50],ss3[50];
	
	char *G_NS_status = (GPIO_PIN_6 & GPIOB->ODR)?"ON":"OFF";
	char *R_NS_status = (GPIO_PIN_7 & GPIOB->ODR)?"ON":"OFF";
	char *G_EW_status = (GPIO_PIN_5 & GPIOB->ODR)?"ON":"OFF";
	char *R_EW_status = (GPIO_PIN_4 & GPIOB->ODR)?"ON":"OFF";
	char *Y_NS_status = (R_NS == 0 && G_NS == 0)? "ON" : "OFF";
	char *Y_EW_status = (R_EW == 0 && G_EW == 0)? "ON" : "OFF";
	
	char *NS_traffic = (GPIO_PIN_1 & GPIOB->ODR)?"heavy traffic":"light traffic";
	char *EW_traffic = (GPIO_PIN_2 & GPIOB->ODR)?"heavy traffic":"light traffic";
	
	sprintf(ss0, "\n%d traffic light 1 %s %s %s\n", tottime, G_NS_status, Y_NS_status, R_NS_status);
	sprintf(ss1, "%d traffic light 2 %s %s %s\n", tottime, G_EW_status, Y_EW_status, R_EW_status);
	sprintf(ss2, "%d road north south %s \n", tottime, NS_traffic);
	sprintf(ss3, "%d road east west %s \n", tottime, EW_traffic); 

	strcpy(temp, stat[1]);
	strcpy(stat[1],stat[2]);
	strcpy(stat[0], temp);
	sprintf(temp,"%s%s%s%s",ss0,ss1,ss2,ss3);
	strcpy(stat[2],temp);

	UART_SendString(USART2, "\n-------------------------------");
	UART_SendString(USART2,stat[2]);
	UART_SendString(USART2, "-------------------------------\n");
}

void parse(char string[]){
	  sscanf(string,"%s %s %s %d",s1,s2,s3,&trf);
    if(strcmp(s1,"config")==0)
		{
			if(strcmp(s3,"light")==0) /*config traffic light*/
			{
					if(trf==1)	sscanf(string,"%s %s %s %d %c %c %c %d %d %d %d",
						s1,s2,s3,&trf,&ch1,&ch2,&ch3,&g_delayNS,&y_delayNS,&r_delayNS,&extendT);		
					else	sscanf(string,"%s %s %s %d %c %c %c %d %d %d %d",
						s1,s2,s3,&trf,&ch1,&ch2,&ch3,&g_delayEW,&y_delayEW,&r_delayEW,&extendT);  						
			}
			/*config traffic monitor*/
			if(strcmp(s3, "monitor")==0)	sscanf(string, "%s %s %s %d",s1,s2,s3,&interval);	
		}
		
		if(strcmp(s1,"read")==0)
		{
			if(strcmp(s3, "stat") ==0) /*read (traffic status)*/
			{
				sprintf(string,"traffic light 1 G Y R %d %d %d %d\n", g_delayNS, y_delayNS, r_delayNS, extendT);
				UART_SendString(USART2, string);
				sprintf(string,"traffic light 2 G Y R %d %d %d %d\n", g_delayEW, y_delayEW, r_delayEW, extendT);
				UART_SendString(USART2, string);
				sprintf(string,"traffic monitor %d\n", interval);
				UART_SendString(USART2, string);	
			}
			if(strcmp(s3,"light")==0) /*read traffic light*/
			{
					if(trf==1)
					{
						sprintf(string,"traffic light 1 G Y R %d %d %d %d\n", g_delayNS, y_delayNS, r_delayNS, extendT);
					  UART_SendString(USART2, string);
					}		
					else
					{
						sprintf(string,"traffic light 2 G Y R %d %d %d %d\n", g_delayEW, y_delayEW, r_delayEW, extendT);
					  UART_SendString(USART2, string);
					}						
			}
			if(strcmp(s3, "monitor")==0) /*read traffic monitor*/
			{
				sprintf(string,"traffic monitor %d\n", interval);
				UART_SendString(USART2, string);
			}
		}
}


void TIM5Config(void){
	RCC->APB1ENR |= (1<<3);
	
	TIM5->PSC = 45000 - 1; 
	TIM5->ARR = 0xFFFF; 
	
	TIM5->CR1 |= (1<<0);
	
	while(!(TIM5->SR & (1<<0)));
	
}
void TIM2Config(void){
	RCC->APB1ENR |= (1<<0);
	
	TIM2->PSC = 45000 - 1; 
	TIM2->ARR = 0xFFFF; 
	
	TIM2->CR1 |= (1<<0);
	
	while(!(TIM2->SR & (1<<0)));
	
}


void TIM3Config(void){
	RCC->APB1ENR |= (1<<1);
	
	TIM3->PSC = 45000 - 1; 
	TIM3->ARR = 0xFFFF;
	
	TIM3->CR1 |= (1<<0);
	
	while(!(TIM3->SR & (1<<0)));
}

void tim5_delay(uint32_t ms){
	ms = 2 * ms; /*tim clock freq half of tim2, tim3 freq*/
	TIM5->CNT = 0;
	while(TIM5->CNT < ms){
		if(TIM2->CNT > _interval*2){
			tottime += _interval/1000;
			traffic_info();
			TIM2->CNT = 0;
		}
	}
}

void getString(void){
    uint8_t ch,idx = 0;
    ch = UART_GetChar(USART2);
    while(ch != '.'){
        in_buff[idx++] = ch;
        ch = UART_GetChar(USART2);
        if(ch == '.')break;
    }      
    in_buff[idx] = '\0';
    
}
void USART2_IRQHandler(void){
    USART2->CR1 &= ~(USART_CR1_RXNEIE);
    getString();
    USART2->CR1 |= (USART_CR1_RXNEIE);
}

void I2C1_EV_IRQHandler(void){ 
    
    I2C1->CR2 &= ~I2C_CR2_ITEVTEN;/*disables event interrupt*/

    if(I2C1->SR1 & I2C_SR1_ADDR)
        rcv_str = I2C1_ReceiveSlave(&rcv_ptr);     

		if(strlen(rcv_str) != 0){
				sendString("Received : ");
				sendString(rcv_str);
				sendString(" \n");
				strcpy(in_buff,rcv_str);
				parse(in_buff);
		}

    I2C1->CR2 |= I2C_CR2_ITEVTEN;
}

void init(void){
	initClock();
	sysInit();
	TIM5Config();
	TIM2Config();
	TIM3Config();
	UART2_Config();
	GPIO_InitTypeDef gp;
	 
	gp.Mode = 01;
	gp.Speed = 11;
	gp.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_Init(GPIOB, &gp);
	
	TIM2->CNT = 0;
}

void mainloop(void){
    while(true){
		int v1, v2; 
		v1 = (rand() % 2);
		v2 = (rand() % 2);
		

		if(v1>0) GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
		if(v2>0) GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
		
		GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); /*red of road NS*/
		GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET); /*green of EW*/
		
		ms_delay(r_delayNS*1000);
		
		if(v1 == 1 && v2==0) /*road1 heavy*/
		{
			GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
			GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); /*green for NS*/
			GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET); /*red for EW*/
			ms_delay((r_delayNS+extendT)*1000);
		}
		
		if(v1 == 0 && v2==1) /*road2 heavy*/
		{
			GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
			GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
			GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
			ms_delay((r_delayEW+extendT)*1000);
			
		}
		if((v1 ==0 && v2==0) || (v1==1 && v2==1))
		{
			GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
			GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
			GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
			GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
			ms_delay(g_delayEW*1000);
		}
		
		GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
		GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
		GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
		GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
		GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
		GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
		
	}
     
}

int main(void){   
		uint8_t Mode = 1;
    I2C1_Config(Mode);
    init();
	
    strcpy(in_buff,"");
    sendString("HELLO\n");
    
    if(!Mode){
        sendString("Inside Slave Loop\n");
        strcpy(in_buff,"");
        mainloop();
    }

    else{
        sendString("Inside Master Loop\n");
        while(Mode){
            if (strlen(in_buff) != 0){          
                sendString(in_buff);
                sendString(" << Data to send\n");
                
                while(!I2C1_TransmitMaster(in_buff,strlen(in_buff)));
                strcpy(in_buff, "");
            }
        }
    } 
}



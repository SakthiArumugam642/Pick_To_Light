/*
 * File:   main.c
 * Author: sakthi
 * Pick To Light Client
 * Created on January 1, 2026, 7:39 PM
 */

#include <xca.h>
#include <stdio.h>

#define TRUE  1
#define FALSE 0

typedef enum {
    ST_BLANK,
    ST_NID,
    ST_PC,
    ST_SEND_ACK
} state_t;

state_t state = ST_BLANK;

#define INC   0x01
#define DEC   0x02
#define ACK   0x04
#define NID   0x08
#define NO_KEY 0x00

unsigned int my_node_id;
unsigned int pick_cnt;
unsigned char can_payload[13];
unsigned char blink = 0;
unsigned char sidl;
unsigned char sidh;

#define SIDL 3
#define D0   5
#define D1   6
#define D2   7
#define D3   8
#define D4   9
#define D5   10
#define D6   11
#define D7   12

void delay_ms(unsigned int ms)
{
    unsigned int i,j;
    for(i=0;i<ms;i++)
        for(j=120;j--;);
}

void write_eeprom(unsigned char address, unsigned char data)
{
//Write the address
	EEADR = address;

	//Write the data
	EEDATA = data;

	//Point to data memory
	EECON1bits.EEPGD = 0;

	//Access data EEPROM memory
	EECON1bits.CFGS = 0;

	//Data write enable bit
	EECON1bits.WREN = 1;

	//Global interrupt disable 
	GIE = 0;

	//Write protection sequence
	EECON2 = 0x55;
	EECON2 = 0xAA;

	//Initiate write
	EECON1bits.WR = 1;

	//Global interrupt enable
	GIE = 1;

	//Wait till write is complete
	while (!PIR2bits.EEIF);

	//Disable the flag
	PIR2bits.EEIF = 0;
}

unsigned char read_eeprom(unsigned char address)
{
    EEADR = address;

	//Inhibits write cycles to Flash program/data EEPROM
	EECON1bits.WREN = 0;

	//Point to data memory
	EECON1bits.EEPGD = 0;

	//Access data EEPROM memory
	EECON1bits.CFGS = 0;
	
	//Initiate read
	EECON1bits.RD = 1;

	//Data available in EEDATA register
	return EEDATA;
}

void init_can(void)
{
    TRISB2 = 0;
    TRISB3 = 1;

    CANCON = 0x80;
    while((CANSTAT & 0xE0)!=0x80);

    ECANCON = 0x00;
    BRGCON1 = 0xE1;
    BRGCON2 = 0x1B;
    BRGCON3 = 0x03;

    RXB0CON = 0x20;

    CANCON = 0x00;
    while((CANSTAT & 0xE0)!=0x00);
}

unsigned char can_receive(void)
{
    if(RXB0FUL)
    {
        sidl = RXB0SIDL;
        sidh = RXB0SIDH;
        can_payload[D0]=RXB0D0;
        can_payload[D1]=RXB0D1;
        can_payload[D2]=RXB0D2;
        can_payload[D3]=RXB0D3;
        can_payload[D4]=RXB0D4;
        can_payload[D5]=RXB0D5;
        can_payload[D6]=RXB0D6;
        can_payload[D7]=RXB0D7;
        RXB0FUL=0;
        return TRUE;
    }
    return FALSE;
}

void send_ack(void)
{
    TXB0SIDH = 0x6B;
    TXB0SIDL = 0xC0;
    TXB0DLC  = 1;
    TXB0D0   = 0xAA;
    TXB0REQ  = 1;
    while(TXB0REQ);
}

unsigned char digit[]={
0x3F,0x06,0x5B,0x4F,0x66,
0x6D,0x7D,0x07,0x7F,0x6F
};

void display(unsigned char d3, unsigned char d2, unsigned char d1, unsigned char d0)
{
    unsigned char i;

    for(i = 0; i < 5; i++)
    {

        PORTA = 0x07;
        PORTD = d3;
        delay_ms(1);

        PORTA = 0x0B; 
        PORTD = d2;
        delay_ms(1);

        PORTA = 0x0D; 
        PORTD = d1;
        delay_ms(1);

        PORTA = 0x0E;  
        PORTD = d0;
        delay_ms(1);
    }

    PORTA = 0x0F;
}

void display_number(unsigned int n)
{
    PORTA=0x0E; PORTD=digit[n%10]; delay_ms(2);
    PORTA=0x0D; PORTD=digit[(n/10)%10]; delay_ms(2);
    PORTA=0x0B; PORTD=digit[(n/100)%10]; delay_ms(2);
    PORTA=0x07; PORTD=digit[n/1000]; delay_ms(2);
    PORTA=0x0F;
}

unsigned char edge_triggering(){
    static int pressed = 1;
    unsigned char key_val = PORTC & 0x0f;
    
    if(key_val != 0x00 && pressed == 1){
        pressed = 0;
        return key_val;
    }
    if(key_val == 0x00){
        pressed = 1;
    }
    return NO_KEY;
}

void main(void)
{
    ADCON1=0x0F; CMCON=0x07;
    TRISC|=0x0F;
    TRISD=0x00;
    TRISA&=0xF0;
    PORTA=0x0F; PORTD=0x00;
    TRISB0 = 0;
    RB0 = 0;
    init_can();

    my_node_id = read_eeprom(0) | (read_eeprom(1)<<8);

    while(1)
    {
        unsigned char key = edge_triggering();

        if(can_receive())
        {
            if(sidh == 0x6B && (sidl & 0xE0) == 0xC0){
                unsigned int rx_id = (can_payload[D0]-'0')*1000 + (can_payload[D1]-'0')*100 + (can_payload[D2]-'0')*10 + (can_payload[D3]-'0');

                if(rx_id == my_node_id)
                {
                    pick_cnt =(can_payload[D4]-'0')*1000 + (can_payload[D5]-'0')*100 + (can_payload[D6]-'0')*10 + (can_payload[D7]-'0');

                    state = ST_PC;
                }
            }
        }

        switch(state)
        {
            case ST_BLANK:
                PORTA=0x0F;
                RB0 = 0;
                if(key==NID) state=ST_NID;
                break;

            case ST_NID:
                display(0x5e, 0x10, 0x40, 0x54);
//                delay_ms(50);
                display_number(my_node_id);
                if(key==INC) my_node_id++;
                if(key==DEC && my_node_id) my_node_id--;
                if(key==ACK){
                    write_eeprom(0, (unsigned char)(my_node_id & 0xFF));
                    write_eeprom(1, (unsigned char)(my_node_id >> 8));
                    state=ST_BLANK;
                }
                break;

            case ST_PC:
                display_number(pick_cnt);
                static unsigned int tick = 0;
                tick++;
                if(tick > 3000)
                {
                    tick = 0;
                    RB0 ^= 1;
                    if(pick_cnt) pick_cnt--;
                    else state = ST_SEND_ACK;
                }
                break;

            case ST_SEND_ACK:
                RB0 = 1;
                send_ack();
                RB0 = 0;
                state=ST_BLANK;
                break;
        }
    }
}

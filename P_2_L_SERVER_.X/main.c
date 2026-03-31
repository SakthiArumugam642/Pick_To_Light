/*
 * File:   main.c
 * Author: sakthi
 * Pick To Light Project - Server
 * Created on January 1, 2026, 2:34 PM
 */

#include <xc.h>
#include <stdio.h>
#include <string.h>

typedef enum States{
    ST_BLANK,       // Blank screen state (power-on/waiting for ACK)
    ST_NID,         // Node ID configuration
    ST_PC,          // Pick Count configuration
    ST_OPER,        // Operation mode
    ST_WAIT_ACK     // Waiting for ACK from client
}state_t;

state_t state = ST_BLANK;  // Start with blank screen on power-on

#define TRUE 1
#define FALSE 0
unsigned int pick_cnt = 0;
unsigned int node_id = 0;

// Keys
#define MODE 0x01   // oper / config
#define CF 0x02     // set node id or set pick cnt
#define INC 0x04    // inc whatever value shown in config mode
#define ACK 0x08    // ack to send value over can or okay key
#define NO_KEY 0x00

// CAN
#define CAN_SET_OPERATION_MODE_NO_WAIT(mode) \
{ \
    CANCON &= 0x1F; \
    CANCON |= mode; \
}

// ENUMS
typedef enum _CanOpMode {
    e_can_op_mode_bits    = 0xE0,
    e_can_op_mode_normal  = 0x00,
    e_can_op_mode_sleep   = 0x20,
    e_can_op_mode_loop    = 0x40,
    e_can_op_mode_listen  = 0x60,
    e_can_op_mode_config  = 0x80
} CanOpMode;

void delay_ms(unsigned int ms)
{
    unsigned int i, j;
    for (i = 0; i < ms; i++)
    {
        for (j = 120; j--; );
    }
}

void init_can(void)
{
    TRISB2 = 0;
    TRISB3 = 1;

    // Enter config mode
    CANCON = 0x80;
    while ((CANSTAT & 0xE0) != 0x80);

    ECANCON = 0x00;

    BRGCON1 = 0xe1;
    BRGCON2 = 0x1b;
    BRGCON3 = 0x03;

    RXB0CON = 0x20;

    CANCON = 0x00;
    while ((CANSTAT & 0xE0) != 0x00);
}

void can_transmit(unsigned char * buf){
    /* Set data in CAN transmit buffer */
    TXB0EIDH = 0x00;
    TXB0EIDL = 0x00;
    TXB0SIDH = 0x6B;
    TXB0SIDL = 0xC0;
    TXB0DLC = 0x08;  // 8 bytes of data
    TXB0D0 = buf[0];
    TXB0D1 = buf[1];
    TXB0D2 = buf[2];
    TXB0D3 = buf[3];
    TXB0D4 = buf[4];
    TXB0D5 = buf[5];
    TXB0D6 = buf[6];
    TXB0D7 = buf[7];
    TXB0REQ = 1;
    while(TXB0REQ);
}

unsigned char check_can_received(){
    // Check if message received in RXB0
    if(RXB0CONbits.RXFUL){
        RXB0CONbits.RXFUL = 0;  // Clear flag
        return TRUE;
    }
    return FALSE;
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

// 7-segment patterns for 0-9
unsigned char digit[] = {
     0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F  // 9
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
void display_number(unsigned int num){
    unsigned char d0, d1, d2, d3;
    d0 = num % 10;
    d1 = (num / 10) % 10;
    d2 = (num / 100) % 10;
    d3 = (num / 1000);
    display(digit[d0], digit[d1], digit[d2], digit[d3]);
}

void init_config(){

    ADCON1 = 0x0F;  // All digital I/O
    CMCON  = 0x07;
    // Keys input
    TRISC |= 0x0f;
    
    // SSD outputs
    TRISD = 0x00;
    TRISA &= 0xF0;
    
    // Initialize OFF
    PORTD = 0x00;
    PORTA &= 0xF0;
    
    // Initialize CAN
     init_can();
}



void main(void) {
    init_config();
    unsigned char buf[10];
    unsigned char key;
    
    while(1){
        key = edge_triggering();
        
        switch(state){
            case ST_BLANK:
                // Show ----
                display(0x08, 0x08, 0x08, 0x08);
                
                if(key == MODE){
                    state = ST_NID;
                }
                break;
                
            case ST_OPER:
                // Operation mode - show ----
                display(0x08, 0x08, 0x08, 0x08);
                
                if(key == MODE){
                    state = ST_NID;
                }
                else if(key == ACK && pick_cnt != 0){
                    // Transmit over CAN
                    sprintf(buf,"%04d%04d", node_id, pick_cnt);
                    can_transmit(buf);
                    state = ST_WAIT_ACK;
                }
                else if(key == ACK && pick_cnt == 0){
                    state = ST_PC;
                }
                break;
                
            case ST_NID:
                // Show "n-id" first
                if(key == NO_KEY && node_id == 0){
                    display(0x5e, 0x10, 0x40, 0x54); // n-id
                }
                else{
                    display_number(node_id);
                }
                
                if(key == CF || key == ACK){
                    state = ST_PC;
                }
                else if(key == MODE){
                    state = ST_OPER;
                }
                else if(key == INC){
                    if(node_id < 9999) node_id++;
                }
                break;
                
            case ST_PC:
                // Show "U-st" first
                if(key == NO_KEY && pick_cnt == 0){
                    display(0x78, 0x6d, 0x40, 0x3e); // U-st
                }
                else{
                    display_number(pick_cnt);
                }
                
                if(key == CF){
                    state = ST_NID;
                }
                else if(key == INC){
                    if(pick_cnt < 9999) pick_cnt++;
                }
                else if(key == MODE || key == ACK){
                    state = ST_OPER;
                }
                break;
                
            case ST_WAIT_ACK:
                // Show ---- and wait
                display(0x08, 0x08, 0x08, 0x08);
                
                if(RXB0CONbits.RXFUL)
                {
                    if(RXB0D0 == 0xAA)
                    {
                        RXB0CONbits.RXFUL = 0;
                        PIR3bits.RXB0IF = 0;
                        pick_cnt = 0;
                        state = ST_BLANK;
                    }
                }
                break;
        }
    }
    return;
}
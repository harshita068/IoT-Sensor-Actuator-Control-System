#include <reg51.h>
#include <stdio.h>
#include <string.h>

#define LCD_PORT P0
sbit RS = P2^6;
//sbit RW = P3^6;
sbit E  = P2^7;

sbit DHT11 = P1^7;
sbit TOUCH_SENSOR   = P1^6;
sbit IR_SENSOR      = P1^5;
sbit RAIN_SENSOR    = P1^4;
sbit GAS_SENSOR     = P1^3;
sbit SOIL_SENSOR    = P1^2;
sbit FLAME_SENSOR   = P1^1;
sbit PROXIMITY_SENSOR = P1^0;
sbit VIBRATION_SENSOR = P1^2;
sbit SOUND_SENSOR   = P1^0;

sbit FAN    = P3^2;
sbit BUZZER = P3^3;
sbit PUMP   = P3^4;

int I_RH, D_RH, I_Temp, D_Temp, CheckSum;
char dat[40];

void delay(unsigned int time);
void UART_Init();
char UART_Receive();
void UART_Transmit(char ch);
void UART_SendString(char* str);
void timer_delay20ms();
void timer_delay30us();
void Request();
void Response();
int Receive_data();
void LCD_Command(unsigned char cmd);
void LCD_Char(unsigned char dat);
void LCD_Init();
void LCD_String(char* str);
void LCD_Clear();

void delay(unsigned int time) {
    unsigned int i, j;
    for (i = 0; i < time; i++) {
        for (j = 0; j < 1275; j++);
    }
}

void UART_Init() {
    TMOD |= 0x20;
    TH1 = 0xFD;
    SCON = 0x50;
    TR1 = 1;
}

char UART_Receive() {
    while (RI == 0);
    RI = 0;
    return SBUF;
}

void UART_Transmit(char ch) {
    SBUF = ch;
    while (TI == 0);
    TI = 0;
}

void UART_SendString(char* str) {
    while (*str)
        UART_Transmit(*str++);
}

void LCD_Command(unsigned char cmd) {
    LCD_PORT = cmd;
    RS = 0;
    //RW = 0;
    E = 1;
    delay(2);
    E = 0;
}

void LCD_Char(unsigned char dat) {
    LCD_PORT = dat;
    RS = 1;
    //RW = 0;
    E = 1;
    delay(2);
    E = 0;
}

void LCD_Init() {
    LCD_Command(0x38);
    LCD_Command(0x0C);
    LCD_Command(0x01);
    LCD_Command(0x06);
    LCD_Command(0x80);
}

void LCD_String(char* str) {
    while (*str)
        LCD_Char(*str++);
}

void LCD_Clear() {
    LCD_Command(0x01);
    LCD_Command(0x80);
}

void timer_delay20ms() {
    TMOD |= 0x01;
    TH0 = 0xB8;
    TL0 = 0x0C;
    TR0 = 1;
    while (TF0 == 0);
    TR0 = 0;
    TF0 = 0;
}

void timer_delay30us() {
    TMOD |= 0x01;
    TH0 = 0xFF;
    TL0 = 0xF1;
    TR0 = 1;
    while (TF0 == 0);
    TR0 = 0;
    TF0 = 0;
}

void Request() {
    DHT11 = 0;
    timer_delay20ms();
    DHT11 = 1;
}

void Response() {
    while (DHT11 == 1);
    while (DHT11 == 0);
    while (DHT11 == 1);
}

int Receive_data() {
    int q, c = 0;
    for (q = 0; q < 8; q++) {
        while (DHT11 == 0);
        timer_delay30us();
        if (DHT11 == 1)
            c = (c << 1) | 1;
        else
            c <<= 1;
        while (DHT11 == 1);
    }
    return c;
}

void main() {
    char command;

    UART_Init();
    LCD_Init();
    LCD_String("System Ready");

    while (1) {
        command = UART_Receive();

        if (command == 'R') {
            Request();
            Response();
            I_RH = Receive_data();
            D_RH = Receive_data();
            I_Temp = Receive_data();
            D_Temp = Receive_data();
            CheckSum = Receive_data();

            LCD_Clear();

            if ((I_RH + D_RH + I_Temp + D_Temp) == CheckSum) {
                sprintf(dat, "Hum:%d.%d%%", I_RH, D_RH);
                LCD_String(dat);
                UART_SendString(dat);
                UART_SendString("\r\n");

                LCD_Command(0xC0);
                sprintf(dat, "Temp:%d.%dC", I_Temp, D_Temp);
                LCD_String(dat);
                UART_SendString(dat);
                UART_SendString("\r\n");
            } else {
                LCD_String("Checksum Error.");
                UART_SendString("Checksum Error.\r\n");
            }
        } else if (command == 'T') {
            LCD_Clear();
            if (TOUCH_SENSOR == 0) {
                LCD_String("Touch: Not Touched");
                UART_SendString("Touch: Not Touched\r\n");
            } else {
                LCD_String("Touch: Touched");
                UART_SendString("Touch: Touched\r\n");
            }
        } else if (command == 'I') {
            LCD_Clear();
            if (IR_SENSOR == 1) {
                LCD_String("IR: Object Detected");
                UART_SendString("IR: Object\r\n");
            } else {
                LCD_String("IR: Clear");
                UART_SendString("IR: Clear\r\n");
            }
        } else if (command == 'N') {
            LCD_Clear();
            if (RAIN_SENSOR == 0) {
                LCD_String("Rain: Detected");
                UART_SendString("Rain: Detected\r\n");
            } else {
                LCD_String("Rain: None");
                UART_SendString("Rain: None\r\n");
            }
        } else if (command == 'G') {
            LCD_Clear();
            if (GAS_SENSOR == 1) {
                LCD_String("Gas: Detected");
                UART_SendString("Gas: Detected\r\n");
            } else {
                LCD_String("Gas: None");
                UART_SendString("Gas: None\r\n");
            }
        } else if (command == 'S') {
            LCD_Clear();
            if (SOIL_SENSOR == 0) {
                LCD_String("Soil: Moist");
                UART_SendString("Soil: Moist\r\n");
            } else {
                LCD_String("Soil: Dry");
                UART_SendString("Soil: Dry\r\n");
            }
        } else if (command == 'F') {
            LCD_Clear();
            if (FLAME_SENSOR == 1) {
                LCD_String("Flame: Detected");
                UART_SendString("Flame: Detected\r\n");
            } else {
                LCD_String("Flame: None");
                UART_SendString("Flame: None\r\n");
            }
        } else if (command == 'A') {
            LCD_Clear();
            if (SOUND_SENSOR == 0) {
                LCD_String("Sound: Detected");
                UART_SendString("Sound: Detected\r\n");
            } else {
                LCD_String("Sound: None");
                UART_SendString("Sound: None\r\n");
            }
        } else if (command == 'V') {
            LCD_Clear();
            if (VIBRATION_SENSOR == 1) {
                LCD_String("Vibration: Detected");
                UART_SendString("Vibration: Detected\r\n");
            } else {
                LCD_String("Vibration: None");
                UART_SendString("Vibration: None\r\n");
            }
        } else if (command == 'P') {
            LCD_Clear();
            if (PROXIMITY_SENSOR == 0) {
                LCD_String("Proximity: Detected");
                UART_SendString("Proximity: Detected\r\n");
            } else {
                LCD_String("Proximity: None");
                UART_SendString("Proximity: None\r\n");
            }
        } else if (command == '1') {
            FAN = 0;
            UART_SendString("ACTUATOR: FAN ON\r\n");
        } else if (command == '2') {
            FAN = 1;
            UART_SendString("ACTUATOR: FAN OFF\r\n");
        } else if (command == '3') {
            BUZZER = 0;
            UART_SendString("ACTUATOR: BUZZER ON\r\n");
        } else if (command == '4') {
            BUZZER = 1;
            UART_SendString("ACTUATOR: BUZZER OFF\r\n");
        } else if (command == '5') {
            PUMP = 0;
            UART_SendString("ACTUATOR: PUMP ON\r\n");
        } else if (command == '6') {
            PUMP = 1;
            UART_SendString("ACTUATOR: PUMP OFF\r\n");
        }
    }
}

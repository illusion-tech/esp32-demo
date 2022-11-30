#ifndef _HW507_H_
#define _HW507_H_


#define HW507_PIN CONFIG_HW507_GPIO
 
#define uchar unsigned char
#define uint8 unsigned char
#define uint16 unsigned short

void InputInitial(void); //设置端口为输入
void OutputHigh(void);   //输出1
void OutputLow(void);    //输出0
uint8 getData(); //读取状态
void COM(void); // 温湿写入
void Delay_ms(uint16 ms);
void hw507_init(void);
uchar getTemp();
uchar getHumi();
uchar getHumiSmall();
uchar getTempSmall();



#endif
/*这是BK4802数字电台的第一个正式版固件程序，CLG新一代数字电台，本程序复用了原Rda5820第一代数字电台
的部分代码，并基于此改进优化，本程序分为底层I2C操作层，电台寄存器指令层，系统控制层，显示层四等分。
所有代码均由我独立编写。*/
//项目开始日期：2026年4月16日  编写人：SG
//电台功能：430-440业余无线电U段收发，支持频率调节，RSSI值，信噪比进度条显示，电源电压显示，看门狗防死机

//第一部分-----------------头文件引用部分---------------------
#include <BK4802P.h>//BK4802的驱动库
BK4802P radio;      //创建电台芯片对象

//显示屏的头文件引用及设置
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//看门狗的头文件
#include <avr/wdt.h>


//第二部分------------常量宏定义及全局变量声明------------------

//硬件按钮
#define UPBUTTON 2     //频率加按钮
#define DOWNBUTTON 3   //频率减按钮
#define MENUBUTTON 4   //菜单按钮
#define PTTBUTTON 5    //PTT按钮

#define FREQ_OFFSET 20 //频率修正偏移量

//信噪比，RSSI进度条的位置和大小
#define PROGRESS_SNR_BAR_X 45
#define PROGRESS_SNR_BAR_Y 1
#define PROGRESS_SNR_BAR_WIDTH 80
#define PROGRESS_SNR_BAR_HEIGHT 4

#define PROGRESS_RSSI_BAR_X 48
#define PROGRESS_RSSI_BAR_Y 41
#define PROGRESS_RSSI_BAR_WIDTH 77
#define PROGRESS_RSSI_BAR_HEIGHT 5

//汉字字库
//PROGMEM配置，将汉字数据放置在闪存中
#include <avr/pgmspace.h>
 const uint8_t PROGMEM shu[] = {0x08,0x20,0x49,0x20,0x2A,0x20,0x08,0x3E,0xFF,0x44,0x2A,0x44,0x49,0x44,0x88,0xA4,0x10,0x28,0xFE,0x28,0x22,0x10,0x42,0x10,0x64,0x28,0x18,0x28,0x34,0x44,0xC2,0x82};
 const uint8_t PROGMEM zi[] = {0x02,0x00,0x01,0x00,0x7F,0xFE,0x40,0x02,0x80,0x04,0x1F,0xE0,0x00,0x40,0x00,0x80,0x01,0x00,0xFF,0xFE,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x05,0x00,0x02,0x00};
 const uint8_t PROGMEM dian[] = {0x01,0x00,0x01,0x00,0x01,0x00,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x0A,0x01,0x02,0x01,0x02,0x00,0xFE};
 const uint8_t PROGMEM tai[] = {0x02,0x00,0x02,0x00,0x04,0x00,0x08,0x20,0x10,0x10,0x20,0x08,0x7F,0xFC,0x20,0x04,0x00,0x00,0x1F,0xF0,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1F,0xF0,0x10,0x10};
const uint8_t PROGMEM phone[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4B,0xA4,0xA4,0x88,0x92,0x92,0xAA,0xD8,0x92,0x92,0xEE,0xA8,0x92,0x92,0xAA,0xA8,0x4A,0xA4,0xAA,0xA8,0x02,0x80,0x00,0x00,0x02,0x81,0xFF,0xF0,0x02,0xC0,0x00,0x1C,0x02,0xBF,0xFF,0xE0,0x02,0x80,0x00,0x10,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x3F,0xFF,0x88,0x02,0x20,0x00,0x88,0x02,0x20,0x00,0x88,0x02,0x20,0x00,0x88,0x06,0x20,0x00,0x88,0x06,0x3F,0xFF,0x88,0x06,0x00,0x00,0x08,0x06,0x00,0x00,0x08,0x06,0x00,0x00,0x08,0x06,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x01,0x00,0x00,0x10,0x00,0xFF,0xFF,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};


//全局变量参数
uint32_t currentFreq = 438500; //全局频率（MHz），默认438.5
int SNR;                    //信噪比
int RSSI;                   //RSSI值
int power = 7;              //发射功率
int volume = 50;            //喇叭音量
//int voltage;              //设备电压

bool PTTPressed;              //PTT是否按下
bool SQLState = 1;            //静噪开启标记

const int stepValue = 25;//步进
const uint32_t freqMin = 430000;  //频率下限430M
const uint32_t freqMax = 440000;  //频率上限440M

//这是第一个版本，目前只开放业余无线电U段使用，之后版本会新增



//第三部分--------------系统功能函数实现-----------------

int voltageRead(void){
  int voltage;
  static unsigned long lastVoltRead;
  if (millis() - lastVoltRead > 1000) {  // 每秒读一次电压
    voltage = analogRead(A0) * (5.0 / 1023.0);  // 直接浮点计算
    lastVoltRead = millis();
  }
  return voltage;
}// 电压读取

void PTTcontrol(bool buttonState) {
    static bool lastButtonState = HIGH;  //静态变量，只初始化一次
    if (buttonState != lastButtonState) {
        if (buttonState == LOW) {
            radio.TX_BK4802(currentFreq);
        } else {
            radio.RX_BK4802(currentFreq);
        }
        lastButtonState = buttonState;
    }
}//发射控制

void handleFreqChange(uint32_t &Freq, uint32_t FreqMax, uint32_t FreqMin){
 //全局频率传入引用
 // 频率加减短按处理
 static int lastDown = HIGH, lastUp = HIGH;   //按钮上拉，未按下为 HIGH，记录上次变化状态

 // 频率加减短按处理（只改变变量，不写入寄存器）
 // 不要用那个狗屁防抖！！！
 if (digitalRead(DOWNBUTTON) == LOW && Freq > FreqMin) Freq -= stepValue;
 if (digitalRead(UPBUTTON) == LOW && Freq < FreqMax) Freq += stepValue;

 // 松开按钮的瞬间执行且只执行一次写入（检测上升沿）
 if (lastDown == LOW && digitalRead(DOWNBUTTON) == HIGH) radio.RX_BK4802(Freq);
 if (lastUp == LOW && digitalRead(UPBUTTON) == HIGH) radio.RX_BK4802(Freq);
 //接收模式下设置频率，0代表接收模式

 // 更新状态
 lastDown = digitalRead(DOWNBUTTON);
 lastUp = digitalRead(UPBUTTON);
}//按钮控制振荡器频率，输入当前频率值，频率上下限


//第四部分---------------UI显示函数实现------------------
void init_screen(void){
 if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);//如果显示屏初始化失败就死循环
    }
  display.setTextColor(WHITE);
  display.clearDisplay();
}//显示屏的初始化设置

void displayInitCaption(void){
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.print("BK4802");  //BK4802，首行顶格
    display.setCursor(3, 19);
    display.setTextSize(2);
    display.print("->");  
    display.drawBitmap(35, 18, shu, 16, 16, WHITE);
    display.drawBitmap(51, 18, zi, 16, 16, WHITE);
    display.drawBitmap(67, 18, dian, 16, 16, WHITE);
    display.drawBitmap(83, 18, tai, 16, 16, WHITE);
    //中文：数字电台
    display.drawBitmap(98, 0, phone, 32, 40, WHITE);
    //对讲机图标
    display.setCursor(8, 38);
    display.setTextSize(2);
    display.print("V1.0");  //版本号
    display.setCursor(67, 55);
    display.setTextSize(1);
    display.print("HF VHF UHF");//小标识  
    display.display();
    delay(3000); // 显示3秒
}// 显示初始化信息

void displayProgressSNRBar(int SNRValue){
  display.setTextSize(1);
  display.setCursor(26, 0);
  display.print("SNR");//信噪比小标识
  int progressValue = map(SNRValue, 0, 63, 0, 100);//信噪比范围是0-63
  // 绘制进度条边框
   display.drawRect(PROGRESS_SNR_BAR_X, PROGRESS_SNR_BAR_Y, PROGRESS_SNR_BAR_WIDTH, PROGRESS_SNR_BAR_HEIGHT, SSD1306_WHITE);
   // 计算进度条的填充长度
   int fillWidth = (progressValue * PROGRESS_SNR_BAR_WIDTH) / 100;
   // 绘制进度条填充部分
   display.fillRect(PROGRESS_SNR_BAR_X, PROGRESS_SNR_BAR_Y, fillWidth, PROGRESS_SNR_BAR_HEIGHT, SSD1306_WHITE);
   // 显示当前进度值
   display.setCursor(PROGRESS_SNR_BAR_X, PROGRESS_SNR_BAR_Y + PROGRESS_SNR_BAR_HEIGHT + 5);
} //显示信噪比进度条

void displayProgressRSSIBar(int RSSIValue){
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("RSSI ");
  display.println(RSSIValue);//RSSI值显示
  int progressValue = map(RSSIValue, 0, 127, 0, 100);//RSSI范围是0-127
  // 绘制进度条边框
   display.drawRect(PROGRESS_RSSI_BAR_X, PROGRESS_RSSI_BAR_Y, PROGRESS_RSSI_BAR_WIDTH, PROGRESS_RSSI_BAR_HEIGHT, SSD1306_WHITE);
   // 计算进度条的填充长度
   int fillWidth = (progressValue * PROGRESS_RSSI_BAR_WIDTH) / 100;
   // 绘制进度条填充部分
   display.fillRect(PROGRESS_RSSI_BAR_X, PROGRESS_RSSI_BAR_Y, fillWidth, PROGRESS_RSSI_BAR_HEIGHT, SSD1306_WHITE);
   // 显示当前进度值
   display.setCursor(PROGRESS_RSSI_BAR_X, PROGRESS_RSSI_BAR_Y + PROGRESS_RSSI_BAR_HEIGHT + 5);
} //显示RSSI进度条

void displayVoltageValue(float voltage){
    display.setCursor(0, 55);
    display.print(voltage);
    display.println("V");
} //显示电压

void displayPwrValue(int PwrLevel){
  display.setCursor(35, 55);
    display.print("PWR:");
    display.println(PwrLevel);
} //显示功率值

void displaySQLState(bool isSQL){
  if(isSQL){
    display.setCursor(70, 55);
    display.print("SQ");
  }
} //显示静噪状态

void displayVolValue(int volume){
  display.setCursor(90, 55);
    display.print("VOL:");
    display.println(volume);
} //显示音量值

void displayPTT(bool isPTT){
  if (!isPTT){//按钮按下为低电平，所以反相
     display.setCursor(0, 0);
     display.setTextSize(1);
     display.print("TX");
   }else{
     display.setCursor(0, 0);
     display.setTextSize(1);
     display.print("RX");
  }
}//显示发射状态

void displayFreqBand(uint32_t freq_khz) {
    // BK4802 支持的频率段 (整数型kHz): 24000-32000, 35000-46000, 43000-57000, 128000-170000, 384000-512000
    // 1. 确定波段标签 F1~F5
    const char* bandLabel = nullptr;
    if (freq_khz >= 24000UL && freq_khz <= 32000UL) {
        bandLabel = "F1";
    } else if (freq_khz >= 35000UL && freq_khz <= 46000UL) {
        bandLabel = "F2";
    } else if (freq_khz > 46000UL && freq_khz <= 57000UL) { // 注意：>46kHz 且 <=57kHz
        bandLabel = "F3";
    } else if (freq_khz >= 128000UL && freq_khz <= 170000UL) {
        bandLabel = "F4";
    } else if (freq_khz >= 384000UL && freq_khz <= 512000UL) {
        bandLabel = "F5";
    }

    // 显示波段标签
    display.setCursor(0, 26);
    if (bandLabel) {
        display.print(bandLabel);
    } else {
        display.print("   ");   // 清空区域
    }

    // 2. 确定频率类型 HF / VHF / UHF (阈值：30MHz=30000kHz, 300MHz=300000kHz)
    const char* typeLabel = nullptr;
    if (freq_khz >= 24000UL && freq_khz <= 30000UL) {
        typeLabel = "HF";
    } else if (freq_khz > 30000UL && freq_khz <= 300000UL) {
        typeLabel = "VHF";
    } else if (freq_khz > 300000UL && freq_khz <= 512000UL) {
        typeLabel = "UHF";
    }

    // 显示频率类型
    display.setCursor(0, 16);
    display.setTextSize(1);
    if (typeLabel) {
        display.print(typeLabel);
    } else {
        display.print("   ");
    }
}

void displayFreqValue(uint32_t freq_khz){
    display.setCursor(23, 17);
    display.setTextSize(2);
    display.print(freq_khz/1000.0, 3);  //转换成兆赫兹显示
	  //显示频率
    display.setCursor(110, 24);
    display.setTextSize(1);
    display.print("MHz");
}//频率值及显示


//第五部分-----------------主程序-------------------
void setup(){
    Serial.begin(9600); 
    //开启串口通信
  pinMode(UPBUTTON, INPUT_PULLUP); 
   pinMode(DOWNBUTTON, INPUT_PULLUP); 
   pinMode(PTTBUTTON, INPUT_PULLUP);
   pinMode(A0, INPUT_PULLUP);
  // 设置引脚为输入上拉模式

  Wire.begin();// 初始化 I2C 总线
    delay(50);
    radio.RegistersInit();                    // 初始化芯片寄存器
    radio.RX_BK4802(currentFreq);             // 设置接收频率
    radio.BK4802PwrSet(power);                // 最大发射功率（若发射时）
    //radio.BK4802VolSet(volume);             // 音量 
    //radio.BK4802EasySQLSet(SQLState);       // 开启静噪
  //radio.BK4802AutoTailToneSet(0);         // 关闭自动消尾音

  init_screen();
   //屏幕初始化设置
   displayInitCaption();
   //初始化信息显示
   display.clearDisplay();
   display.setCursor(0, 0);
  //再次清屏，准备进入正常显示流程

  wdt_enable(WDTO_2S);
  //启用看门狗，超时设置2s

}

void loop(){
  wdt_reset();//喂狗
  PTTPressed = digitalRead(PTTBUTTON); //loop里面循环读取PTT状态
  //只有PTT状态是全局变量，直接受按钮控制，该状态机用于控制PTT和显示收发状态
  //其余均采用参数传递
  handleFreqChange(currentFreq, freqMax, freqMin);
  PTTcontrol(PTTPressed);
  //显示程序（从左上角到右下角依次刷新）
   displayPTT(PTTPressed);                       //PTT收发状态
   displayProgressSNRBar(radio.BK4802SNRGet());  //显示信噪比
   displayFreqBand(currentFreq);                 //根据当前频率显示频段参数
   displayFreqValue(currentFreq);                //显示频率值
   displayProgressRSSIBar(radio.BK4802RSSIGet());//显示RSSI
   displayVoltageValue(voltageRead());           //显示电压
   displayPwrValue(power);                       //当前发射功率
   displaySQLState(SQLState);                    //静噪状态
   displayVolValue(volume);                      //音量
   display.display();
   display.clearDisplay();
  //循环结束后仅清一次屏

}




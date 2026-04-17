//本程序为BK4802电台的UI显示程序，不包含系统控制，射频芯片的驱动程序
//开发日期：2026年4月15日
//UI通用界面显示与控制程序

//显示屏的头文件引用及设置
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//信噪比，RSSI进度条的位置和大小
#define PROGRESS_SNR_BAR_X 45
#define PROGRESS_SNR_BAR_Y 1
#define PROGRESS_SNR_BAR_WIDTH 80
#define PROGRESS_SNR_BAR_HEIGHT 4

#define PROGRESS_RSSI_BAR_X 45
#define PROGRESS_RSSI_BAR_Y 41
#define PROGRESS_RSSI_BAR_WIDTH 80
#define PROGRESS_RSSI_BAR_HEIGHT 5

//汉字字库
//PROGMEM配置，将汉字数据放置在闪存中
#include <avr/pgmspace.h>
 const uint8_t PROGMEM shu[] = {0x08,0x20,0x49,0x20,0x2A,0x20,0x08,0x3E,0xFF,0x44,0x2A,0x44,0x49,0x44,0x88,0xA4,0x10,0x28,0xFE,0x28,0x22,0x10,0x42,0x10,0x64,0x28,0x18,0x28,0x34,0x44,0xC2,0x82};
 const uint8_t PROGMEM zi[] = {0x02,0x00,0x01,0x00,0x7F,0xFE,0x40,0x02,0x80,0x04,0x1F,0xE0,0x00,0x40,0x00,0x80,0x01,0x00,0xFF,0xFE,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x05,0x00,0x02,0x00};
 const uint8_t PROGMEM dian[] = {0x01,0x00,0x01,0x00,0x01,0x00,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x0A,0x01,0x02,0x01,0x02,0x00,0xFE};
 const uint8_t PROGMEM tai[] = {0x02,0x00,0x02,0x00,0x04,0x00,0x08,0x20,0x10,0x10,0x20,0x08,0x7F,0xFC,0x20,0x04,0x00,0x00,0x1F,0xF0,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1F,0xF0,0x10,0x10};
const uint8_t PROGMEM phone[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4B,0xA4,0xA4,0x88,0x92,0x92,0xAA,0xD8,0x92,0x92,0xEE,0xA8,0x92,0x92,0xAA,0xA8,0x4A,0xA4,0xAA,0xA8,0x02,0x80,0x00,0x00,0x02,0x81,0xFF,0xF0,0x02,0xC0,0x00,0x1C,0x02,0xBF,0xFF,0xE0,0x02,0x80,0x00,0x10,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x3F,0xFF,0x88,0x02,0x20,0x00,0x88,0x02,0x20,0x00,0x88,0x02,0x20,0x00,0x88,0x06,0x20,0x00,0x88,0x06,0x3F,0xFF,0x88,0x06,0x00,0x00,0x08,0x06,0x00,0x00,0x08,0x06,0x00,0x00,0x08,0x06,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x01,0x00,0x00,0x10,0x00,0xFF,0xFF,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};


//显示方面函数

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
  if (isPTT){
     display.setCursor(0, 0);
     display.setTextSize(1);
     display.print("TX");
   }else{
     display.setCursor(0, 0);
     display.setTextSize(1);
     display.print("RX");
  }
}//显示发射状态

void displayFreqBand(float freq) {
    // BK4802 支持的频率段 (MHz): 24-32, 35-46, 43-57, 128-170, 384-512
    // 1. 确定波段标签 F1~F5
    const char* bandLabel = nullptr;
    if (freq >= 24.0f && freq <= 32.0f) {
        bandLabel = "F1";
    } else if (freq >= 35.0f && freq <= 46.0f) {
        bandLabel = "F2";
    } else if (freq > 46.0f && freq <= 57.0f) {  // 注意：原注释有 43-57，但 43-46 已归入 F2
        bandLabel = "F3";
    } else if (freq >= 128.0f && freq <= 170.0f) {
        bandLabel = "F4";
    } else if (freq >= 384.0f && freq <= 512.0f) {
        bandLabel = "F5";
    }

    // 显示波段标签（位置 Y=26）
    display.setCursor(0, 26);
    if (bandLabel) {
        display.print(bandLabel);
    } else {
        display.print("   ");  // 清空该区域（假设标签宽度为3字符）
    }

    // 2. 确定频率类型 HF / VHF / UHF
    const char* typeLabel = nullptr;
    if (freq >= 24.0f && freq <= 30.0f) {
        typeLabel = "HF";
    } else if (freq > 30.0f && freq <= 300.0f) {
        typeLabel = "VHF";
    } else if (freq > 300.0f && freq <= 512.0f) {
        typeLabel = "UHF";
    }

    // 显示频率类型（位置 Y=16）
    display.setCursor(0, 16);
    display.setTextSize(1);  // 确保文字大小一致
    if (typeLabel) {
        display.print(typeLabel);
    } else {
        display.print("   ");
    }
}//显示频段信息

void displayFreqValue(double Freq){
    display.setCursor(23, 17);
    display.setTextSize(2);
    display.print(Freq, 3);  
	  //显示频率
    display.setCursor(110, 24);
    display.setTextSize(1);
    display.print("MHz");
}//频率值及显示



void setup() {
  init_screen();
  //屏幕初始化设置
  displayInitCaption();
  //初始化信息显示
  display.clearDisplay();
  display.setCursor(0, 0);
  //再次清屏，准备进入正常显示流程

}

void loop() {
 displayPTT(0);
 displayProgressSNRBar(20);
 displayFreqBand(144);
 displayFreqValue(144);
 displayProgressRSSIBar(60);
 displayVoltageValue(7);
 displayPwrValue(5);
 displaySQLState(30);
 displayVolValue(40);
 display.display();
 display.clearDisplay();
 //循环结束后仅清一次屏

}

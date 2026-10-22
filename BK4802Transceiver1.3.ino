/*1.3更新内容：增加错误返回，如果电台屏幕在初始化时候发现不可用，将保留电台的发射和接收功能，不会卡死循环
如果电台芯片不可用，屏幕能用的话，会显示芯片故障，如果两个都不可用，那神仙都救不了了，机子直接丢垃圾桶就行
还修改了频率补偿逻辑，将补偿值加进参数数组内，可直接修改，方便省事*/
//更新日期：2026年10月22日  编写人：SG
//电台功能：增加初始化自检错误返回，降级处理逻辑，优化频率补偿
//注意：BK4802库要升级到最新版才能启用音量，静噪等辅助功能

// ==================== 第一部分：头文件 ====================
#include <BK4802P.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <avr/wdt.h>
#include <EEPROM.h>//新增EEPROM储存参数
#include <avr/pgmspace.h>

BK4802P radio;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ==================== 第二部分：常量和全局变量 ====================
#define UPBUTTON 2     //频率加按钮
#define DOWNBUTTON 3   //频率减按钮
#define MENUBUTTON 4   //菜单按钮
#define PTTBUTTON 5    //PTT按钮

#define STEP_VALUE  25          // 频率步进 25kHz
#define NUM_BANDS   5           // 频段数量
#define SYSTEM_VERSION "V1.3"   //版本号


// 进度条位置
#define PROGRESS_SNR_BAR_X 45
#define PROGRESS_SNR_BAR_Y 1
#define PROGRESS_SNR_BAR_WIDTH 80
#define PROGRESS_SNR_BAR_HEIGHT 4

#define PROGRESS_RSSI_BAR_X 48
#define PROGRESS_RSSI_BAR_Y 41
#define PROGRESS_RSSI_BAR_WIDTH 77
#define PROGRESS_RSSI_BAR_HEIGHT 5

// 汉字点阵
//数
const uint8_t PROGMEM shu[] = {0x08,0x20,0x49,0x20,0x2A,0x20,0x08,0x3E,0xFF,0x44,0x2A,0x44,0x49,0x44,0x88,0xA4,0x10,0x28,0xFE,0x28,0x22,0x10,0x42,0x10,0x64,0x28,0x18,0x28,0x34,0x44,0xC2,0x82};
//字
const uint8_t PROGMEM zi[] = {0x02,0x00,0x01,0x00,0x7F,0xFE,0x40,0x02,0x80,0x04,0x1F,0xE0,0x00,0x40,0x00,0x80,0x01,0x00,0xFF,0xFE,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x05,0x00,0x02,0x00};
//电
const uint8_t PROGMEM dian[] = {0x01,0x00,0x01,0x00,0x01,0x00,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x0A,0x01,0x02,0x01,0x02,0x00,0xFE};
//台
const uint8_t PROGMEM tai[] = {0x02,0x00,0x02,0x00,0x04,0x00,0x08,0x20,0x10,0x10,0x20,0x08,0x7F,0xFC,0x20,0x04,0x00,0x00,0x1F,0xF0,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1F,0xF0,0x10,0x10};
//无
const uint8_t PROGMEM wu[] = {0x00,0x00,0x3F,0xF0,0x02,0x00,0x02,0x00,0x02,0x00,0x02,0x00,0x7F,0xFC,0x04,0x80,0x04,0x80,0x04,0x80,0x08,0x80,0x08,0x80,0x10,0x84,0x20,0x84,0x40,0x7C,0x80,0x00};
//线
const uint8_t PROGMEM xian[] = {0x10,0x50,0x10,0x48,0x20,0x40,0x24,0x5C,0x45,0xE0,0xF8,0x40,0x10,0x5E,0x23,0xE0,0x40,0x44,0xFC,0x48,0x40,0x30,0x00,0x22,0x1C,0x52,0xE0,0x8A,0x43,0x06,0x00,0x02};
//芯
const uint8_t PROGMEM xin[] = {0x08,0x20,0x08,0x20,0xFF,0xFE,0x08,0x20,0x08,0x20,0x02,0x00,0x01,0x00,0x00,0x80,0x04,0x08,0x24,0x04,0x24,0x02,0x44,0x02,0x84,0x10,0x04,0x10,0x03,0xF0,0x00,0x00};
//片
const uint8_t PROGMEM pian[] = {0x00,0x40,0x10,0x40,0x10,0x40,0x10,0x40,0x10,0x40,0x1F,0xFC,0x10,0x00,0x10,0x00,0x10,0x00,0x1F,0xE0,0x10,0x20,0x10,0x20,0x10,0x20,0x20,0x20,0x20,0x20,0x40,0x20};
//故
const uint8_t PROGMEM gu[] = {0x10,0x40,0x10,0x40,0x10,0x40,0x10,0x80,0xFE,0xFE,0x11,0x08,0x12,0x88,0x10,0x88,0x7C,0x88,0x44,0x50,0x44,0x50,0x44,0x20,0x44,0x50,0x7C,0x88,0x45,0x04,0x02,0x02};
//障
const uint8_t PROGMEM zhang[] = {0x00,0x20,0x7D,0xFC,0x44,0x88,0x48,0x50,0x4B,0xFE,0x50,0x00,0x49,0xFC,0x49,0x04,0x45,0xFC,0x45,0x04,0x45,0xFC,0x68,0x20,0x53,0xFE,0x40,0x20,0x40,0x20,0x40,0x20};
//设
const uint8_t PROGMEM she[] = {0x00,0x00,0x21,0xF0,0x11,0x10,0x11,0x10,0x01,0x10,0x02,0x0E,0xF4,0x00,0x13,0xF8,0x11,0x08,0x11,0x10,0x10,0x90,0x14,0xA0,0x18,0x40,0x10,0xA0,0x03,0x18,0x0C,0x06};
//置
const uint8_t PROGMEM zhi[] = {0x7F,0xFC,0x44,0x44,0x7F,0xFC,0x01,0x00,0x7F,0xFC,0x01,0x00,0x1F,0xF0,0x10,0x10,0x1F,0xF0,0x10,0x10,0x1F,0xF0,0x10,0x10,0x1F,0xF0,0x10,0x10,0xFF,0xFE,0x00,0x00};

//电台图标
const uint8_t PROGMEM phone[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4B,0xA4,0xA4,0x88,0x92,0x92,0xAA,0xD8,0x92,0x92,0xEE,0xA8,0x92,0x92,0xAA,0xA8,0x4A,0xA4,0xAA,0xA8,0x02,0x80,0x00,0x00,0x02,0x81,0xFF,0xF0,0x02,0xC0,0x00,0x1C,0x02,0xBF,0xFF,0xE0,0x02,0x80,0x00,0x10,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x3F,0xFF,0x88,0x02,0x20,0x00,0x88,0x02,0x20,0x00,0x88,0x02,0x20,0x00,0x88,0x06,0x20,0x00,0x88,0x06,0x3F,0xFF,0x88,0x06,0x00,0x00,0x08,0x06,0x00,0x00,0x08,0x06,0x00,0x00,0x08,0x06,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x7F,0xFF,0xC8,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x01,0x00,0x00,0x10,0x00,0xFF,0xFF,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
//芯片故障图标
const uint8_t PROGMEM chip_error_icon[] = {0x39,0xC2,0x36,0x42,0xE0,0x72,0x20,0x42,0xE0,0x70,0x20,0x42,0xE0,0x70,0x23,0x43,0xE1,0xF6,0x20,0xCC,0xE0,0x78,0x20,0x30,0x3F,0xF8,0x00,0xCC,0x01,0x86,0x03,0x03};

// 频段结构体
struct Band {
    uint32_t freqMin;      // 最低频率 (kHz)
    uint32_t freqMax;      // 最高频率 (kHz)
    uint32_t defaultFreq;  // 默认频率 (kHz) – 首次使用或 EEPROM 无效时使用
    uint32_t currentFreq;  // 当前频率 (kHz) – 运行时修改，并保存到 EEPROM
    int8_t freqCorrection; // 频率值修正系数
    const char* label;     // 显示标签 "F1", "F2"...
    const char* typeLabel; // 波段类型 "HF", "VHF", "UHF"（可选）
};

// 频段数组（参数数组）
Band bands[NUM_BANDS] = {
    { 28000, 29700, 28500, 28500, 0, "F1", "HF" },   // 10米波段
    { 50000, 54000, 51500, 51500, 2, "F2", "VHF" },  // 6米波段
    { 144000, 148000, 145000, 145000, 5, "F3", "VHF" }, // 2米波段
    { 409000, 410000, 409700, 409700, 10, "F4", "UHF" }, // 公众段
    { 430000, 440000, 438500, 438500, 10, "F5", "UHF" }  // 业余U段
};

uint8_t currentBand = 0;   // 当前频段索引
uint8_t power = 7;         //发射功率
uint8_t volume = 50;       //音量
bool SQLState = true;      //静噪状态
bool PTTPressed = false;   //PTT状态（是否被按下）
bool screen_available = false;//检查屏幕是否可用（1.3）

// ==================== 第三部分：函数声明（避免顺序问题） ====================
float voltageRead(void);// 电压读取(返回float)
void PTTcontrol(bool buttonState);//发射控制
void handleFreqChange();//按钮控制振荡器频率，输入当前频率值，频率上下限(1.1更新：全部从数组里拿数据)
void handleMenuButton();//处理菜单按键控制，按一次改变当前频段（1.1新增）
void saveBandFreq(int bandIdx, uint32_t freq);// 保存某个频段的当前频率到 EEPROM（1.1新增）
uint32_t loadBandFreq(int bandIdx, uint32_t defaultFreq);// 从 EEPROM 读取某个频段的频率，如果无效则返回默认值（1.1新增）
void saveCurrentBandIndex(int idx);// 保存当前选中的频段索引（1.1新增）
int loadCurrentBandIndex();// 加载上次保存的频段索引，如果无效则返回 0（1.1新增）
void readFromEEPROM();//初始化时加载EEPROM（1.1新增）
uint32_t compFreq(uint32_t freq);// U段频率补偿 (修复1.2版本10kHz偏差)
void radioErrorLoop(void);//BK4802芯片错误处理（1.3新增）
bool init_screen(void);//显示屏的初始化设置
void displayInitCaption(void);//显示开屏信息
void displayRadioError(void);//显示BK4802芯片初始化错误信息（1.3新增）
void displayProgressSNRBar(int SNRValue);//显示SNR进度条
void displayProgressRSSIBar(int RSSIValue);//显示RSSI进度条
void displayVoltageValue(float voltage);//显示电压
void displayPwrValue(int PwrLevel);//显示发射功率
void displaySQLState(bool isSQL);//显示静噪状态
void displayVolValue(int volume);//显示音量值
void displayPTT(bool isPTT);//显示PTT状态
void displayBandInfo();//显示频段信息
void displayFreqValue(uint32_t freq_khz);//显示频率值

// ==================== 第四部分：功能实现 ====================

uint32_t compFreq(uint32_t freq) {
    int32_t corrected = (int32_t)freq + bands[currentBand].freqCorrection;
    //这一步是因为有些补偿值可能是负数，uint没有符号结果回绕就麻烦了，所以必须转到有符号运算
    if (corrected < 0) corrected = 0;   // 防御，虽然实际不会到 0
    return (uint32_t)corrected;         //返回无符号32位整数，对齐之前版本
}// 频率补偿 (1.2C版本优化而来，直接从参数数组取值)

float voltageRead() {
    static float voltage = 0;//依旧静态变量，避免重蹈覆辙
    static unsigned long lastVoltRead = 0;
    if (millis() - lastVoltRead > 1000) {
        voltage = analogRead(A0) * (5.0 / 1023.0);
        lastVoltRead = millis();
    }
    return voltage;
}// 电压读取(返回float)

void radioErrorLoop(void){
    wdt_enable(WDTO_2S); //进来了，把这里当自己家，养条小狗吧
    while(1){
        if (screen_available){
            displayRadioError();
            delay(500);
        }else{
            delay(500);
        }
        wdt_reset();//喂狗
    }
}//BK4802芯片错误处理(函数永不返回，一旦进入这个，连loop都不会进，电台就用不了啦hiahiahia)

void PTTcontrol(bool buttonState) {
    static bool lastButtonState = HIGH;
    if (buttonState != lastButtonState) {
        if (buttonState == LOW) {
            radio.TX_BK4802(compFreq(bands[currentBand].currentFreq)); // 发射补偿
        } else {
            radio.RX_BK4802(compFreq(bands[currentBand].currentFreq)); // 接收补偿
        }
        lastButtonState = buttonState;
    }
}//发射控制

void handleFreqChange() {
    // 发射时不允许改变频率（1.1新增）
    if (digitalRead(PTTBUTTON) == LOW) return;

    static int lastDown = HIGH, lastUp = HIGH;
    bool down = digitalRead(DOWNBUTTON);
    bool up = digitalRead(UPBUTTON);
    Band& b = bands[currentBand];
    //依旧不用狗屁防抖
    if (down == LOW && b.currentFreq > b.freqMin) {
        b.currentFreq -= STEP_VALUE;
    }
    if (up == LOW && b.currentFreq < b.freqMax) {
        b.currentFreq += STEP_VALUE;
    }

    // 松开瞬间写入电台并保存到EEPROM
    if (lastDown == LOW && down == HIGH) {
        radio.RX_BK4802(compFreq(b.currentFreq)); // 接收补偿
        saveBandFreq(currentBand, b.currentFreq);
    }
    if (lastUp == LOW && up == HIGH) {
        radio.RX_BK4802(compFreq(b.currentFreq)); // 接收补偿
        saveBandFreq(currentBand, b.currentFreq);
    }

    lastDown = down;
    lastUp = up;
}//按钮控制振荡器频率，输入当前频率值，频率上下限(1.1更新：全部从数组里拿数据)

void handleMenuButton() {
    // 发射时不允许切换频段
    if (digitalRead(PTTBUTTON) == LOW) return;

    static int lastMenuState = HIGH;
    int menuState = digitalRead(MENUBUTTON);
    // 检测松开瞬间（上升沿）
    if (lastMenuState == LOW && menuState == HIGH) {
        currentBand = (currentBand + 1) % NUM_BANDS;// 切换到下一个频段，循环
        // 注意：currentBand 已经改变，现在 bands[currentBand] 是新频段
        // 确保该频段的 currentFreq 已经加载过（在 setup 中已加载，但刚开始可能从未修改过，所以已经是默认值或 EEPROM 值）
        // 将电台频率设置为新频段的当前频率
        radio.RX_BK4802(compFreq(bands[currentBand].currentFreq)); // 接收补偿
        // 保存当前频段索引到 EEPROM
        saveCurrentBandIndex(currentBand);
    }
    lastMenuState = menuState;//复位
}//处理菜单按键控制，按一次改变当前频段（1.1新增）

void saveBandFreq(int bandIdx, uint32_t freq) {
    int addr = bandIdx * 4;    //每个频率值占用4个地址，从0开始
    EEPROM.put(addr, freq);    //由当前频段确定存储地址，并存入EEPROM
}// 保存某个频段的当前频率到 EEPROM（1.1新增）

uint32_t loadBandFreq(int bandIdx, uint32_t defaultFreq) {
    uint32_t freq;
    int addr = bandIdx * 4;//查地址
    EEPROM.get(addr, freq);//读取当前存储的频率值
    if (freq < bands[bandIdx].freqMin || freq > bands[bandIdx].freqMax) {
        // 检查频率是否在合理范围内（根据频段上下限）
        return defaultFreq;
    }
    return freq;
}// 从 EEPROM 加载某个频段的频率，如果无效则返回默认值（1.1新增）

void saveCurrentBandIndex(int idx) {
    int addr = NUM_BANDS * 4;//5*4=20，频率存了 0~19 地址（20 字节），那么频段索引号就存在地址 20
    EEPROM.update(addr, idx);// update 只在值不同时写入，延长寿命
}// 保存当前选中的频段索引（1.1新增）

int loadCurrentBandIndex() {
    int addr = NUM_BANDS * 4;//地址20存频段索引号
    int idx = EEPROM.read(addr);
    if (idx < 0 || idx >= NUM_BANDS) return 0;//默认索引
    return idx;
}// 加载上次保存的频段索引，如果无效则返回 0（1.1新增）

void readFromEEPROM() {// 加载所有频段的当前频率（从 EEPROM 或默认值）
    for (int i = 0; i < NUM_BANDS; i++) {
        bands[i].currentFreq = loadBandFreq(i, bands[i].defaultFreq);
    }//某频段的当前频率是从loadBandFreq里加载的当前频率，无效则用默认频率
     // 加载上次关机时的频段索引
    currentBand = loadCurrentBandIndex();
    // 设置电台频率为当前频段的当前频率
    radio.RX_BK4802(compFreq(bands[currentBand].currentFreq)); // 接收补偿
}//初始化时加载EEPROM（1.1新增）

// ==================== 第五部分：显示函数 ====================
bool init_screen() {
    // 先探测 I2C 总线上有没有屏幕
    Wire.beginTransmission(0x3C);
    if (Wire.endTransmission() != 0) {
        return false;   // 屏幕不存在，直接返回失败
    }
    // 能走到这里说明屏幕存在，再调 begin()
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        return false;   // 这里只会因为 RAM 不足失败,不会检测屏幕通讯能不能成功
    }//妈的藏这么几把深，在这里等着我呢，曹丕岳父不讲话___甄姬爸无语

    display.setTextColor(WHITE);
    display.clearDisplay();
    return true;
}

void displayInitCaption() {
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.print(F("BK4802"));//BK4802，首行顶格
    display.setCursor(3, 19);
    display.setTextSize(2);
    display.print("->");
    display.drawBitmap(35, 18, shu, 16, 16, WHITE);
    display.drawBitmap(51, 18, zi, 16, 16, WHITE);
    display.drawBitmap(67, 18, dian, 16, 16, WHITE);
    display.drawBitmap(83, 18, tai, 16, 16, WHITE);
    display.drawBitmap(98, 0, phone, 32, 40, WHITE);//对讲机图标
    //中文：数字电台
    display.setCursor(8, 38);
    display.setTextSize(2);
    display.print(F(SYSTEM_VERSION));//版本号
    display.setCursor(67, 55);
    display.setTextSize(1);
    display.print(F("HF VHF UHF"));//小标识
    display.display();
    delay(3000);
}// 显示初始化信息

void displayRadioError(void){
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.print(F("BK4802"));//BK4802，首行顶格
    display.drawBitmap(10, 17, wu, 16, 16, WHITE);
    display.drawBitmap(27, 17, xian, 16, 16, WHITE);
    display.drawBitmap(43, 17, dian, 16, 16, WHITE);
    display.drawBitmap(59, 17, xin, 16, 16, WHITE);
    display.drawBitmap(75, 17, pian, 16, 16, WHITE);
    display.drawBitmap(91, 17, gu, 16, 16, WHITE);
    display.drawBitmap(106, 17, zhang, 16, 16, WHITE);
    display.drawBitmap(5, 43, chip_error_icon, 16, 16, WHITE);
    display.setCursor(23, 43);
    display.setTextSize(1);
    display.print(F("RegistersInit()"));
    display.setCursor(23, 52);
    display.print(F("== false"));
    display.display();
}//显示BK4802芯片初始化错误信息

void displayProgressSNRBar(int SNRValue) {
    display.setTextSize(1);
    display.setCursor(26, 0);
    display.print(F("SNR"));//信噪比小标识
    int progressValue = map(SNRValue, 0, 63, 0, 100);//信噪比范围是0-63
    //复用前序代码
    display.drawRect(PROGRESS_SNR_BAR_X, PROGRESS_SNR_BAR_Y, PROGRESS_SNR_BAR_WIDTH, PROGRESS_SNR_BAR_HEIGHT, SSD1306_WHITE);
    int fillWidth = (progressValue * PROGRESS_SNR_BAR_WIDTH) / 100;
    display.fillRect(PROGRESS_SNR_BAR_X, PROGRESS_SNR_BAR_Y, fillWidth, PROGRESS_SNR_BAR_HEIGHT, SSD1306_WHITE);
}//显示信噪比进度条

void displayProgressRSSIBar(int RSSIValue) {
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.print(F("RSSI "));//RSSI小标识
    display.println(RSSIValue);//值
    int progressValue = map(RSSIValue, 0, 127, 0, 100);//RSSI范围0-127
    display.drawRect(PROGRESS_RSSI_BAR_X, PROGRESS_RSSI_BAR_Y, PROGRESS_RSSI_BAR_WIDTH, PROGRESS_RSSI_BAR_HEIGHT, SSD1306_WHITE);
    int fillWidth = (progressValue * PROGRESS_RSSI_BAR_WIDTH) / 100;
    display.fillRect(PROGRESS_RSSI_BAR_X, PROGRESS_RSSI_BAR_Y, fillWidth, PROGRESS_RSSI_BAR_HEIGHT, SSD1306_WHITE);
}//显示RSSI进度条

void displayVoltageValue(float voltage) {
    display.setCursor(0, 55);
    display.print(voltage);
    display.println(F("V"));
}//显示电压

void displayPwrValue(int PwrLevel) {
    display.setCursor(35, 55);
    display.print(F("PWR:"));
    display.println(PwrLevel);
}//显示功率值

void displaySQLState(bool isSQL) {
    if (isSQL) {
        display.setCursor(70, 55);
        display.print(F("SQ"));
    }
}//显示静噪状态

void displayVolValue(int volume) {
    display.setCursor(90, 55);
    display.print(F("VOL:"));
    display.println(volume);
}//显示音量值

void displayPTT(bool isPTT) {
    display.setCursor(0, 0);
    display.setTextSize(1);
    if (!isPTT) display.print(F("TX"));//按下发射，反相
    else display.print(F("RX"));
}//显示发射状态

void displayBandInfo() {
    // 显示当前频段标签（如 F1）
    display.setCursor(0, 16);
    display.setTextSize(1);
    display.print(bands[currentBand].label);
    // 显示类型（HF/VHF/UHF）
    display.setCursor(0, 26);
    display.print(bands[currentBand].typeLabel);
}//1.1修改原函数

void displayFreqValue(uint32_t freq_khz) {
    display.setCursor(23, 17);
    display.setTextSize(2);
    display.print(freq_khz / 1000.0, 3);//转换成兆赫兹显示
    display.setCursor(110, 24);
    display.setTextSize(1);
    display.print(F("MHz"));
}//频率值及显示

// ==================== 第六部分：主程序 ====================
void setup() {
    //Serial.begin(9600);//释放RAM
    pinMode(UPBUTTON, INPUT_PULLUP);
    pinMode(DOWNBUTTON, INPUT_PULLUP);
    pinMode(PTTBUTTON, INPUT_PULLUP);
    pinMode(MENUBUTTON, INPUT_PULLUP);
    pinMode(A0, INPUT_PULLUP);
    //设置按钮
    Wire.begin();                   // 初始化 I2C 总线
    delay(50);                      // 等待加载

    bool radio_available = radio.RegistersInit();
    // 初始化芯片寄存器，如果初始化失败返回值假的，那就显示错误信息（1.3）
    screen_available = init_screen();//屏幕初始化

    if (!radio_available) radioErrorLoop();//戳烧咯，电台用不了咯
    //如果屏幕还在，就显示错误信息，如果屏幕也没了，那就寄

    //初始化成功之后设置4802基础参数
    readFromEEPROM();               // 加载所有频段频率和当前频段
    //使用最新的4802库文件，这些可以启用
    radio.BK4802PwrSet(power);      // 功率设置
    radio.BK4802VolSet(volume);     // 音量调节启用
    radio.BK4802EasySQLSet(SQLState);
    radio.BK4802AutoTailToneSet(0);

    if (screen_available){
        displayInitCaption();       //初始化信息显示
        display.clearDisplay();     //再次清屏，准备进入正常显示流程
    }
    

    wdt_enable(WDTO_2S);            //所有系统全部启动启动启动，还有这个，乐迪~~
}

void loop() {
    wdt_reset();//喂狗
    PTTPressed = digitalRead(PTTBUTTON);
    PTTcontrol(PTTPressed);
    //只有PTT状态是全局变量，直接受按钮控制，该状态机用于控制PTT和显示收发状态
    //其余均采用参数传递
    if (screen_available){
    //这两个放进来，如果屏幕都看不了了，你改频率改频段那不是瞎戟把扯淡吗
    handleFreqChange();
    handleMenuButton();
    //显示程序（从左上角到右下角依次刷新）
    displayPTT(PTTPressed);//PTT收发状态
    displayProgressSNRBar(radio.BK4802SNRGet());//显示信噪比
    displayBandInfo();// 显示当前频段信息
    displayFreqValue(bands[currentBand].currentFreq);//显示频率值
    displayProgressRSSIBar(radio.BK4802RSSIGet());//显示RSSI
    displayVoltageValue(voltageRead());//显示电压
    displayPwrValue(power);//显示发射功率
    displaySQLState(SQLState);//显示静噪状态
    displayVolValue(volume);//显示音量
    //循环绘制，清屏
    display.display();
    display.clearDisplay();//换汤不换药这一块，实在是太好用了
    }
}
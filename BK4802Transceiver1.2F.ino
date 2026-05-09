/*1.2外贸版：仅支持基础功能：U段430，公共409频段，收发无SNR，RSSI进度条显示*/
//更新日期：2026年5月1日  编写人：SG
//此版本为青春版（其实就是阉割版）

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
#define NUM_BANDS   2           // 频段数量（只留2个频率段）


// 汉字点阵
const uint8_t PROGMEM shu[] = {0x08,0x20,0x49,0x20,0x2A,0x20,0x08,0x3E,0xFF,0x44,0x2A,0x44,0x49,0x44,0x88,0xA4,0x10,0x28,0xFE,0x28,0x22,0x10,0x42,0x10,0x64,0x28,0x18,0x28,0x34,0x44,0xC2,0x82};
const uint8_t PROGMEM zi[] = {0x02,0x00,0x01,0x00,0x7F,0xFE,0x40,0x02,0x80,0x04,0x1F,0xE0,0x00,0x40,0x00,0x80,0x01,0x00,0xFF,0xFE,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x05,0x00,0x02,0x00};
const uint8_t PROGMEM dian[] = {0x01,0x00,0x01,0x00,0x01,0x00,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x0A,0x01,0x02,0x01,0x02,0x00,0xFE};
const uint8_t PROGMEM tai[] = {0x02,0x00,0x02,0x00,0x04,0x00,0x08,0x20,0x10,0x10,0x20,0x08,0x7F,0xFC,0x20,0x04,0x00,0x00,0x1F,0xF0,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1F,0xF0,0x10,0x10};
const uint8_t PROGMEM wai[] = {0x10,0x40,0x10,0x40,0x10,0x40,0x10,0x40,0x3E,0x40,0x22,0x60,0x42,0x50,0x42,0x48,0xA4,0x44,0x14,0x44,0x08,0x40,0x08,0x40,0x10,0x40,0x20,0x40,0x40,0x40,0x80,0x40};
const uint8_t PROGMEM mao[] = {0x06,0x00,0x78,0xFC,0x40,0x44,0x48,0x44,0x44,0x44,0x5A,0x94,0x61,0x08,0x00,0x00,0x1F,0xF0,0x10,0x10,0x11,0x10,0x11,0x10,0x11,0x10,0x02,0x60,0x0C,0x18,0x70,0x04};
const uint8_t PROGMEM ban[] = {0x08,0x08,0x48,0x1C,0x49,0xE0,0x49,0x00,0x49,0x00,0x7D,0xFC,0x41,0x44,0x41,0x44,0x79,0x44,0x49,0x28,0x49,0x28,0x49,0x10,0x49,0x10,0x4A,0x28,0x4A,0x44,0x8C,0x82};


// 频段结构体
struct Band {
    uint32_t freqMin;      // 最低频率 (kHz)
    uint32_t freqMax;      // 最高频率 (kHz)
    uint32_t defaultFreq;  // 默认频率 (kHz) – 首次使用或 EEPROM 无效时使用
    uint32_t currentFreq;  // 当前频率 (kHz) – 运行时修改，并保存到 EEPROM
    const char* label;     // 显示标签 "F1", "F2"...
    const char* typeLabel; // 波段类型 "HF", "VHF", "UHF"（可选）
};

// 频段数组
Band bands[NUM_BANDS] = {
    { 409000, 410000, 409700, 409700, "F1", "UHF" }, // 公众段
    { 430000, 440000, 438500, 438500, "F2", "UHF" }  // 业余U段
};

uint8_t currentBand = 0;   // 当前频段索引
uint8_t power = 7;         //发射功率
uint8_t volume = 50;       //音量
bool SQLState = true;      //静噪状态
bool PTTPressed = false;   //PTT状态（是否被按下）

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
void init_screen(void);//显示屏的初始化设置
void displayInitCaption(void);//显示开屏信息
void displayProgressRSSIBar(int RSSIValue);//显示RSSI进度条
void displayVoltageValue(float voltage);//显示电压
void displayPwrValue(int PwrLevel);//显示发射功率
void displaySQLState(bool isSQL);//显示静噪状态
void displayPTT(bool isPTT);//显示PTT状态
void displayBandInfo();//显示频段信息
void displayFreqValue(uint32_t freq_khz);//显示频率值

// ==================== 第四部分：功能实现 ====================
// U段频率补偿 (修复1.2版本10kHz偏差)
uint32_t compFreq(uint32_t freq) {
        return freq + 10;   // 补偿10kHz，使实际收发频率与显示一致
}

float voltageRead() {
    static float voltage = 0;//依旧静态变量，避免重蹈覆辙
    static unsigned long lastVoltRead = 0;
    if (millis() - lastVoltRead > 1000) {
        voltage = analogRead(A0) * (5.0 / 1023.0);
        lastVoltRead = millis();
    }
    return voltage;
}// 电压读取(返回float)

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
void init_screen() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        //Serial.println(F("SSD1306 allocation failed"));去掉节省RAM
        for (;;);
    }
    display.setTextColor(WHITE);
    display.clearDisplay();
}//显示屏的初始化设置

void displayInitCaption() {
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.print(F("BK4802Lite"));//BK4802，首行顶格
    display.drawBitmap(9, 18, shu, 16, 16, WHITE);
    display.drawBitmap(25, 18, zi, 16, 16, WHITE);
    display.drawBitmap(41, 18, dian, 16, 16, WHITE);
    display.drawBitmap(57, 18, tai, 16, 16, WHITE);
    display.drawBitmap(73, 18, wai, 16, 16, WHITE);
    display.drawBitmap(89, 18, mao, 16, 16, WHITE);
    display.drawBitmap(105, 18, ban, 16, 16, WHITE);
    //display.drawBitmap(98, 0, phone, 32, 40, WHITE);取消对讲机图标
    //中文：数字电台外贸版
    display.setCursor(8, 38);
    display.setTextSize(2);
    display.print(F("V1.2F"));//版本号
    display.setCursor(75, 55);
    display.setTextSize(1);
    display.print(F("ONLY UHF"));//小标识
    display.display();
    delay(3000);
}// 显示初始化信息（外贸版，删去开屏显示电台图标）

void displayProgressRSSIBar(int RSSIValue) {
    display.setTextSize(1);
    display.setCursor(85, 0);
    display.print(F("RSSI "));//RSSI小标识
    display.println(RSSIValue);//值
}//显示RSSI值（无进度条）

void displayVoltageValue(float voltage) {
    display.setCursor(0, 55);
    display.print(voltage);
    display.println(F("V"));
}//显示电压

void displayPwrValue(int PwrLevel) {
    display.setCursor(45, 55);
    display.print(F("PWR:MAX"));
}//显示功率值（显示最大而不是实际数值，让用户以为很屌）

void displaySQLState(bool isSQL) {
    if (isSQL) {
        display.setCursor(115, 55);
        display.print(F("SQ"));
    }
}//显示静噪状态（1.2F微调位置）

void displayPTT(bool isPTT) {
    display.setCursor(0, 0);
    display.setTextSize(1);
    if (!isPTT) display.print(F("TX"));//按下发射，反相
    else display.print(F("RX"));
}//显示发射状态

void displayBandInfo() {
    // 显示当前频段标签（如 F1）
    display.setCursor(25, 0);
    display.setTextSize(1);
    display.print(bands[currentBand].label);
    // 显示类型（HF/VHF/UHF）
    display.setCursor(50, 0);
    display.print(bands[currentBand].typeLabel);
}//1.2F修改位置至顶格显示

void displayFreqValue(uint32_t freq_khz) {
    display.setCursor(10, 22);
    display.setTextSize(2);
    display.print(freq_khz / 1000.0, 3);//转换成兆赫兹显示
    display.setCursor(110, 29);
    display.setTextSize(1);
    display.print(F("MHz"));
}//频率值及显示（1.2F微调位置）

// ==================== 第六部分：主程序 ====================
void setup() {
    //Serial.begin(9600);释放RAM
    pinMode(UPBUTTON, INPUT_PULLUP);
    pinMode(DOWNBUTTON, INPUT_PULLUP);
    pinMode(PTTBUTTON, INPUT_PULLUP);
    pinMode(MENUBUTTON, INPUT_PULLUP);
    pinMode(A0, INPUT_PULLUP);
    //设置按钮
    Wire.begin();                   // 初始化 I2C 总线
    delay(50);                      // 等待加载
    radio.RegistersInit();          // 初始化芯片寄存器
    readFromEEPROM();               // 加载所有频段频率和当前频段
    radio.BK4802PwrSet(power);      // 功率设置
    // radio.BK4802VolSet(volume);  // 音量调节后续启用
    // radio.BK4802EasySQLSet(SQLState);
    // radio.BK4802AutoTailToneSet(0);

    init_screen();                  //屏幕初始化
    displayInitCaption();           //初始化信息显示
    display.clearDisplay();         //再次清屏，准备进入正常显示流程

    wdt_enable(WDTO_2S);            //所有系统全部启动启动启动，还有这个，乐迪~~
}

void loop() {
    wdt_reset();//喂狗
    PTTPressed = digitalRead(PTTBUTTON);
    //只有PTT状态是全局变量，直接受按钮控制，该状态机用于控制PTT和显示收发状态
    //其余均采用参数传递
    handleFreqChange();
    handleMenuButton();
    PTTcontrol(PTTPressed);
    ////显示程序（从左上角到右下角依次刷新）
    displayPTT(PTTPressed);//PTT收发状态
    displayBandInfo();// 显示当前频段信息
    displayFreqValue(bands[currentBand].currentFreq);//显示频率值
    displayProgressRSSIBar(radio.BK4802RSSIGet());//显示RSSI
    displayVoltageValue(voltageRead());//显示电压
    displayPwrValue(power);//显示发射功率
    displaySQLState(SQLState);//显示静噪状态
    //循环绘制，清屏
    display.display();
    display.clearDisplay();//换汤不换药这一块，实在是太好用了
}
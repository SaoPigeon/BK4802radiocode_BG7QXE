#include <Wire.h>
#include "BK4802P.h"

BK4802P radio;

void setup() {
    Serial.begin(9600);
    Wire.begin();                 // 初始化 I2C 总线
    delay(50);
    radio.RegistersInit();        // 初始化芯片寄存器
    radio.RX_BK4802(409.0);       // 设置接收频率 409MHz
    radio.BK4802PwrSet(7);        // 最大发射功率（若发射时）
    radio.BK4802VolSet(60);       // 音量 60
    radio.BK4802EasySQLSet(1);    // 开启静噪
    radio.BK4802AutoTailToneSet(0); // 关闭自动消尾音
}

void loop() {
    Serial.print("RSSI: ");
    Serial.println(radio.BK4802RSSIGet());
    Serial.print("Noise: ");
    Serial.println(radio.BK4802NoiseLevelGet());
    delay(5000);
}
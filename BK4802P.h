// BK4802P 驱动程序 自主设计 编写者：SG  归属：CLG队伍
// 此驱动程序是根据 BK4802P 的数据手册编写，因开源作者删库跑路，无奈只得自己写库
// 起编日期：2026年3月24日

#ifndef BK4802P_H
#define BK4802P_H

#include <Arduino.h>

class BK4802P {
public:
    // 构造函数（无需参数，Wire.begin() 需在外部调用）
    BK4802P();

    // 初始化寄存器
    void RegistersInit();

    // 频率设置
    void BK4802FreqSet(double freq_mhz, bool isTX);

    // 收发模式切换
    void BK4802RXModeSet();
    void BK4802TXModeSet();

    // 发射功率设置 (0~7)
    void BK4802PwrSet(int level);

    // 喇叭音量设置 (0~255)
    void BK4802VolSet(uint8_t volume);

    // 获取 RSSI、SNR、带外噪声等状态
    uint8_t BK4802RSSIGet();
    uint8_t BK4802SNRGet();
    uint16_t BK4802NoiseLevelGet();
    uint8_t BK4802ADCRSSIGet();     // SOP16 封装可能固定值
    uint8_t BK4802ADCNslGet();      // SOP16 封装可能固定值

    // 静噪设置
    void BK4802SquelchSet(uint8_t rssi_thresh, uint8_t sq_thresh);
    void BK4802SQNSet(uint8_t code);                // 噪声倍率设置 (0~3)
    void BK4802EasySQLSet(bool isSQL);              // 简易静噪：0-不静噪，1-静噪

    // 自动消尾音设置 (0-关闭,1-开启)
    void BK4802AutoTailToneSet(bool enable);

    // 便捷收发函数
    void RX_BK4802(double freq);
    void TX_BK4802(double freq);

private:
    // 底层 I2C 读写（私有）
    void writeRegister(uint8_t regAddr, uint16_t value);
    uint16_t readRegister(uint8_t regAddr);
};

#endif // BK4802P_H
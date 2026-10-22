// BK4802P 驱动程序 自主设计 编写者：SG  归属：CLG队伍
// 此驱动程序是根据 BK4802P 的数据手册编写，因开源作者删库跑路，无奈只得自己写库
// 起编日期：2026年3月24日
// 修改日期：2026年4月16日 修改内容：频率计算改为kHz整数运算，避免浮点误差（1.1）
// 修改日期：2026年10月22日 修改内容：增加初始化失败返回，让上层知道芯片寄了（1.2）
// 修改日期：2026年10月23日 修改内容：改掉功率音量等函数在收发切换时候寄存器被覆盖掉的大bug，功能函数可以自由使用
#ifndef BK4802P_H
#define BK4802P_H

#include <Arduino.h>

class BK4802P {
public:
    // 构造函数（无需参数，Wire.begin() 需在外部调用）
    BK4802P();

    // 初始化寄存器
    bool RegistersInit();

    // 频率设置
    void BK4802FreqSet(uint32_t freq_khz, bool isTX);

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
    void RX_BK4802(uint32_t freq);
    void TX_BK4802(uint32_t freq);

private:

    // 用户配置结构体（1.3新增）
    struct UserConfig {
        uint8_t power;      // 发射功率 0-7
        uint8_t volume;     // 音量 0-255（建议不超过200）
        bool    easySQLState;    // 静噪开关：true=开启，false=关闭
        bool    tailTone;   // 尾音消除：true=开启，false=关闭
    } _cfg;//cfg是configuration的缩写，意思是配置（四级没过是这样的）

    // 底层 I2C 读写（私有）
    bool writeRegister(uint8_t regAddr, uint16_t value);
    uint16_t readRegister(uint8_t regAddr);
};

#endif // BK4802P_H
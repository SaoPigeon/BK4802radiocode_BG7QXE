//BK4802P 驱动程序 自主设计 编写者：SG  归属：CLG队伍
//此驱动程序是我根据BK4802P的数据手册编写，因开源作者删库跑路，无奈只得自己写库
//起编日期：2026年3月24日

#include <Wire.h>    //IIC库
#define BK4802P_ADDR 0x48  //芯片通讯地址
#define XTAL_FREQ_MHZ 21.25//晶振频率
#define IF_FREQ_MHZ 0.137  //中频频率


double Freq = 409.0;//测试用，后续编库时可删除


//BK4802基础寄存器读写函数（最底层最重要！万万不可删掉）

// 向BK4802指定寄存器写入16位数值（寄存器地址，数值）
void writeRegister(uint8_t regAddr, uint16_t value) {
    Wire.beginTransmission(BK4802P_ADDR);   //开启IIC通讯
    Wire.write(regAddr);                    //定位寄存器位址
    Wire.write(highByte(value));            //写入高字节
    Wire.write(lowByte(value));             //写入低字节
    Wire.endTransmission();                 //结束IIC通讯
    delay(1);                          // I2C 写入后短暂延时
}

//  从BK4802指定寄存器读回16位数值（具体数值）
uint16_t readRegister(uint8_t regAddr) {
    Wire.beginTransmission(BK4802P_ADDR);   //开启IIC通讯
    Wire.write(regAddr);                    //定位寄存器位址
    Wire.endTransmission(false);            //false 表示不发送 STOP，保持总线，此乃BK4802必须
    Wire.requestFrom(BK4802P_ADDR, 2);      //请求读取2个字节，对应高8位，低8位字节
    
    uint8_t high = Wire.read();    // 先读的是高8位
    uint8_t low  = Wire.read();    // 再读低8位
    return (high << 8) | low;      // 组合成16位并返回
}


//BK4802驱动函数（初始化，频率设置，收发模式设置，发射功率设置，喇叭音量设置,获取RSSI，SNR，带外噪声值，静噪设置，自动消尾音）



//初始化寄存器
void RegistersInit() {
 writeRegister(23, 0xACD0);//第一步初始化，先写23号寄存器！
 //这里原参考代码是A8D0，我已修改寄存器Reg23<10>为1，收发完全由寄存器控制
 // 初始化寄存器数组（参照参考代码并基于数据手册修改，后续可改）
 const uint16_t initreg[] = {
    0x0300,    //收发模式寄存器，此处为接收
    0x8E04,    //晶振，采样ADC寄存器
    0xF140,    //收发音频设置寄存器
    0xED00,    //接收中频增益（B15~13）
    0x17E0,    //功放功率，ASK开关（关），低电流模式（关）
    0xE0E0,    //收发模式，PLL工作模式，射频开关接收通路
    0x8543,    //PLL设置
    0x0700,    //PLL设置
    0xA066,    //PLL频率校准
    0xFFFF,    //PLL设置
    0xFFE0,    //发射音频数字滤波器增益，发射音量控制，带内信号能量设置
    0x07A0,    //发射限幅设置寄存器
    0x9E3C,    //发射音频AGC使能（已开启）
    0x1F00,    //TX Audio AGC设置寄存器
    0xD1D1,    //开关喇叭延时设置
    0x200F,    //CIC滤波器增益，FM解调输出幅度，自动消尾音设置，接收音量调整
    0x01FF,    //自动频率校正（关）
    0xE000,    //AFC阈值设置寄存器
    0x0C00     //决定静噪，高8位有时候静噪会开启，设置为0c00能彻底关闭
 };
    //初始化批量设置寄存器，后续不再更改，或由后续专门函数操作内部寄存器位（精准开盒）
    // 批量写入寄存器 4~22
    for (size_t i = 0; i < sizeof(initreg) / sizeof(initreg[0]); i++) {
        writeRegister(4 + i, initreg[i]);
    }
}

// 设置频率函数//BK4802支持的频率段有：24-32；35-46；43-57；128-170；384-512（MHz）
void BK4802FreqSet(double freq_mhz, bool isTX){
// 1. 根据频段确定 Ndiv 和 div_code
  uint8_t ndiv, div_code;
  if (freq_mhz >= 384 && freq_mhz <= 512) {
    ndiv = 4;      // Band1
    div_code = 0;
  } else if (freq_mhz >= 128 && freq_mhz <= 170) {
    ndiv = 12;     // Band2
    div_code = 1;
  } else if (freq_mhz >= 43 && freq_mhz <= 57) {
    ndiv = 36;     // Band3
    div_code = 4;
  } else if (freq_mhz >= 35 && freq_mhz <= 46) {
    ndiv = 44;     // Band4（注意：这一段有问题，因为35-46有一段和43-57交集）
    div_code = 5;
  } else if (freq_mhz >= 24 && freq_mhz <= 32) {
    ndiv = 64;     // Band5
    div_code = 6;
  } else {
    // 超出范围，不做处理
    Serial.println("频率超出4802的支持范围");
    return;
  }

  // 2. 计算 Frac-N
  double f_effective = freq_mhz;
  if (!isTX) {
    // 接收模式需减去中频（因为 f_xtal/21.25 = 1）
    f_effective = freq_mhz - IF_FREQ_MHZ;
  }
  double frac_n = (f_effective * ndiv * (1UL << 24)) / XTAL_FREQ_MHZ;
  uint32_t frac_int = (uint32_t)(frac_n + 0.5);

  // 3. 拆分寄存器0、1
  uint16_t reg0 = (frac_int >> 16) & 0xFFFF;
  uint16_t reg1 = frac_int & 0xFFFF;

  // 4. 寄存器2：高3位 = div_code，低5位 = 0
  uint16_t reg2 = (div_code << 13) & 0xE000;

  // 5. 写入寄存器
  writeRegister(0, reg0);
  writeRegister(1, reg1);
  writeRegister(2, reg2);

}

//接收模式设置函数
void BK4802RXModeSet(){
    writeRegister(4, 0x0300);  //收发模式寄存器，此处为接收
    writeRegister(5, 0x8E04);  //晶振，采样ADC寄存器
    writeRegister(8, 0x1700);  //功放功率（Reg8<5-7>已置零），ASK开关（关），低电流模式（关）
    writeRegister(15, 0x07A0); //发射限幅设置寄存器
    writeRegister(18, 0xD1D1); //开关喇叭延时设置
    writeRegister(22, 0x0C00); //静噪
}

//发射模式设置函数
void BK4802TXModeSet(){
    writeRegister(4, 0x7C00);  //收发模式寄存器，此处为发射
    writeRegister(5, 0x0C04);  //晶振，采样ADC寄存器
    writeRegister(8, 0x3FE0);  //功放功率
    writeRegister(15, 0x061F); //发射限幅设置寄存器
    writeRegister(18, 0xD1C1); //开关喇叭延时设置
    writeRegister(22, 0x0C00); //静噪
}

//发射功率设置函数 功率值0-7（对应寄存器Reg8<5-7>）
void BK4802PwrSet(int level){
    if (level > 7) level = 7;                  //超过最大值就设为7
    uint16_t reg8 = readRegister(8);           //读取当前值，确保其他寄存器位不被覆盖
    reg8 = (reg8 & ~(0x07 << 5)) | (level << 5); //用~(0x07 << 5)清零5-7，再写入新值
    writeRegister(8, reg8);                    //写入寄存器Reg8
}

//喇叭音量设置函数 音量值0-255（对应寄存器Reg32<0-7>）（实测超过200会降低接收灵敏度？？？不过我还不确定）
void BK4802VolSet(uint8_t volume) {
    uint16_t reg32 = readRegister(32);       // 读取当前值
    reg32 |= (1 << 8);                       // 启用数字音量（bit8=1）
    reg32 = (reg32 & 0xFF00) | volume;       // 保留高8位，更新低8位（音量阈值）
    writeRegister(32, reg32);                // 写回
    //备注：麦克风音频增益不需刻意设置，保持默认值即可
    /*（原话）麦克风放大器的增益由寄存器 REG16<5:0>控制。BK4802P 发射音频
     通路具有自动增益控制功能，因此该寄存器无需手动设置，系统会自动选
     择最佳增益。 */
}

//获取RSSI值（0-127）（对应寄存器Reg24<0-6>）
uint8_t BK4802RSSIGet(){
    uint16_t reg24 = readRegister(24);
    return reg24 & 0x7F;    //RSSI返回值是24寄存器的低7位
}

//获取信噪比（SNR）值（0-63）（对应寄存器Reg24<8-13>）
uint8_t BK4802SNRGet() {
    uint16_t reg24 = readRegister(24);  // 读取整个16位寄存器
    return (reg24 >> 8) & 0x3F;         // 右移8位后取低6位得到SNR
}

//获取带外噪声电平值（0~8191）（对应寄存器Reg26<0-12>）
uint16_t BK4802NoiseLevelGet() {
    uint16_t reg26 = readRegister(26);
    return reg26 & 0x1FFF;  //带外噪声电平是26寄存器的低13位
}

//获取ADC的rssi值（对应寄存器Reg31<0-7>）因sop16封装原因此值可能为固定，实际使用时可注释掉这个功能
uint8_t BK4802ADCRSSIGet(){
    uint16_t reg31 = readRegister(31);
    return reg31 & 0xFF;          // 只取低8位
}

//获取ADC的带外噪声电平值（对应寄存器Reg30<0-7>）因sop16封装原因此值可能为固定，实际使用时可注释掉这个功能
uint8_t BK4802ADCNslGet(){
    uint16_t reg30 = readRegister(30);
    return reg30 & 0xFF;
}

//静噪设置，对应寄存器（Reg22,23）
 /*rssi_thresh  信号强度阈值 (0~255)，RSSI(43) > 此值才可能开喇叭
  sq_thresh    噪声阈值 (0~255)，噪声电平(171)*SQN < 此值才可能开喇叭
  注意：SOP16的ADC噪声电平171不可修改的情况下，SQN最低值是2，那么最低值就是342，
  也就是说，就算阈值设最大了这个条件也是恒满足的，那么可以推断静噪只与RSSI有关？2026年3月27日注记*/
  /*不对，实测发现噪声阈值设为0了它也不响至少要设为2才能响，上面可以推翻了，那和ADC有什么关系，是不是
  数据手册写错了，但是实测是否表明和直接读取reg26的噪声电平有关？2026年3月27日注记*/
  /*不应该，SQN设为1了，噪声电平设为0都直接开喇叭了，那么说明实际上和rssi值没什么关系，因为我都设到
  最大了它还是响，那么结局应该是这样：BK4802的自动静噪与RSSI值无关，与SQN值强相关，设为0则静噪开
  非0则关闭静噪，也许是因为无信号状态下RSSI和噪声阈值都偏高的原因？但无论怎么讲（老蒋名言）这两条都可以先
  废掉，我写一个简易静噪设置函数，0为关，1为开启。80万对60万，优势在我*/
void BK4802SquelchSet(uint8_t rssi_thresh, uint8_t sq_thresh) {
    // 1. 设置 RSSI 阈值（寄存器22低8位）
    uint16_t reg22 = readRegister(22);
    reg22 = (reg22 & 0xFF00) | rssi_thresh;
    writeRegister(22, reg22);

    // 2. 设置 SQ 阈值（寄存器23低8位）
    uint16_t reg23 = readRegister(23);
    reg23 = (reg23 & 0xFF00) | sq_thresh;
    writeRegister(23, reg23);
}

//噪声阈值倍率设置，对应乘数：0->2, 1->4, 2->8, 3->16
void BK4802SQNSet(uint8_t code) {
    if (code > 3) code = 3;                // 依旧限制范围
    uint16_t reg22 = readRegister(22);     // 读取当前值
    reg22 &= ~(0x03 << 10);                // 清零 bit11~bit10
    reg22 |= (code << 10);                 // 写入新的编码
    writeRegister(22, reg22);              // 写回芯片
}

//BK4802简易静噪设置函数，0->不静噪，1->静噪
void BK4802EasySQLSet(bool isSQL){
    if(isSQL) writeRegister(22, 0x0340); //静噪
    else writeRegister(22, 0x0c00);     //不静噪  
}

//自动消尾音功能设置（对应寄存器Reg19<10>）0->关闭，1->开启
void BK4802AutoTailToneSet(bool enable) {
    uint16_t reg19 = readRegister(19);      // 读取当前寄存器19的值
    if (enable) reg19 &= ~(1 << 10);        // 开启：bit10 = 0
    else  reg19 |= (1 << 10);               // 关闭：bit10 = 1
    writeRegister(19, reg19);               // 写回芯片
}


//后续可以增加更多功能函数（开放接口给应用层调用）


//启用接收（调用时用这个）
void RX_BK4802(double freq){
    BK4802FreqSet(freq, false);
    BK4802RXModeSet(); 
}

//启用发射（调用时用这个）
void TX_BK4802(double freq){
    BK4802FreqSet(freq, true);
    BK4802TXModeSet(); 
}



//测试用
void setup(){
    Serial.begin(9600);
    Wire.begin();  // 加入 I2C 总线作为主机
    delay(50);     // 等待 BK4802P 稳定
    RegistersInit();
    RX_BK4802(Freq);
    //BK4802PwrSet(7);
    //BK4802VolSet(60);
    //Serial.println(readRegister(22));
    //Serial.println(readRegister(23));
    //BK4802SQNSet(3);
    //BK4802SquelchSet(0, 0);
    //BK4802EasySQLSet(0);
    //BK4802AutoTailToneSet(0);

}
void loop(){
   
    
    delay(5000);
   
    Serial.println(BK4802RSSIGet());
    Serial.println(BK4802NoiseLevelGet());
    //Serial.println(BK4802ADCRSSIGet());
    //Serial.println(BK4802ADCNslGet());


}




#include <hardware/pwm.h>
#include <U8g2lib.h>
#include "Button.hpp"
#include "SmoothAnalogRead.hpp"
#include "EzOscilloscope.hpp"

// #define PROTO

// GIPO割り当て
#define SW0 15
#define SW1 14
#define POT0 A0
#define POT1 A1
#define POT2 A2
#define PWM_POT0 10
#define PWM_POT1 11
#define PWM_POT2 12
#define S0 1
#define S1 2
#define S2 3
#define T0 13
#define ROM1 6
#define ROM2 7
#define CV A3

// 操作関係
static Button sw0;
static Button sw1;
#define POTS_MAX 3
#define POTS_BIT 12
#define POTS_MAX_VALUE 4095
static SmoothAnalogRead pots[POTS_MAX];
static uint potSlices[POTS_MAX] = {0};
static uint potChs[POTS_MAX] = {PWM_CHAN_A, PWM_CHAN_B, PWM_CHAN_A};
static uint pwmPotGpios[POTS_MAX] = {PWM_POT0, PWM_POT1, PWM_POT2};

static SmoothAnalogRead cv;
static EzOscilloscope ezOscillo;

// 表示関係
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/U8X8_PIN_NONE);
static int8_t presetIndex = 0;
static uint16_t potValues[POTS_MAX] = {0};

#define PRESET_SELECT_MAX 8
#define PRESET_MAP_MAX 3 // INTERNAL PRESETS + (EEPROM x 2)
// presetIndexの最大値はPRESET_SELECT_MAX * PRESET_MAP_MAXとなる
#define PRESET_TOTAL (PRESET_SELECT_MAX * PRESET_MAP_MAX)

// プリセット名やパラメタ名などは、EEPROMには入ってないしEEPROMを直接読まないのでここで都度定義する必要がある
const static char *effectNames[PRESET_TOTAL][4] = {
    // INTERNAL PRESETS
    {"ChorusReverb", "Rev Mix ", "ChorRate", "Chor Mix"},
    {"FlangeReverb", "Rev Mix ", "FlngRate", "Flag Mix"},
    {"Tremolo-rev ", "Rev Mix ", "TremRate", "Trem Mix"},
    {"Pitch shift ", "PtchSemi", "--------", "--------"},
    {"Pitch-echo  ", "PtchShft", "EchoDlay", "EchoMix "},
    {"Test        ", "--------", "--------", "--------"},
    {"Reverb 1    ", "Rev Time", "HF  Filt", "LF  Filt"},
    {"Reverb 2    ", "Rev Time", "HF  Filt", "LF  Filt"},
    // EEPROM A（例）
    {"Echo Reverb ", "Delay   ", "Repeat  ", "Reverb  "}, // Spin Semi  3K_V1_4_ECHO-REV.spn
    {"Rv+Flnge+LP ", "Reverb  ", "Flanger ", "LPF     "}, // Dave Spinkler  dance_ir_fla_l.spn
    {"Rv+Pitch+LP ", "Reverb  ", "Pitch   ", "Filter  "}, // Dave Spinkler  dance_ir_ptz_l.spn
    {"ShimmerRvOct", "Shimmer ", "Time    ", "Damping "}, // Dattorro Mix Reverb  dattorro-shimmer_oct_var-lvl.spn
    {"ShimmerValLv", "Shimmer ", "Time    ", "Damping "}, // Dattorro Mix Reverb  dattorro-shimmer_val-lvl.spn
    {"SnglTapeEcRv", "Time    ", "Feedback", "Damping "}, // ---  dv103-1head-pp-2_1-4xreverb.spn
    {"DualTapeEcRv", "DlayTime", "Feedback", "Damping "}, // ---  dv103-2head-2_1-reverb.spn
    {"RoomReverb  ", "DlayTime", "Damping ", "Feedback"}, // Digital Larry  room-reverb-3-4-5.spn
    // // EEPROM A
    // {"EEPROM A    ", "P1      ", "P2      ", "P3      "}, //
    // {"EEPROM A    ", "P1      ", "P2      ", "P3      "}, //
    // {"EEPROM A    ", "P1      ", "P2      ", "P3      "}, //
    // {"EEPROM A    ", "P1      ", "P2      ", "P3      "}, //
    // {"EEPROM A    ", "P1      ", "P2      ", "P3      "}, //
    // {"EEPROM A    ", "P1      ", "P2      ", "P3      "}, //
    // {"EEPROM A    ", "P1      ", "P2      ", "P3      "}, //
    // {"EEPROM A    ", "P1      ", "P2      ", "P3      "}, //
    // EEPROM B
    {"EEPROM B    ", "P1      ", "P2      ", "P3      "}, //
    {"EEPROM B    ", "P1      ", "P2      ", "P3      "}, //
    {"EEPROM B    ", "P1      ", "P2      ", "P3      "}, //
    {"EEPROM B    ", "P1      ", "P2      ", "P3      "}, //
    {"EEPROM B    ", "P1      ", "P2      ", "P3      "}, //
    {"EEPROM B    ", "P1      ", "P2      ", "P3      "}, //
    {"EEPROM B    ", "P1      ", "P2      ", "P3      "}, //
    {"EEPROM B    ", "P1      ", "P2      ", "P3      "}, //
};

void initOLED()
{
    u8g2.begin();
    u8g2.setContrast(40);
    u8g2.setFontPosTop();
    u8g2.setDrawColor(2);
#ifndef PROTO
    u8g2.setFlipMode(1);
#endif
}

#ifdef PROTO
#define TITLE_ROW 3
#define POTS_ROW 0
#else
#define TITLE_ROW 0
#define POTS_ROW 1
#endif

void dispOLED(byte index, uint16_t values[POTS_MAX])
{
    u8g2.setFont(u8g2_font_7x14B_tf);
    u8g2.clearBuffer();
    static char disp_buf[24] = {0};

    for (byte i = 0; i < POTS_MAX; ++i)
    {
        byte height = 16 * (i + POTS_ROW);

        // プリセット項目名と数値を表示
        byte value = map(values[i], 0, POTS_MAX_VALUE, 0, 127); // 表示する棒グラフの幅範囲も兼ねてる
        sprintf(disp_buf, "P%d: %s %03d", i, (const char *)effectNames[index][i + 1], value);
        u8g2.drawStr(0, height, disp_buf);
        // ラベル背景に棒グラフ的に表示
        u8g2.drawBox(0, height, value, 14);
    }

    // ROM/EEPROPM1/2の表示、プリセット名表示
    static char rom[2] = {0};
    byte mapIndex = index / PRESET_SELECT_MAX;
    sprintf(rom, mapIndex == 0 ? "R" : mapIndex == 1 ? "A"
                                                     : "B");
    sprintf(disp_buf, "%s%d: %s", rom, index % 8, (const char *)effectNames[index][0]);
    u8g2.drawStr(0, 16 * TITLE_ROW, disp_buf);

    u8g2.sendBuffer();
}

void initRomBit()
{
    pinMode(T0, OUTPUT);
    pinMode(ROM1, OUTPUT);
    pinMode(ROM2, OUTPUT);
}

// プリセットの現在値から現在のROM/EEPROM切り替え設定を行う
void setRomBit(byte index)
{
    static byte mapIndexOld = 255;
    byte mapIndex = index / PRESET_SELECT_MAX;
    if (mapIndexOld == mapIndex)
    {
        return;
    }

    mapIndexOld = mapIndex;
    byte t0 = LOW;
    byte rom1 = LOW;
    byte rom2 = LOW;
    switch (mapIndex)
    {
    case 0:
        t0 = LOW;
        rom1 = HIGH;
        rom2 = HIGH;
        break;
    case 1:
        t0 = HIGH;
        rom1 = LOW;
        rom2 = HIGH;
        break;
    case 2:
        t0 = HIGH;
        rom1 = HIGH;
        rom2 = LOW;
        break;
    default:
        t0 = LOW;
        rom1 = HIGH;
        rom2 = HIGH;
        break;
    }

    digitalWrite(T0, t0);
    digitalWrite(ROM1, rom1);
    digitalWrite(ROM2, rom2);
}

void initPresetBit()
{
    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
}

// プリセットの現在値からプリセット設定を行う
void setPresetBit(byte index)
{
    static byte indexOld = 255;
    if (indexOld == index)
    {
        return;
    }

    indexOld = index;
    // presetIndexは8以上入るがretReadで3bitしか読んでないので問題なし
    digitalWrite(S0, bitRead(index, 0));
    digitalWrite(S1, bitRead(index, 1));
    digitalWrite(S2, bitRead(index, 2));
}

void initPWMPotsOut()
{
    for (byte i = 0; i < POTS_MAX; ++i)
    {
        // GP0-GP3のピン機能をPWMに設定
        gpio_set_function(pwmPotGpios[i], GPIO_FUNC_PWM);
        // GP0-GP3のPWMスライスを取得
        potSlices[i] = pwm_gpio_to_slice_num(pwmPotGpios[i]);
        // PWM周期
        // 31.25kHz ビットは設定による
        pwm_set_clkdiv(potSlices[i], 3.90625);
        pwm_set_wrap(potSlices[i], POTS_MAX_VALUE);
        // PWM出力イネーブル
        pwm_set_enabled(potSlices[i], true);
    }
}

template <typename su = uint8_t>
su constrainCyclic(su value, su min, su max)
{
    if (value > max)
        return min;
    if (value < min)
        return max;
    return value;
}

void initController()
{
    // internal regulator output mode -> PWM
    pinMode(23, OUTPUT);
    digitalWrite(23, HIGH);

    // ADC解像度とパルス出力解像度はあわせること
    analogReadResolution(POTS_BIT);
    sw0.init(SW0);
    sw1.init(SW1);
    sw0.setHoldTime(1000);
    sw1.setHoldTime(1000);
    pots[0].init(POT0);
    pots[1].init(POT1);
    pots[2].init(POT2);
    cv.init(CV);
    ezOscillo.init(&u8g2, &cv, POTS_ROW * 16);

    initRomBit();
    setRomBit(presetIndex);

    initPresetBit();
    setPresetBit(presetIndex);

    initPWMPotsOut();
}

static byte dispMode = 0;
void updateController()
{
    // ポット処理更新
    for (byte i = 0; i < POTS_MAX; ++i)
    {
        potValues[i] = pots[i].analogRead();
        // FV-1へポットの値をパルス出力
        pwm_set_chan_level(potSlices[i], potChs[i], potValues[i]);
    }
    
    byte stateSw0 = sw0.getState();
    byte stateSw1 = sw1.getState();
    // ボタン処理：プリセット変更
    if (stateSw0 == 2)
    {
        if (dispMode)
        {
            ezOscillo.incDelay();
        }
        else
        {
            presetIndex = constrainCyclic(presetIndex + 1, 0, PRESET_TOTAL - 1);
            setRomBit(presetIndex);
            setPresetBit(presetIndex);
        }
    }
    else if (stateSw0 == 3)
    {
        dispMode = dispMode ? 0 : 1;
    }
    if (stateSw1 == 2)
    {
        if (dispMode)
        {
            ezOscillo.decDelay();
        }
        else
        {
            presetIndex = constrainCyclic(presetIndex - 1, 0, PRESET_TOTAL - 1);
            setRomBit(presetIndex);
            setPresetBit(presetIndex);
        }
    }
}

// CPU 1は操作系専用
void setup()
{
    Serial.begin(9600);
    initController();
}

void loop()
{
    updateController();
    delay(1);
}

// CPU 2は表示専用
void setup1()
{
    initOLED();
    dispOLED(presetIndex, potValues);
}

void loop1()
{
    // 30fpsで更新
    delay(33);
    if (dispMode)
    {
        ezOscillo.play();
    }
    else
    {
        dispOLED(presetIndex, potValues);
    }
}

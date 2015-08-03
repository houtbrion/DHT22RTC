
// http://www.geocities.jp/zattouka/GarageHouse/micon/Arduino/RTC/RTC.htm
// https://github.com/adafruit/DHT-sensor-library
// https://learn.adafruit.com/dht

/*******************************************************************************
*  Inter.pde - ＲＴＣ－８５６４ＮＢを使ったテストサンプル                      *
*                                                                              *
*  RTCからの１秒毎割り込み信号のタイミングでRTCから時刻を読み取って表示        *
*  定周期タイマー機能も起動後、１０秒間隔で出力し２分後に終了する              *
* ============================================================================ *
*  VERSION DATE        BY                    CHANGE/COMMENT                    *
* ---------------------------------------------------------------------------- *
*  1.00    2012-01-15  きむ茶工房(きむしげ)  Create                            *
*******************************************************************************/

//#define USE_SLEEP
//
#define USE_DHT
#define DHTPIN 5     // what pin we're connected to

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// RTCからarduinoに割込みを上げる端子と割込み番号の指定
// 本当なら，割込み番号を入れたら端子も機種毎の切り替えを効かせたいが，とりあえず固定指定
//
// 手持ちのMega2560では，INT0,1が動作しなかったため，大きい番号の割込みを利用
//#define INT_NUMBER 5
//#define PIN_NUMBER 18
// UNOはINT0,1が動作するため，こちらを利用
#define INT_NUMBER 0
#define PIN_NUMBER 2


//
//　端末が眠る場合の眠りの深さの指定
//
#ifdef USE_SLEEP
//#define STANDBY_MODE SLEEP_MODE_IDLE
//#define STANDBY_MODE SLEEP_MODE_ADC
//#define STANDBY_MODE SLEEP_MODE_PWR_SAVE
//#define STANDBY_MODE SLEEP_MODE_STANDBY
#define STANDBY_MODE SLEEP_MODE_PWR_DOWN
#else
#define STANDBY_MODE 0
#endif /* USE_SLEEP */
//
// 端末が眠る期間の指定
//
#define SLEEP_DURATION 10 //単位の10倍
//#define SLEEP_UNIT 0 // 244.14us単位
//#define SLEEP_UNIT 1 //15.625ms単位
#define SLEEP_UNIT 2 //秒単位
//#define SLEEP_UNIT 3 //分単位


//
// RTC8564用のライブラリを使うためのインクルードのロード
//
#include <Wire.h>
#include <skRTClib.h>

#ifdef USE_SLEEP
#include <avr/sleep.h>
#endif /* USE_SLEEP */


#include <stdio.h>
#include <DHT.h>

/*
 * DHT関係の定義
 */


// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor for normal 16mhz Arduino
//DHT dht(DHTPIN, DHTTYPE);
// NOTE: For working with a faster chip, like an Arduino Due or Teensy, you
// might need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// Example to initialize DHT sensor for Arduino Due:
//DHT dht(DHTPIN, DHTTYPE, 30);
// arduino M0 pro
DHT dht(DHTPIN, DHTTYPE,18);

static FILE uartout;

static int uart_putchar (char c, FILE *stream) {
  if (Serial.write(c) > 0) {
    return 0;
  } else {
    return -1;
  }
}

////////////////////////////////////////////////////////////////////////////////
// RTC(CLKOUT)からの外部割込みで処理される関数
////////////////////////////////////////////////////////////////////////////////
void InterRTC()
{
  skRTC.InterFlag = 1 ;
  Serial.println("Interupt!");
}



/*******************************************************************************
*  電源起動時とリセットの時だけのみ処理される関数(初期化と設定処理)            *
*******************************************************************************/
void setup()
{
  int ans ;
  fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &uartout;
  pinMode( PIN_NUMBER, INPUT);    // 後の項で使用
  Serial.begin(9600) ;                    // シリアル通信の初期化
  ans = skRTC.begin(PIN_NUMBER,INT_NUMBER,InterRTC,12,1,10,2,15,30,0) ;  // 2012/01/10 火 15:30:00 でRTCを初期化する
  if (ans == 0) {
    Serial.println("Successful initialization of the RTC") ;// 初期化成功
  } else {
    Serial.print("Failed initialization of the RTC ans=") ; // 初期化失敗
    Serial.println(ans) ;
    while(1) ;                         // 処理中断
  }
  skRTC.SetTimer(SLEEP_UNIT,SLEEP_DURATION) ;
}
/*******************************************************************************
*  繰り返し実行される処理の関数(メインの処理)                                  *
*******************************************************************************/
void loop()
{
  byte tm[7] ; 
  char buf[24] ;
  goodNight(STANDBY_MODE);// 割込みが発生したか？
  if (skRTC.InterFlag == 1) {  // 割込みが発生したか？
    skRTC.rTime(tm) ;                    // RTCから現在の日付と時刻を読込む
    skRTC.cTime(tm,(byte *)buf) ;        // 日付と時刻を文字列に変換する
    Serial.println(buf) ;              // シリアルモニターに表示
    if ((tm[1]==0x32)&&(tm[0]==0x00)){ // ３２分になったら機能終了
      skRTC.StopTimer() ;             // タイマー機能終了し無効にする
    }
    float h = dht.readHumidity(); //湿度
    float t = dht.readTemperature(false); //摂氏
    float hi = dht.convertFtoC(dht.computeHeatIndex(dht.convertCtoF(t),h)); //体感温度
    if (isnan(t) || isnan(h)) {
      Serial.println("Failed to read from DHT");
    } else {
      Serial.print("Humidity: "); 
      Serial.print(h);
      Serial.print(" %\t");
      Serial.print("Temperature: "); 
      Serial.print(t);
      Serial.println(" *C");
      Serial.print("Heat Index : ");
      Serial.println(hi);
    }
    skRTC.InterFlag = 0 ;                // 割込みフラグをクリアする
  }
}

void goodNight(int i) {
  Serial.println("  Good Night");
  delay(100);
#ifdef USE_SLEEP
  noInterrupts();
  set_sleep_mode(i);
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();
#endif /*USE_SLEEP*/
}




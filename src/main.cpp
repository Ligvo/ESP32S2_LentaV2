#include "ColorMusik.h"
#include "grpalet.h"

#define VERSION             "6.02"                  // Версия прошивки
#define HOSTNAME            "ESP32S2_Lenta"
#define LED_COUNT           120
#define LED_PIN             40                      // К какому пину подключеа панель с WS1812B

#define SWITCH_PIN          12                      // Пин включение РЭЛЕ
#define KEY_PIN_SWMODE      5                       // Подключены кнопки
#define KEY_PIN_AUT_SW_ON   3
#define PIN_BRIGHTNESS      9                      // вход резистора регулировки яркости 
#define TIME_LENTA_OFF      30                      // Время в секундач, через которое отключается гирлянда при отсутствии сигнала

#define NUMBCOLORBLOCKS     5                       // количество Цветных блоков 

const uint8_t NumbLedsInBlock = (LED_COUNT / NUMBCOLORBLOCKS); // Количество LED в блоке
const uint8_t PolBloka = (LED_COUNT / NUMBCOLORBLOCKS) / 2;
const uint8_t PolBlokaAllLed = LED_COUNT / 2;

void CheckPinsTask(void *pvParameters);


// Определяем функции действий
void raduga_spectrum();
void Clasick();
void raduga_fier();
void Wave();
void WaveNew();
void raduga_pixel();

void CheckPins();
void CheckPinsT();
void KeyPress(unsigned long miliSekonds) ;



volatile bool needSendParam = false;              // Глобальный флаг, отправка параметров, чтобы не блокировать UDP прием

TimerHandle_t ChangeModeTimer;                    // таймер смены режима работы
TimerHandle_t CheckParametersTimer;               // Таймер проверки Параметров
//TimerHandle_t CheckPinsTimer;                     // таймер проверки нажатия кнопок

//NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0Ws2812xMethod> leds(LED_COUNT, LED_PIN);
MeopixelArray<NeoGrbFeature, NeoEsp32I2s0Ws2812xMethod> leds(LED_COUNT, LED_PIN);
//SettingsManager settings; //- определе как глобальная переменная


// Создаем Timer
hw_timer_t * timer = NULL;

void IRAM_ATTR onTimer(){
  FrameFps = _FrameFps;
  PackRecived =_PackRecived;
  _FrameFps = 0;
  _PackRecived = 0;
}

void ChangeMode() // Метод Таймера Моргалка
{
    if(WiFi.status() == WL_CONNECTED)
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); 
    else
      digitalWrite(LED_BUILTIN, LOW );
  }


void CheckParam() // Метод Таймера проверки параметров
{
  //if(FrameFps != 0 && DebugInfo) 
   // Serial.printf("Debug : Frame FPS %d\n\r", FrameFps);
   // Serial.printf("⚠️ роверяем параметры 🌟 ❌ 🚒.");
   needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
}

// Функция-обработчик изменений (связующее звено)
void myChangeModeEvent(uint8_t newMode) {

    Serial.printf("🌟  Система переключена в режим: %d\n\r", newMode);
    //Serial.println(newMode);
    
    // Тут можно включить светодиод или обновить дисплей ESP32
}

const uint8_t DebugMode = 5;

// Создаем список действий
ModeEngine::Action myModes[] = { raduga_spectrum,  Wave, Clasick, raduga_fier, raduga_pixel, WaveNew};

ModeEngine engine(myModes, 6);





void setup() {
  
  Serial.begin(921600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SWITCH_PIN, OUTPUT);        // Пин включение РЭЛЕ
    // Кнопки упраления
  pinMode(KEY_PIN_SWMODE, INPUT_PULLUP); 
  pinMode(KEY_PIN_AUT_SW_ON, INPUT_PULLUP); 
  pinMode(PIN_BRIGHTNESS, INPUT);

  digitalWrite(SWITCH_PIN, HIGH);
  vTaskDelay(pdMS_TO_TICKS(2000)); 

  analogReadResolution(12);
  
 
  
  uint8_t brightness, mode;

  unsigned long start = millis();
  while (!Serial && millis() - start < 10000) {
    //delay(10);  // даём время USB CDC и не перегружаем CPU
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }

  Version = VERSION;  


  Serial.println("----------------------------------------------");  
  Serial.println("\n\rBooting");
  Serial.println(ARDUINO_BOARD);
  Serial.printf("ESP32 Chip model = %s Rev %d\n\r", ESP.getChipModel(), ESP.getChipRevision());
	Serial.printf("This chip has %d cores\n\r", ESP.getChipCores());
  Serial.printf("SDK version: %s \n\r", ESP.getSdkVersion());
  Serial.printf("CPU speed: %u MHz\n\r", ESP.getCpuFreqMHz());
  Serial.printf("Flash real size: %u bytes\n\r", ESP.getFlashChipSize());
  Serial.printf("Версия прошивки: %s \n\r", VERSION);
  
  



  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);
  
  
  ChangeModeTimer = xTimerCreate("ChModeTimer", pdMS_TO_TICKS(500), pdTRUE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(ChangeMode));

  CheckParametersTimer = xTimerCreate("CheckParametersTimer", pdMS_TO_TICKS(30000), pdTRUE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(CheckParam));
  
  
  //CheckPinsTimer = xTimerCreate("CheckPinsTimer", pdMS_TO_TICKS(100), pdTRUE, (void*)0,
  //                                  reinterpret_cast<TimerCallbackFunction_t>(CheckPins));

 
  //xTaskCreatePinnedToCore(CheckPinsTask, "CheckPinsTask", 4096, NULL, 1, NULL, 0);

  settings.ReadSettings();


  leds.Begin();
  leds.SetTemperature(TypicalSMD5050Neo);
  leds.ClearTo(0);
  leds.Show();
  leds.ClearTo(BACKGROUND);
  leds.Show();

  // Привязываем обработчик ПЕРЕД использованием
  engine.setOnChangeHandler(myChangeModeEvent);
  engine.setAutoInterval(TimeChangeMmode); // Задаем интервал смены режимов

  //Net::ScanWifiSsid();
  Net::Init(HOSTNAME);          // запускаем wifi и все службы
  
  xTimerStart(ChangeModeTimer, 0);
  xTimerStart(CheckParametersTimer, 0);
  //xTimerStart(CheckPinsTimer, 0);

  // запускаем задачу чтнения кнопок и резисторя яркости
  xTaskCreate(CheckPinsTask, "CheckPinsTask", 4096, NULL, 1, NULL);
  // отлаживаем режим, автопреключение отключается
  engine.setCurrentMode(DebugMode);
}

/// @brief Чтение команд из терминала
void ReadTerminal()
{
 if (Serial.available() > 0) {
    // Читаем до символа новой строки
    String input = Serial.readStringUntil('\n');
    input.trim();           // Удаляем лишние пробелы и \r

    input.toLowerCase(); 

    char cmd[20];             // Буфер для самой команды
    int val1, val2, val3;

   
    if (sscanf(input.c_str(), "%s %d", cmd, &val1) == 2) {
      if (strcmp(cmd, "wifi") == 0) 
      {
        //Serial.printf("Мотор запущен на скорости: %d\n", val1);

          size_t MyWiFicount = settings.networks.size() ;    // Размер массива networks
          for(int s = 0; s < MyWiFicount; s++)
          {
            Serial.printf("SSID: %s, Password  %s \n", settings.networks[s].Ssid.c_str(), settings.networks[s].Password.c_str());
          }

        return;
      }
    }
    else
    {
      size_t MyWiFicount = settings.networks.size() ;    // Размер массива networks
      for(int s = 0; s < MyWiFicount; s++)
        {
          Serial.printf("SSID: %s, Password  %s \n", settings.networks[s].Ssid.c_str(), settings.networks[s].Password.c_str());
          
        }
        return;
    }

    //  Команда с тремя числами: "rgb 255 128 0"
    if (sscanf(input.c_str(), "%s %d", cmd, &val1) == 2) {
      if (strcmp(cmd, "br") == 0) {
       // Serial.printf("Цвет: R=%d, G=%d, B=%d\n", val1, val2, val3);
        
        Serial.printf("Яркость: %d -> %d\n", chDt.GetMaxBrightness(), val1);
        chDt.SetMaxBrightness(val1) ;

        return;
      }
    }

    //  Простая команда без параметров: "status"
    if (input == "status") {
      Serial.println("Система работает стабильно.");
      Serial.printf("Яркость: %d\n", chDt.GetMaxBrightness());
      
    } else {
      Serial.println(input);
      Serial.println("Ошибка: Команда не распознана.");
    }
  }
}


void loop()
{
  static bool triger = false;
  static uint32_t LastTickEvents = TickEvents;
  static bool  trigerbg = false;
  bool notick = false;
  static uint16_t thcount = 0;
  //static unsigned long _times = millis();


  // Работает в фоне, меняет индекс 0 -> 1 -> 2 каждые N секунд
  engine.update(Tichina); // вместо таймера переключения режимов


  if(TickEvents == LastTickEvents)
  {
    //Serial.println("Тики не меняются.");
    notick = true;
  }
 

  if(chDt.MonoChenal7l.GetVolume() <= MINIMAL_LEVEL or notick)
  {
    if(!triger){
      PrintDebugInfo("Тишина");
      
      //ChekTishiny = true;
      triger = true;
    }

    if(thcount >= 2){
      if(!trigerbg){
        //if(DebugInfo) Serial.println("Врубаем фон.");
        PrintDebugInfo("Врубаем фон.");
        leds.ClearTo(BACKGROUND);
        leds.Show();
        Tichina = true;
        trigerbg = true;
      }
    }


    // if ((millis() - _times) >= TIME_LENTA_OFF * 1000)
    //   if(digitalRead(SWITCH_PIN) == HIGH)
    //       {digitalWrite(SWITCH_PIN, LOW);}

    if(DebugInfo) 
      if (thcount % 10 == 0) // Проверяем, делится ли thcount на 10 без остатка (выводим каждыый 10)
        Serial.printf("Пака все тихо... %d\n", thcount);

    thcount ++;
  }
  else
  {
    if(trigerbg)
    {
      PrintDebugInfo("Пошел музон...");
      thcount = 0;
      Tichina = false;

      //_times = millis();
      // if(digitalRead(SWITCH_PIN) == LOW)
      //     {digitalWrite(SWITCH_PIN, HIGH);}
    }

    triger = false;
    trigerbg = false;
  }

  //ReadTerminal();


  if(needSendParam) {
    Net::GetParam(); 
    needSendParam = false;
  }
 
  LastTickEvents = TickEvents;
  vTaskDelay(pdMS_TO_TICKS(500)); 
}


void  SetBackGround()
{
  static unsigned long CountMillis = millis() - 2000;
  // отправляем сигнал каждую секунду
  
  if (millis() - CountMillis >= 1000)
  {
    leds.ClearTo(BACKGROUND);
    leds.Show();
    CountMillis = millis();
  }
}

// Задача управления NeoPixel
void Net::LEDTask(void* parameter)
{
  for (;;) {

  

    // Ждём сигнал от производителя
    //ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // сбрасывает счётчик
    if (xSemaphoreTake(dataReadySemaphore, portMAX_DELAY) == pdTRUE) 
    {    
      switch (IncCmd.Command)
      {
        case CMD_SPECTRODATA:
              if(!flStopMode && !Tichina) 
              {
                // Заполняем цветовые палитры для всех эфектов
                CrgbColorRed = ColorFromPalette(RedGrPal, chDt.RedChenal.VolumePeack, chDt.GetMaxBrightness(), LINEARBLEND);
                CrgbColorGreen = ColorFromPalette(GreenGrPal, chDt.GreenChenal.VolumePeack, chDt.GetMaxBrightness(), LINEARBLEND);
                CrgbColorYellow = ColorFromPalette(YellowGrPal, chDt.YellowChenal.VolumePeack, chDt.GetMaxBrightness(), LINEARBLEND);
                CrgbColorOrange = ColorFromPalette(OrangeGrPal, chDt.OrangeChenal.VolumePeack, chDt.GetMaxBrightness(), LINEARBLEND);
                CrgbColorBlue = ColorFromPalette(BlueGrPal, chDt.BlueChenal.VolumePeack, chDt.GetMaxBrightness(), LINEARBLEND);
                CrgbColorViolet = ColorFromPalette(VioletGrPal, chDt.VioletChenal.VolumePeack, chDt.GetMaxBrightness(), LINEARBLEND);
                CrgbColorPurple = ColorFromPalette(IndigoGrPal, chDt.PurpleChenal.VolumePeack, chDt.GetMaxBrightness(), LINEARBLEND);
                CrgbColorGolden = ColorFromPalette(GoldenPal, chDt.GoldChenal.VolumePeack, chDt.GetMaxBrightness(), LINEARBLEND);
                CrgbColorRainbow = ColorFromPalette(RainbowGrPal, chDt.MonoChenal.VolumePeack, chDt.GetMaxBrightness(), LINEARBLEND);

                engine.MakeKadr();        // формирует светомузыкальные кадры 
              }
              _FrameFps++;
        break;
        
        case CMD_STOPMODE:
          flStopMode = (bool)IncCmd.CommandData;
          //Serial.printf("Стоп Моде %d\r\n", flStopMode);
          leds.ClearTo(BACKGROUND);
          leds.Show();
          needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
        break;
        
        case CMD_GETPARAM:
          needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
        break;
        
        case CMD_SWITSHTIME:

          needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
        break;  

        case CMD_MODE:
            //CurrentMode = (LocalMode)IncCmd.CommandData;
            //AutoSwOn = false;                             // если пришла команда на изменения режима,                                               //отключается режим автопереключения
            engine.setCurrentMode(IncCmd.CommandData); // Переключили режим, таймер встал
            needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
        break;  
        
          case CMD_BRIGHTNESS:
            chDt.SetMaxBrightness(IncCmd.CommandData);
            if(DebugInfo) Serial.printf("Яркость - %d\n\r", IncCmd.CommandData);
            needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
          break;  
      }
    }
    else 
      vTaskDelay(50 / portTICK_PERIOD_MS); // обновление каждые 100 мс
  }
}

void Test()
{
   leds.fadeToBlack(128);

    byte LeftValue = scale8(chDt.MonoChenal7l.GetVolume(), 8);
    byte RightValue = scale8(chDt.MonoChenalLocal.GetVolume(), 8);

    HslColor hsl(chDt.MonoChenal7l.GetVolume() / 255.0, 1.0, 0.2);

    HslColor hslGreen(HUE_GREEN / 255.0, 1.0, 0.2);
    HslColor hslBlue(HUE_BLUE/ 255.0, 1.0, 0.2);

    

    for (byte i = 0; i < LeftValue; i++)
    {
      leds.SetPixelColor(i, hsl);
    }

    for (byte i = 0; i < RightValue; i++)
    {
      leds.SetPixelColor(15 - i, hslBlue);
    }

    leds.Show();  
    //Serial.printf("📥 - %d\n", chDt.MonoChenal.GetVolume());
    //Serial.println("📥 Получено byte: ");// + String(packetSize));
}


void Test3()
{
  leds.fadeToBlack(128);


  u_int8_t GreenValue = scale8(chDt.MonoChenal7l.GetVolume(), 7);


  leds(GreenValue) = RgbColor(CrgbColorGreen.r, CrgbColorGreen.g, CrgbColorGreen.b);   
  leds(15, 15 - GreenValue) = RgbColor(CrgbColorRainbow.r, CrgbColorRainbow.g, CrgbColorRainbow.b);
  //for (u_int8_t g = 0; g < GreenValue ; g++)
  //      leds(15 - g) = RgbColor(CrgbColorGreen.r, CrgbColorGreen.g, CrgbColorGreen.b);

  leds.Show(); 
}

void Test2()
{
  leds.fadeToBlack(128);

    u_int8_t RedValue = scale8(chDt.RedChenal.GetVolume(), 3);
    u_int8_t GreenValue = scale8(chDt.GreenChenal.GetVolume(), 3);
    u_int8_t GoldenVal= scale8(chDt.GoldChenal.GetVolume(), 3);
    
    uint8_t mix = (chDt.BlueChenal.GetVolume() + chDt.CyanChenal.GetVolume() + chDt.VioletChenal.GetVolume())/ 3;
    
    u_int8_t BlueValue = scale8(mix, 3);
    //u_int8_t PurpleValue = scale8(chDt.PurpleChenal.GetVolume(), 3);




    //for (u_int8_t r = 0; r < RedValue; r++)
    leds(11, 11 - RedValue) = RgbColor(CrgbColorRed.r, CrgbColorRed.g, CrgbColorRed.b);

    //for (u_int8_t gl = 0; gl < GoldenVal; gl++)
    leds(4, 4 + GoldenVal) = RgbColor(CrgbColorGolden.r, CrgbColorGolden.g, CrgbColorGolden.b);  

    //for (u_int8_t g = 0; g < GreenValue; g++)
    //  leds(12 + g) = RgbColor(CrgbColorGreen.r, CrgbColorGreen.g, CrgbColorGreen.b);
    leds(12, 12 + GreenValue) = RgbColor(CrgbColorGreen.r, CrgbColorGreen.g, CrgbColorGreen.b);

    //for (u_int8_t bl = 0; bl < BlueValue; bl++)
    leds(3, 3 - BlueValue ) = RgbColor(CrgbColorBlue.r, CrgbColorBlue.g, CrgbColorBlue.b);

  leds.Show(); 
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CheckPinsTask(void *pvParameters) {
  uint8_t _pin_swmode = 10;                 // предыдущее состояние вывода для кнопки
  uint8_t _pin_brightness = 0;
  uint8_t _pin_autoswon = 10;
  unsigned long lastDebounceTime = 0;      // последнее время
  unsigned long debounceDelay = 50;        // задержка, увеличьте в случае необходимости
  int _keypress = true;                    // текущее состояние кнопки
  unsigned long TimePressKey = 0;          // Время в течении которого была нажата клавиша
  unsigned long _TimeCounter = millis();
  int buttonState = HIGH;                  // текущее состояние вывода для кнопки переключения режимов
  bool _lastas ;  
  
  
  for(;;) {
        //CheckPins();
      uint8_t pin_swmode = digitalRead(KEY_PIN_SWMODE); // считываем состояние кнопки
 
      // если нажали кнопку,
      // то немного ожидаем, чтобы исключить дребезг
      // если состояние изменилось (дребезг или нажатие)
      if (pin_swmode != _pin_swmode) {
        // сбрасываем таймер
        lastDebounceTime = millis();
      }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
      // whatever the reading is at, it's been there for longer than the debounce
      // delay, so take it as the actual current state:
      // если состояние кнопки изменилось
      // Serial.println(pin_swmode);

      if (pin_swmode != buttonState)
      {
        buttonState = pin_swmode;
        // считаем время в течении которого была нажата кнопка
        if (buttonState == HIGH)
        {
          KeyPress(TimePressKey);
          //Serial.printf("Key Off: %d\n\r", TimePressKey/1000);
          _keypress = false;
          TimePressKey = 0;
          _TimeCounter = 0;
        }
        else
        {
          //Serial.printf("Key On: %d\n\r", TimePressKey/1000);
          _keypress = true;
          _TimeCounter = millis();
          TimePressKey = 0;
        }
      }
      
      if (_keypress)
        TimePressKey = millis() - _TimeCounter;
    }
    
    // сохраняем состояние кнопки. В следующий раз в цикле это станет значением lastButtonState:
    _pin_swmode = pin_swmode;
    
    uint8_t pin_autoswon = digitalRead(KEY_PIN_AUT_SW_ON);

    if(pin_autoswon == 0) 
      AutoSwOn = true;
    else 
      AutoSwOn = false;

    //Serial.println(pin_autoswon);
    if(_lastas != AutoSwOn){
      if(AutoSwOn)
      {
        needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
        PrintDebugInfo("AutoSw On");
      }
      else
      {
        needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
        PrintDebugInfo("AutoSw Off");
      }

      _lastas = AutoSwOn;
    }

   uint8_t pin_brightness = map(analogRead(PIN_BRIGHTNESS),0,2047,32,255);
   if(abs(pin_brightness - _pin_brightness) >= 16)
   {
     //chDt.SetMaxBrightness(pin_brightness);
     _pin_brightness = pin_brightness;
     needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети

     Serial.printf("🌟 Яркость поменялась: %d\n\r", pin_brightness);
   }

  //Serial.println("Проверка пинов");
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

/// Чтение состояния кнопок
void CheckPins()
{
  static uint8_t _pin_swmode = 10;                 // предыдущее состояние вывода для кнопки
  static uint8_t _pin_brightness = 0;
  static uint8_t _pin_autoswon = 10;
  static unsigned long lastDebounceTime = 0;      // последнее время
  static unsigned long debounceDelay = 50;        // задержка, увеличьте в случае необходимости
  static int _keypress = true;                    // текущее состояние кнопки
  static unsigned long TimePressKey = 0;          // Время в течении которого была нажата клавиша
  static unsigned long _TimeCounter = millis();
  static int buttonState = HIGH;                  // текущее состояние вывода для кнопки переключения режимов
  static bool _lastas ;

  // считываем состояние кнопки
  uint8_t pin_swmode = digitalRead(KEY_PIN_SWMODE);
 
  // если нажали кнопку,
  // то немного ожидаем, чтобы исключить дребезг
  // если состояние изменилось (дребезг или нажатие)
  if (pin_swmode != _pin_swmode) {
    // сбрасываем таймер
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
    // если состояние кнопки изменилось
    // Serial.println(pin_swmode);

    if (pin_swmode != buttonState)
    {
      buttonState = pin_swmode;
      // считаем время в течении которого была нажата кнопка
      if (buttonState == HIGH)
      {
        KeyPress(TimePressKey);
        //Serial.printf("Key Off: %d\n\r", TimePressKey/1000);
        _keypress = false;
        TimePressKey = 0;
        _TimeCounter = 0;
      }
      else
      {
        //Serial.printf("Key On: %d\n\r", TimePressKey/1000);
        _keypress = true;
        _TimeCounter = millis();
        TimePressKey = 0;
      }
    }
    
    if (_keypress)
      TimePressKey = millis() - _TimeCounter;
  }
  
  // сохраняем состояние кнопки. В следующий раз в цикле это станет значением lastButtonState:
  _pin_swmode = pin_swmode;


  uint8_t pin_autoswon = digitalRead(KEY_PIN_AUT_SW_ON);
  
  if(pin_autoswon == 0) AutoSwOn = true;
  else AutoSwOn = false;
  //Serial.println(pin_autoswon);
  if(_lastas != AutoSwOn){
    if(AutoSwOn)
    {
      needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
      PrintDebugInfo("AutoSw On");
    }
    else
    {
      needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
      PrintDebugInfo("AutoSw Off");
    }

    _lastas = AutoSwOn;
  }

   uint8_t pin_brightness = map(analogRead(PIN_BRIGHTNESS),0,2047,32,255);
  // //Serial.println(abs(pin_brightness - _pin_brightness));
   if(abs(pin_brightness - _pin_brightness) >= 10)
   {
  //   //Serial.printf("Kn1 %d, Kn2 %d, Brigh %d\n\r", pin_swmode, pin_autoswon, pin_brightness);
     chDt.SetMaxBrightness(pin_brightness);
     _pin_brightness = pin_brightness;
    needSendParam = true; // СИГНАЛ: нужно отправить отчет по сети
  }
   //Serial.printf("🌟Kn1 %d, Kn2 %d, Brigh %d\n\r", pin_swmode, pin_autoswon, pin_brightness);
}

/// обработчик нажатия кнопки
void KeyPress(unsigned long miliSekonds)     
{
  if(miliSekonds == 0) return;
  // Режим переключается если кнопка нажата не более 3 сек
  //if((miliSekonds > 0) && (miliSekonds <= 3000))  engine.NextMode();  
  
  //Serial.printf("Кнопка нажмата: %d\n\r", miliSekonds/1000);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/// @brief  Эффект волна (-)
void Wave()
{
  static uint8_t offset = 0;
  static uint16_t frameCounter = 0;

  leds.fadeToBlack(128);
  
  frameCounter++;


  RgbColor color1 = RgbColor(CrgbColorRed.r, CrgbColorRed.g, CrgbColorRed.b);
  RgbColor color2 = RgbColor(CrgbColorGolden.r, CrgbColorGolden.g, CrgbColorGolden.b);
  RgbColor color3 = RgbColor(CrgbColorBlue.r, CrgbColorBlue.g, CrgbColorBlue.b);
  RgbColor color4 = RgbColor(CrgbColorGreen.r, CrgbColorGreen.g, CrgbColorGreen.b);
  RgbColor color5 = RgbColor(CrgbColorPurple.r, CrgbColorPurple.g, CrgbColorPurple.b);
  RgbColor color6 = RgbColor(CrgbColorRainbow.r, CrgbColorRainbow.g, CrgbColorRainbow.b);


  uint8_t PolBloka = 2;
  uint8_t NumColBlock = 8;

  ColorValue palette[] = {
    {color1, scale8(chDt.RedChenal.GetVolume(),    PolBloka), false},
    {color1, scale8(chDt.RedChenal.GetVolume(),    PolBloka), true},
    {color2, scale8(chDt.GoldChenal.GetVolume(),   PolBloka), false}, 
    {color2, scale8(chDt.GoldChenal.GetVolume(),   PolBloka), true},
    {color3, scale8(chDt.BlueChenal.GetVolume(),   PolBloka), false},
    {color3, scale8(chDt.BlueChenal.GetVolume(),   PolBloka), true},
    {color4, scale8(chDt.GreenChenal.GetVolume(),  PolBloka), false},
    {color4, scale8(chDt.GreenChenal.GetVolume(),  PolBloka), true},
    {color5, scale8(chDt.PurpleChenal.GetVolume(), PolBloka), false},
    {color5, scale8(chDt.PurpleChenal.GetVolume(), PolBloka), true},
    {color6, scale8(chDt.MonoChenal.GetVolume(),   PolBloka), false},
    {color6, scale8(chDt.MonoChenal.GetVolume(),   PolBloka), true}
  };



  if (frameCounter >= 30)
  {
      frameCounter = 0;
      offset = (offset + PolBloka / 4) % LED_COUNT;
  }

  int lastcolorindex = -1;
  int y = 0;

  for (int i = 0; i < LED_COUNT  ; i++) 
  {
    uint8_t shiftedIndex = (i + offset) % (PolBloka * NumColBlock * 2);
    uint8_t blockIndex = shiftedIndex / PolBloka;
    //Serial.println(blockIndex);
    //uint32_t color = palette[blockIndex % BLOCK_COUNT];
    int colorindex = blockIndex % (NumColBlock * 2);
    RgbColor color = palette[colorindex].color;
    if(colorindex != lastcolorindex)
    {
      lastcolorindex = colorindex;            // смена блока
      y = 0;
    }

    if(palette[colorindex].Direct)
    {
      if(y < palette[colorindex].value)
            leds.SetPixelColor(i, color);
    }
    else
    {
      if(y >= PolBloka - palette[colorindex].value)
            leds.SetPixelColor(i, color);
    }

    y++;
  }

  leds.Show();
}


void WaveNew()
{
    static uint16_t frameCounter = 0;
    static int offset = 0;

    leds.fadeToBlack(128);

    uint8_t RealArray = 120;

    ColorValue palette[RealArray] = {0};

    u_int8_t RedValue = scale8(chDt.RedChenal.GetVolume(), PolBloka);
    u_int8_t GoldenVal= scale8(chDt.GoldChenal.GetVolume(), PolBloka);
    u_int8_t GreenValue = scale8(chDt.GreenChenal.GetVolume(), PolBloka);
    
    u_int8_t BlueValue = scale8(chDt.BlueChenal.GetVolume(), PolBloka);
    u_int8_t PurpleValue =scale8(chDt.PurpleChenal.GetVolume(), PolBloka);

    u_int8_t MonotValue = scale8(chDt.MonoChenal7l.GetVolume(), PolBloka);
    u_int8_t RightValue = scale8(chDt.MonoChenalLocal.GetVolume(), PolBloka);


    if( offset >= RealArray) offset = 0;


    for (byte i = 0; i < RedValue; i++)
    {
      palette[i].color  = RgbColor(CrgbColorRed.r, CrgbColorRed.g, CrgbColorRed.b);
    }

    for (byte i = 0; i < GoldenVal; i++)
    {
      palette[i + PolBloka].color  = RgbColor(CrgbColorGolden.r, CrgbColorGolden.g, CrgbColorGolden.b);
    }

    for (byte i = 0; i < GreenValue; i++)
    {
      palette[i + PolBloka*2].color  = RgbColor(CrgbColorGreen.r, CrgbColorGreen.g, CrgbColorGreen.b);
    }
    for (byte i = 0; i < BlueValue; i++)
    {
      palette[i + PolBloka*3].color  = RgbColor(CrgbColorBlue.r, CrgbColorBlue.g, CrgbColorBlue.b);
    }

    for (byte i = 0; i < PurpleValue; i++)
    {
      palette[i + PolBloka*4].color  = RgbColor(CrgbColorPurple.r, CrgbColorPurple.g, CrgbColorPurple.b);
    }


    for (byte i = 0; i < MonotValue; i++)
    {
      palette[i + PolBloka*6].color  = RgbColor(CrgbColorRainbow.r, CrgbColorRainbow.g, CrgbColorRainbow.b);
      palette[PolBloka*6 - i].color = RgbColor(CrgbColorRainbow.r, CrgbColorRainbow.g, CrgbColorRainbow.b);
    }


    // Сдвигаем начало окна . Инструкция (offset + i) % 100 позволяет «окну» плавно перетекать из конца массива в начало.
    
    for (int i = 0; i < LED_COUNT; i++) { // Заполняем LED_COUNT пикселей панели
      int pixelIndex = (offset + i) % RealArray; // Берем индекс с зацикливанием
      
      leds.SetPixelColor(i, palette[pixelIndex].color ); 
      //Serial.printf("pixelIndex : %d\n", pixelIndex);
    }

    leds.Show();
  
  if (frameCounter >= 180)
  {
      frameCounter = 0;
      offset += PolBloka;
      //offset = (offset + PolBloka / 4) % LED_COUNT;
  }
  
  frameCounter++;
}


void raduga_spectrum()
{
  leds.fadeToBlack(128);




  byte Redval = scale8(chDt.RedChenal.GetVolume(), PolBloka ); // Возвращаем количество LED для цвета
  byte Yellowval = scale8(chDt.YellowChenal.GetVolume(), PolBloka );
  byte Greenval = scale8(chDt.GreenChenal.GetVolume(), PolBloka );
  byte Blueval = scale8(chDt.BlueChenal.GetVolume(), PolBloka );
  byte Purpleval = scale8(chDt.PurpleChenal.GetVolume(), PolBloka );



    for (int i = 0; i < Redval; i++)
    {
        leds(PolBlokaAllLed - 1- i) = CrgbColorRed; 
        leds(PolBlokaAllLed + i) = CrgbColorRed;
    }

    for (int i = 0; i < Yellowval; i++)
    {
      leds(PolBlokaAllLed - 1 - Redval - i) = CrgbColorGolden; 
      leds(PolBlokaAllLed + Redval + i) = CrgbColorGolden; 
    }

    for (int i = 0; i < Greenval; i++)
    {
      leds(PolBlokaAllLed - 1 - Redval - Yellowval - i) = CrgbColorGreen; 
      leds(PolBlokaAllLed + Redval + Yellowval + i) = CrgbColorGreen; 
    }

    for (int i = 0; i < Blueval; i++)
    {
      leds(PolBlokaAllLed - 1 - Redval - Yellowval - Greenval - i) = CrgbColorBlue;   
      leds(PolBlokaAllLed + Redval + Yellowval + Greenval + i) = CrgbColorBlue; 
    }
     for (int i = 0; i < Purpleval; i++)
    {
      leds(PolBlokaAllLed - 1 - Redval - Yellowval - Greenval - Blueval - i) = CrgbColorPurple;
      leds(PolBlokaAllLed + Redval + Yellowval + Greenval + Blueval + i) = CrgbColorPurple;
    }

  leds.Show();
}

/// @brief  Класический эффект (-)
void Clasick()
{
    static uint8_t offset = 0;
    static uint16_t frameCounter = 0; 

    leds.fadeToBlack(128);

    int lastcolorindex = -1;
    int y = 0;

  

    RgbColor color1 = RgbColor(CrgbColorRed.r, CrgbColorRed.g, CrgbColorRed.b);
    RgbColor color2 = RgbColor(CrgbColorGolden.r, CrgbColorGolden.g, CrgbColorGolden.b);
    RgbColor color3 = RgbColor(CrgbColorBlue.r, CrgbColorBlue.g, CrgbColorBlue.b);
    RgbColor color4 = RgbColor(CrgbColorGreen.r, CrgbColorGreen.g, CrgbColorGreen.b);
    RgbColor color5 = RgbColor(CrgbColorPurple.r, CrgbColorPurple.g, CrgbColorPurple.b);
    RgbColor color6 = RgbColor(CrgbColorRainbow.r, CrgbColorRainbow.g, CrgbColorRainbow.b);

    byte Redval = scale8(chDt.RedChenal.GetVolume(), PolBloka ); // Возвращаем количество LED для цвета
    byte Yellowval = scale8(chDt.YellowChenal.GetVolume(), PolBloka );
    byte Greenval = scale8(chDt.GreenChenal.GetVolume(), PolBloka );
    byte Blueval = scale8(chDt.BlueChenal.GetVolume(), PolBloka );
    byte Purpleval = scale8(chDt.PurpleChenal.GetVolume(), PolBloka );


    ColorValue palette[] = {
      {color1, scale8(chDt.RedChenal.GetVolume(),    PolBloka), false},
      {color1, scale8(chDt.RedChenal.GetVolume(),    PolBloka), true},
      {color2, scale8(chDt.GoldChenal.GetVolume(),   PolBloka), false}, 
      {color2, scale8(chDt.GoldChenal.GetVolume(),   PolBloka), true},
      {color3, scale8(chDt.BlueChenal.GetVolume(),   PolBloka), false},
      {color3, scale8(chDt.BlueChenal.GetVolume(),   PolBloka), true},
      {color4, scale8(chDt.GreenChenal.GetVolume(),  PolBloka), false},
      {color4, scale8(chDt.GreenChenal.GetVolume(),  PolBloka), true},
      {color5, scale8(chDt.PurpleChenal.GetVolume(), PolBloka), false},
      {color5, scale8(chDt.PurpleChenal.GetVolume(), PolBloka), true},
      {color6, scale8(chDt.MonoChenal.GetVolume(),   PolBloka), false},
      {color6, scale8(chDt.MonoChenal.GetVolume(),   PolBloka), true}
    };

    if (frameCounter >= 30)
    {
        frameCounter = 0;
        offset = (offset + PolBloka / 4) % LED_COUNT;
    }

    for (int i = 0; i < LED_COUNT; i++) 
    {
      uint8_t shiftedIndex = (i + offset) % (PolBloka * NUMBCOLORBLOCKS * 2);
      uint8_t blockIndex = shiftedIndex / PolBloka;
      //Serial.println(blockIndex);
      //uint32_t color = palette[blockIndex % BLOCK_COUNT];
      int colorindex = blockIndex % (NUMBCOLORBLOCKS * 2);
      RgbColor color = palette[colorindex].color;
      
      if(colorindex != lastcolorindex)
      {
        lastcolorindex = colorindex;            // смена блока
        y = 0;
      }

      if(palette[colorindex].Direct)
      {
        if(y < palette[colorindex].value)
              leds.SetPixelColor(i, color);
      }
      else
      {
        if(y >= PolBloka - palette[colorindex].value)
              leds.SetPixelColor(i, color);
      }

      y++;
    }


   leds.Show();
}


// Радуга стреляющая огоньками (+)
void raduga_fier()
{
  byte nl;
  byte BeginPoint, EndPoint;
  // static byte lastep;
  static byte peackRed, peackGreen, peackPurple, peackYellow, peackBlue;
  static uint16_t LastTicPurple = 0; //;
  static uint16_t LastTicRed = 0;
  static uint16_t LastTicGreen = 0;
  static uint16_t LastTicYellow = 0;
  static uint16_t LastTicBlue = 0;

  static uint16_t tic = 0; // количество входов в функцию
  uint16_t MinSpeed = 5;

  uint8_t LedInBlock = 12;

  leds.fadeToBlack(128);

  nl = scale8_video(chDt.RedChenal.GetVolume(), LedInBlock); // уровень красного канала
  BeginPoint = 0;
  EndPoint = BeginPoint + nl;
  leds(BeginPoint, EndPoint) = CrgbColorRed;

  // Leds(0, 1) = 12;

  byte spd = scale8_video(chDt.RedChenal.VolumePeack, MinSpeed);
  if (tic - LastTicRed >= MinSpeed - spd)
  {
    LastTicRed = tic;
    peackRed++;
  }
  if (peackRed > LED_COUNT - 1)
    peackRed = EndPoint;

  nl = scale8_video(chDt.YellowChenal.GetVolume(), LedInBlock);
  BeginPoint = EndPoint + 1;
  EndPoint = BeginPoint + nl;
  leds(BeginPoint, EndPoint) = CrgbColorYellow;

  spd = scale8_video(chDt.YellowChenal.VolumePeack, MinSpeed);
  if (tic - LastTicYellow >= MinSpeed - spd)
  {
    LastTicYellow = tic;
    peackYellow++;
  }

  if (peackYellow > LED_COUNT - 1)
    peackYellow = EndPoint;

  nl = scale8_video(chDt.GreenChenal.GetVolume(), LedInBlock);
  BeginPoint = EndPoint + 1;
  EndPoint = BeginPoint + nl;
  leds(BeginPoint, EndPoint) = CrgbColorGreen;

  spd = scale8_video(chDt.GreenChenal.VolumePeack, MinSpeed);
  if (tic - LastTicGreen >= MinSpeed - spd)
  {
    LastTicGreen = tic;
    peackGreen++;
  }
  if (peackGreen > LED_COUNT - 1)
    peackGreen = EndPoint;

  nl = scale8_video(chDt.BlueChenal.GetVolume(), LedInBlock);
  BeginPoint = EndPoint + 1;
  EndPoint = BeginPoint + nl;
  leds(BeginPoint, EndPoint) = CrgbColorBlue;

  spd = scale8_video(chDt.BlueChenal.VolumePeack, MinSpeed);
  if (tic - LastTicBlue >= MinSpeed - spd)
  {
    LastTicBlue = tic;
    peackBlue++;
  }

  if (peackBlue > LED_COUNT - 1)
    peackBlue = EndPoint;

  nl = scale8_video(chDt.PurpleChenal.GetVolume(), LedInBlock);
  BeginPoint = EndPoint + 1;
  EndPoint = BeginPoint + nl;
  leds(BeginPoint, EndPoint) = CrgbColorPurple;

  spd = scale8_video(chDt.PurpleChenal.VolumePeack, MinSpeed);
  if ((tic - LastTicPurple) >= (MinSpeed - spd))
  {
    LastTicPurple = tic;
    peackPurple++;
  }

  if (peackPurple > LED_COUNT - 1)
    peackPurple = EndPoint;

  if (peackRed > EndPoint)
    leds(peackRed) = CrgbColorRed;
  if (peackYellow > EndPoint)
    leds(peackYellow) = CrgbColorYellow;
  if (peackGreen > EndPoint)
    leds(peackGreen) = CrgbColorGreen;
  if (peackBlue > EndPoint)
    leds(peackBlue) = CrgbColorBlue;
  if (peackPurple > EndPoint)
    leds(peackPurple) = CrgbColorPurple;


  leds.Show();


  tic++;
}


/// @brief 
void raduga_pixel()
{
  leds.fadeToBlack(32);

  byte Redval = scale8(chDt.RedChenal.GetVolume(), PolBloka ); // Возвращаем количество LED для цвета
  byte Yellowval = scale8(chDt.YellowChenal.GetVolume(), PolBloka );
  byte Greenval = scale8(chDt.GreenChenal.GetVolume(), PolBloka);
  byte Blueval = scale8(chDt.BlueChenal.GetVolume(), PolBloka );
  byte Purpleval = scale8(chDt.PurpleChenal.GetVolume(), PolBloka );

  // Purpleval = PolBloka;
  // Blueval = PolBloka;
  // Greenval = PolBloka;
  // Yellowval = PolBloka;
  // Redval = PolBloka;

  leds(PolBlokaAllLed - Redval) = CrgbColorRed;                                                 // CHSV(HUE_RED ,255,brighten8_video(Red));
  leds(PolBlokaAllLed - Redval - Yellowval) = CrgbColorYellow;                                  // CHSV(HUE_YELLOW ,255,brighten8_video(Yellow));
  leds(PolBlokaAllLed - Redval - Yellowval - Greenval) = CrgbColorGreen;                        // CHSV(HUE_GREEN ,255,brighten8_video(Green));
  leds(PolBlokaAllLed - Redval - Yellowval - Greenval - Blueval) = CrgbColorBlue;               // CHSV(HUE_BLUE ,255,brighten8_video(Blue));
  leds(PolBlokaAllLed - Redval - Yellowval - Greenval - Blueval - Purpleval) = CrgbColorPurple; // CHSV(0xCD ,255,brighten8_video(Purple));

  leds(PolBlokaAllLed - 1 + Redval) = CrgbColorRed;                                                 // CHSV(HUE_RED ,255,brighten8_video(Red));
  leds(PolBlokaAllLed - 1 +  Redval + Yellowval) = CrgbColorYellow;                                  // CHSV(HUE_YELLOW ,255,brighten8_video(Yellow));
  leds(PolBlokaAllLed - 1 + Redval + Yellowval + Greenval) = CrgbColorGreen;                        // CHSV(HUE_GREEN ,255,brighten8_video(Green));
  leds(PolBlokaAllLed - 1 + Redval + Yellowval + Greenval + Blueval) = CrgbColorBlue;               // CHSV(HUE_BLUE ,255,brighten8_video(Blue));
  leds(PolBlokaAllLed - 1 + Redval + Yellowval + Greenval + Blueval + Purpleval) = CrgbColorPurple; // CHSV(0xCD ,255,brighten8_video(Purple));


  leds.Show();
}
#include "interface.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>

#if defined(HAS_CAPACITIVE_TOUCH)
    #include "CYD28_TouchscreenC.h"
    #define CYD28_DISPLAY_HOR_RES_MAX 240
    #define CYD28_DISPLAY_VER_RES_MAX 320
    CYD28_TouchC touch(CYD28_DISPLAY_HOR_RES_MAX, CYD28_DISPLAY_VER_RES_MAX);
#elif defined(USE_TFT_eSPI_TOUCH)
    #define XPT2046_CS TOUCH_CS
    bool _IH_touched = false;
    void IRAM_ATTR _IH_touch(void){
        _IH_touched=true;
    }
#else
    #include "CYD28_TouchscreenR.h"
    #define CYD28_DISPLAY_HOR_RES_MAX 320
    #define CYD28_DISPLAY_VER_RES_MAX 240  
    CYD28_TouchR touch(CYD28_DISPLAY_HOR_RES_MAX, CYD28_DISPLAY_VER_RES_MAX);
    #if defined(TOUCH_XPT2046_SPI)
        #define XPT2046_CS XPT2046_SPI_CONFIG_CS_GPIO_NUM
    #else
        #define XPT2046_CS 33
    #endif
#endif

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
SPIClass touchSPI;
void _setup_gpio() { 
    #ifndef HAS_CAPACITIVE_TOUCH // Capacitive Touchscreen uses I2C to communicate
        pinMode(XPT2046_CS, OUTPUT);
        digitalWrite(XPT2046_CS, HIGH);
    #endif

    #if !defined(USE_TFT_eSPI_TOUCH) // Use libraries
    if(!touch.begin()) {
        Serial.println("Touch IC not Started");
        log_i("Touch IC not Started");
    } else log_i("Touch IC Started");
    #endif
    #if defined(USE_TFT_eSPI_TOUCH)
        pinMode(TOUCH_CS, OUTPUT);
        attachInterrupt(TOUCH_CONFIG_INT_GPIO_NUM,_IH_touch,FALLING);
    #endif
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() { 
    // Brightness control must be initialized after tft in this case @Pirata
    pinMode(TFT_BL,OUTPUT);
    ledcSetup(TFT_BRIGHT_CHANNEL,TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits); //Channel 0, 10khz, 8bits
    ledcAttachPin(TFT_BL, TFT_BRIGHT_CHANNEL);
    ledcWrite(TFT_BRIGHT_CHANNEL,255);
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { 
    int dutyCycle;
    if (brightval==100) dutyCycle=255;
    else if (brightval==75) dutyCycle=130;
    else if (brightval==50) dutyCycle=70;
    else if (brightval==25) dutyCycle=20;
    else if (brightval==0) dutyCycle=0;
    else dutyCycle = ((brightval*255)/100);

    log_i("dutyCycle for bright 0-255: %d",dutyCycle);
    ledcWrite(TFT_BRIGHT_CHANNEL,dutyCycle); // Channel 0
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static long tmp=0;
    if (millis()-tmp>200) { // I know R3CK.. I Should NOT nest if statements..
                            // but it is needed to not keep SPI bus used without need, it save resources
      
      #if defined(USE_TFT_eSPI_TOUCH)
        #if defined(CYD2432S024R)
            //uint16_t calData[5] = { 481, 3053, 433, 3296, 3 }; // from https://github.com/Fr4nkFletcher/ESP32-Marauder-Cheap-Yellow-Display/blob/3eed991e9336d3e711e3eb5d6ece7ba023132fef/esp32_marauder/Display.cpp#L43
        #elif defined(CYD2432W328R)
            //uint16_t calData[5] = { 350, 3465, 188, 3431, 2 }; // from https://github.com/Fr4nkFletcher/ESP32-Marauder-Cheap-Yellow-Display/blob/3eed991e9336d3e711e3eb5d6ece7ba023132fef/esp32_marauder/Display.cpp#L40
        #endif
        uint16_t calData[5] = { 391, 3491, 266, 3505, 7 };
        tft.setTouch(calData);
        TouchPoint t;
        checkPowerSaveTime();

        if(_IH_touched) {
            NextPress=false;
            PrevPress=false;
            UpPress=false;
            DownPress=false;
            SelPress=false;
            EscPress=false;
            AnyKeyPress=false;
            NextPagePress=false;
            PrevPagePress=false;
            touchPoint.pressed=false;
            _IH_touched=false;
            digitalWrite(TFT_CS,HIGH);
            digitalWrite(TOUCH_CS,LOW);
            tft.getTouch(&t.x, &t.y,50);
            digitalWrite(TOUCH_CS,HIGH);
            Serial.printf("Touched with Z=%d", tft.getTouchRawZ());
      #else
      if(touch.touched()) { 
        auto t = touch.getPointScaled();
        t = touch.getPointScaled();
      #endif

        if(bruceConfig.rotation==3) {
            t.y = (tftHeight+20)-t.y;
            t.x = tftWidth-t.x;
        }
        if(bruceConfig.rotation==0) {
            int tmp=t.x;
            t.x = tftWidth-t.y;
            t.y = tmp;
        }
        if(bruceConfig.rotation==2) {
            int tmp=t.x;
            t.x = t.y;
            t.y = (tftHeight+20)-tmp;
        }
        Serial.printf("Touch Pressed on x=%d, y=%d\n",t.x, t.y);

        if(!wakeUpScreen()) AnyKeyPress = true;
        else goto END;

        // Touch point global variable
        touchPoint.x = t.x;
        touchPoint.y = t.y;
        touchPoint.pressed=true;
        touchHeatMap(touchPoint);

        tmp=millis();
      }
    }
    END:
    delay(0);
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() { 
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0,LOW); 
    esp_deep_sleep_start();
}


/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to tornoff the device (name is odd btw)
**********************************************************************/
void checkReboot() { }
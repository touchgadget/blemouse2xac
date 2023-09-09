/*
 * This program is based on https://github.com/h2zero/NimBLE-Arduino/tree/master/examples/NimBLE_Client.
 * My changes are covered by the MIT license.
 */

/*
 * MIT License
 *
 * Copyright (c) 2023 touchgadgetdev@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define DUMP_REPORT_MAP 0
#define DEV_INFO_SERVICE 0    // BLE device information

// Set to 0 to remove the CDC ACM serial port but XAC seems to tolerate it
// so it is on by default. Set to 0 if it causes problems. Disabling the
// CDC port means you must press button(s) on the ESP32S3 to put in bootloader
// upload mode before using the IDE to upload.
#define USB_DEBUG 0

#if USB_DEBUG
#define DBG_begin(...)    Serial.begin(__VA_ARGS__)
#define DBG_end(...)      Serial.end(__VA_ARGS__)
#define DBG_print(...)    Serial.print(__VA_ARGS__)
#define DBG_println(...)  Serial.println(__VA_ARGS__)
#define DBG_printf(...)   Serial.printf(__VA_ARGS__)
#else
#define DBG_begin(...)
#define DBG_end(...)
#define DBG_print(...)
#define DBG_println(...)
#define DBG_printf(...)
#endif

#include <esp_wifi.h>

#if defined(ARDUINO_M5Stack_ATOMS3)
#include <OneButton.h>  // https://github.com/mathertel/OneButton
#include <FastLED.h>    // https://github.com/FastLED/FastLED
#include <M5GFX.h>      // https://github.com/m5stack/M5GFX
/* M5Stack AtomS3 display, RGB LED, button */
M5GFX display;
#define TFT_print(...)   do { clrscr(); display.print(__VA_ARGS__); } while (0)
#define TFT_println(...) do { clrscr(); display.println(__VA_ARGS__); } while (0)
#define TFT_printf(...)  do { clrscr(); display.printf(__VA_ARGS__); } while (0)
#define TFT_color(foreground, background)  display.setTextColor(foreground, background)
#define RGBLed(color)    do { RGBled = color; FastLED.show(); } while (0)

static const int LED_DI_PIN = 35;
CRGB RGBled;

static const int BTN_PIN = 41;
OneButton button(BTN_PIN, true);

static const int TEXT_SIZE = 3;

#elif defined(ARDUINO_LILYGO_T_DISPLAY_S3)
#include "pin_config.h"
#include "OneButton.h" // https://github.com/mathertel/OneButton
#include "TFT_eSPI.h"  // https://github.com/Bodmer/TFT_eSPI
#include <FastLED.h>   // https://github.com/FastLED/FastLED

TFT_eSPI tft = TFT_eSPI();
CRGB RGBled;
OneButton button(BTN_PIN, true);
#define TFT_print(...)   do { clrscr(); tft.print(__VA_ARGS__); } while (0)
#define TFT_println(...) do { clrscr(); tft.println(__VA_ARGS__); } while (0)
#define TFT_printf(...)  do { clrscr(); tft.printf(__VA_ARGS__); } while (0)
#define TFT_color(foreground, background)  tft.setTextColor(foreground, background)
#define RGBLed(color)    do { RGBled = color; FastLED.show(); } while (0)
#else   // No display
#define TFT_print(...)
#define TFT_println(...)
#define TFT_printf(...)
#define TFT_color(...)
#define RGBLed(...)
#endif

#include "USB.h"
#include "ESP32_flight_stick.h"
ESP32_flight_stick FSJoy;

typedef struct {
  uint8_t report[32];
  size_t report_len;
  uint32_t last_millis = 0;
  FSJoystick_Report_t joyRpt;
  int16_t xmin = SCHAR_MIN;
  int16_t xmax = SCHAR_MAX;
  int16_t ymin = SCHAR_MIN;
  int16_t ymax = SCHAR_MAX;
  bool isNotify;
  bool available;
  bool isJellyComb;
} Mouse_xfer_state_t;

volatile Mouse_xfer_state_t Mouse_xfer;

extern "C" {
#include "./report_desc.h"
}

const uint16_t JellyComb_VID = 0x1915;
const uint16_t JellyComb_PID = 0x0040;
const uint16_t VRFortune_VID = 0x07d7;
const uint16_t VRFortune_PID = 0x0000;

// Install NimBLE-Arduino by h2zero using the IDE library manager.
#include <NimBLEDevice.h>

const uint16_t APPEARANCE_HID_GENERIC = 0x3C0;
const uint16_t APPEARANCE_HID_KEYBOARD = 0x3C1;
const uint16_t APPEARANCE_HID_MOUSE   = 0x3C2;
const uint16_t APPEARANCE_HID_JOYSTICK = 0x3C3;
const uint16_t APPEARANCE_HID_GAMEPAD = 0x3C4;
const uint16_t APPEARANCE_HID_DIGITIZER_TABLET = 0x3C5;
const uint16_t APPEARANCE_HID_CARD_READER = 0x3C6;
const uint16_t APPEARANCE_HID_DIGITAL_PEN = 0x3C7;
const uint16_t APPEARANCE_HID_BARCODE_SCANNER = 0x3C8;
const uint16_t APPEARANCE_HID_TOUCHPAD = 0x3C9;
const uint16_t APPEARANCE_HID_PRESENTATION_REMOTE = 0x3CA;

const char DEVICE_INFORMATION_SERVICE[] = "180A";
const char DIS_SYSTEM_ID_CHAR[] = "2A23";
const char DIS_MODEL_NUM_CHAR[] = "2A24";
const char DIS_SERIAL_NUM_CHAR[] = "2A25";
const char DIS_FIRMWARE_REV_CHAR[] = "2A26";
const char DIS_HARDWARE_REV_CHAR[] = "2A27";
const char DIS_SOFTWARE_REV_CHAR[] = "2A28";
const char DIS_MANUFACTURE_NAME_CHAR[] = "2A29";
const char DIS_PNP_ID_CHAR[] = "2A50";
const char HID_SERVICE[] = "1812";
const char HID_INFORMATION[] = "2A4A";
const char HID_REPORT_MAP[] = "2A4B";
const char HID_CONTROL_POINT[] = "2A4C";
const char HID_REPORT_DATA[] = "2A4D";
const char HID_PROTOCOL_MODE[] = "2A4E";
const char HID_BOOT_KEYBOARD_OUTPUT_REPORT[] = "2A32";
const char HID_BOOT_MOUSE_INPUT_REPORT[] = "2A33";

void scanEndedCB(NimBLEScanResults results);

static NimBLEAdvertisedDevice* advDevice;

static bool doConnect = false;
static uint32_t scanTime = 0; /** 0 = scan forever */

#if defined(ARDUINO_M5Stack_ATOMS3)
void clrscr() {
  display.clear(TFT_BLACK);
  // Set "cursor" at top left corner of display (0,0) and select font 2
  // (cursor will move to next line automatically during printing with 'display.println'
  //  or stay on the line is there is room for the text with display.print)
  display.setCursor(0, 0);
  display.setTextSize(TEXT_SIZE);
}

void setup_m5_display() {
  display.begin();

  if (display.isEPD()) {
    display.setEpdMode(epd_mode_t::epd_fastest);
    display.invertDisplay(true);
    display.clear(TFT_BLACK);
  }
  if (display.width() < display.height()) {
    display.setRotation(display.getRotation() ^ 1);
  }

  display.setColorDepth(8);
  display.setTextWrap(true);
  display.setTextSize(TEXT_SIZE);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT_print("BLE Mouse XAC");
}

void setup_m5stack_atoms3() {
  /* M5Stack AtomS3 display, RGB LED, and button */
  FastLED.addLeds<WS2812, LED_DI_PIN, GRB>(&RGBled, 1);
  RGBLed(CRGB::Black);
  setup_m5_display();
  display.setTextColor(TFT_YELLOW, TFT_BLACK);
}
#elif defined(ARDUINO_LILYGO_T_DISPLAY_S3)
void clrscr() {
  tft.fillScreen(TFT_BLACK);
  // Set "cursor" at top left corner of display (0,0) and select font 2
  // (cursor will move to next line automatically during printing with 'tft.println'
  //  or stay on the line is there is room for the text with tft.print)
  tft.setCursor(0, 0, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
}

void setup_t_dongle_s3() {
  pinMode(TFT_LEDA_PIN, OUTPUT);
  // Initialise TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  // Turn on backlight
  digitalWrite(TFT_LEDA_PIN, 0);
  TFT_print("BLE Mouse to XAC Joy");

  // Init RGB LED off. BGR ordering is typical
  FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(&RGBled, 1);
  FastLED.setBrightness(64);
  RGBLed(CRGB::Black);
}
#endif

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) {
    DBG_println("Connected");
    TFT_println("Connected");
    /** After connection we should change the parameters if we don't need fast response times.
     *  These settings are 150ms interval, 0 latency, 450ms timout.
     *  Timeout should be a multiple of the interval, minimum is 100ms.
     *  I find a multiple of 3-5 * the interval works best for quick response/reconnect.
     *  Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout
     */
    pClient->updateConnParams(120,120,0,60);
    DBG_printf("%s: peer MTU %u\n", __func__, pClient->getMTU());
  };

  void onDisconnect(NimBLEClient* pClient) {
    DBG_print(pClient->getPeerAddress().toString().c_str());
    DBG_println(" Disconnected - Starting scan");
    TFT_color(TFT_YELLOW, TFT_BLACK);
    TFT_print("Disconnect\nScanning");
    RGBLed(CRGB::Yellow);
    NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
  };

  /** Called when the peripheral requests a change to the connection parameters.
   *  Return true to accept and apply them or false to reject and keep
   *  the currently used parameters. Default will return true.
   */
  bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
    // Failing to accepts parameters may result in the remote device
    // disconnecting.
    return true;
  };

  /********************* Security handled here **********************
   ****** Note: these are the same return values as defaults ********/
  uint32_t onPassKeyRequest(){
    DBG_println("Client Passkey Request");
    /** return the passkey to send to the server */
    return 123456;
  };

  bool onConfirmPIN(uint32_t pass_key){
    DBG_print("The passkey YES/NO number: ");
    DBG_println(pass_key);
    /** Return false if passkeys don't match. */
    return true;
  };

  /** Pairing process complete, we can check the results in ble_gap_conn_desc */
  void onAuthenticationComplete(ble_gap_conn_desc* desc){
    if(!desc->sec_state.encrypted) {
      DBG_println("Encrypt connection failed - disconnecting");
      /** Find the client with the connection handle provided in desc */
      NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
      return;
    }
  };
};

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {

  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    uint8_t advType = advertisedDevice->getAdvType();
    if ((advType == BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_HD) ||
        (advType == BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_LD) ||
        (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(NimBLEUUID(HID_SERVICE))))
    {
      DBG_printf("onResult: AdvType= %d\r\n", advType);
      DBG_print("Advertised HID Device found: ");
      DBG_println(advertisedDevice->toString().c_str());

      /** stop scan before connecting */
      NimBLEDevice::getScan()->stop();
      /** Save the device reference in a global for the client to use*/
      advDevice = advertisedDevice;
      /** Ready to connect now */
      doConnect = true;
    }
  };
};

/** Notification / Indication receiving handler callback */
// Notification from 4c:75:25:xx:yy:zz: Service = 0x1812, Characteristic = 0x2a4d, Value = 1,0,0,0,0,
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic,
    uint8_t* pData, size_t length, bool isNotify) {
  if (Mouse_xfer.available) {
    static uint32_t dropped = 0;
    printf("drops=%u\r\n", ++dropped);
  } else {
    // DBG_println(pRemoteCharacteristic->toString().c_str());
    Mouse_xfer.isNotify = isNotify;
    Mouse_xfer.report_len = length;
    memcpy((void *)Mouse_xfer.report, pData, min(length, sizeof(Mouse_xfer.report)));
    Mouse_xfer.available = true;
    Mouse_xfer.last_millis = millis();
  }
}

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results){
  DBG_println("Scan Ended");
}


/** Create a single global instance of the callback class to be used by all clients */
static ClientCallbacks clientCB;

static NimBLEAddress LastBLEAddress;

/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer()
{
  NimBLEClient* pClient = nullptr;
  bool reconnected = false;

  DBG_printf("Client List Size: %d\r\n", NimBLEDevice::getClientListSize());
  /** Check if we have a client we should reuse first **/
  if(NimBLEDevice::getClientListSize()) {
    /** Special case when we already know this device, we send false as the
     *  second argument in connect() to prevent refreshing the service database.
     *  This saves considerable time and power.
     */
    pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
    if(pClient) {
      if (pClient->getPeerAddress() == LastBLEAddress) {
        if(!pClient->connect(advDevice, false)) {
          DBG_println("Reconnect failed");
          TFT_color(TFT_YELLOW, TFT_BLACK);
          RGBLed(CRGB::Yellow);
          TFT_println("Reconnect failed");
          return false;
        }
        DBG_println("Reconnected client");
        TFT_color(TFT_GREEN, TFT_BLACK);
        TFT_println("Reconnected client");
        RGBLed(CRGB::Green);
        reconnected = true;
      }
    }
    /** We don't already have a client that knows this device,
     *  we will check for a client that is disconnected that we can use.
     */
    else {
      pClient = NimBLEDevice::getDisconnectedClient();
    }
  }

  /** No client to reuse? Create a new one. */
  if(!pClient) {
    if(NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
      DBG_println("Max clients reached - no more connections available");
      return false;
    }

    pClient = NimBLEDevice::createClient();

    DBG_println("New client created");

    pClient->setClientCallbacks(&clientCB, false);
    /** Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
     *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
     *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
     *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
     */
    pClient->setConnectionParams(12,12,0,51);
    /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
    pClient->setConnectTimeout(5);


    if (!pClient->connect(advDevice)) {
      /** Created a client but failed to connect, don't need to keep it as it has no data */
      NimBLEDevice::deleteClient(pClient);
      DBG_println("Failed to connect, deleted client");
      return false;
    }
  }

  if(!pClient->isConnected()) {
    if (!pClient->connect(advDevice)) {
      DBG_println("Failed to connect");
      return false;
    }
  }

  LastBLEAddress = pClient->getPeerAddress();
  DBG_print("Connected to: ");
  DBG_println(pClient->getPeerAddress().toString().c_str());
  DBG_print("RSSI: ");
  DBG_println(pClient->getRssi());

  /** Now we can read/write/subscribe the charateristics of the services we are interested in */
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;
  NimBLERemoteDescriptor* pDsc = nullptr;

  
  Mouse_xfer.xmin = SCHAR_MIN;
  Mouse_xfer.xmax = SCHAR_MAX;
  Mouse_xfer.ymin = SCHAR_MIN;
  Mouse_xfer.ymax = SCHAR_MAX;

#if DEV_INFO_SERVICE
  // Device Information Service
  pSvc = pClient->getService(DEVICE_INFORMATION_SERVICE);
  if(pSvc) {     /** make sure it's not null */
    DBG_println(pSvc->toString().c_str());
    std::vector<NimBLERemoteCharacteristic*>*charvector;
    charvector = pSvc->getCharacteristics(true);
    for (auto &it: *charvector) {
      DBG_print(it->toString().c_str());
      if(it->canRead()) {
        DBG_print(" Value: ");
        DBG_println(it->readValue().c_str());
      }
    }
    pChr = pSvc->getCharacteristic(DIS_PNP_ID_CHAR);
    if(pChr) {     /** make sure it's not null */
      if(pChr->canRead()) {
        typedef struct __attribute__((__packed__)) {
          uint8_t vendor_id_source;
          uint16_t vendor_id;
          uint16_t product_id;
          uint16_t product_version;
        } pnp_id_t;
        pnp_id_t *pnp_id = (pnp_id_t *)pChr->readValue().data();
        uint16_t vid = pnp_id->vendor_id;
        uint16_t pid = pnp_id->product_id;
        DBG_printf("PNP ID: source: %02x, vendor: %04x, product: %04x, version: %04x\r\n",
            pnp_id->vendor_id_source, vid, pid, pnp_id->product_version);
        Mouse_xfer.isJellyComb =
          ((vid == JellyComb_VID) && (pid == JellyComb_PID));
      }
    }
  } else {
    DBG_println("Device Information Service not found.");
  }
#endif

  pSvc = pClient->getService(HID_SERVICE);
  if(pSvc) {     /** make sure it's not null */
      // This returns the HID report descriptor like this
      // HID_REPORT_MAP 0x2a4b Value: 5,1,9,2,A1,1,9,1,A1,0,5,9,19,1,29,5,15,0,25,1,75,1,
      // Copy and paste the value digits to http://eleccelerator.com/usbdescreqparser/
      // to see the decoded report descriptor.
      pChr = pSvc->getCharacteristic(HID_REPORT_MAP);
      if(pChr) {     /** make sure it's not null */
        if(pChr->canRead()) {
          std::string value = pChr->readValue();
          if (!reconnected) {
            parse_hid_report_descriptor((const uint8_t *)value.c_str(),
                (size_t) value.length(), false);
          }
#if DUMP_REPORT_MAP
          DBG_print("HID_REPORT_MAP ");
          DBG_print(pChr->getUUID().toString().c_str());
          DBG_print(" Value: ");
          uint8_t *p = (uint8_t *)value.data();
          for (size_t i = 0; i < value.length(); i++) {
            DBG_print(p[i], HEX);
            DBG_print(',');
          }
          DBG_println();
#endif
        }
      }
      else {
        DBG_println("HID REPORT MAP char not found.");
      }

    // Subscribe to characteristics HID_REPORT_DATA.
    // One real device reports 2 with the same UUID but
    // different handles. Using getCharacteristic() results
    // in subscribing to only one.
    std::vector<NimBLERemoteCharacteristic*>*charvector;
    charvector = pSvc->getCharacteristics(true);
    for (auto &it: *charvector) {
      if (it->getUUID() == NimBLEUUID(HID_REPORT_DATA)) {
        DBG_println(it->toString().c_str());
        if (it->canNotify()) {
          if(it->subscribe(true, notifyCB)) {
            DBG_println("subscribe notification OK");
          } else {
            /** Disconnect if subscribe failed */
            DBG_println("subscribe notification failed");
            pClient->disconnect();
            return false;
          }
        }
        if (it->canIndicate()) {
          if(it->subscribe(false, notifyCB)) {
            DBG_println("subscribe indication OK");
          } else {
            /** Disconnect if subscribe failed */
            DBG_println("subscribe indication failed");
            pClient->disconnect();
            return false;
          }
        }
      }
    }
  }
  DBG_println("Done with this device!");
  return true;
}

void setup ()
{
  // esp_wifi_stop();
  DBG_begin(115200);
  FSJoy.begin();
  USB.begin();
  FSJoy.write();
#if defined(ARDUINO_M5Stack_ATOMS3)
  setup_m5stack_atoms3();
#elif defined(ARDUINO_LILYGO_T_DISPLAY_S3)
  setup_t_dongle_s3();
#endif

#if defined(ARDUINO_LILYGO_T_DISPLAY_S3) || defined(ARDUINO_M5Stack_ATOMS3)
  // Place holder code for various button inputs. Replace or delete the code
  // as needed.
#if 0
  button.attachClick([] {
      TFT_color(TFT_RED, TFT_BLACK);
      TFT_print("Button click");
      DBG_println("Button click");
      RGBLed(CRGB::Red);
      });
  button.attachDoubleClick([] {
      TFT_color(TFT_GREEN, TFT_BLACK);
      TFT_print("Button double click");
      DBG_println("Button double click");
      RGBLed(CRGB::Green);
      });
#endif
  button.attachMultiClick([] {
      //reset settings - wipe bonding credentials
      NimBLEDevice::deleteAllBonds();
      TFT_color(TFT_RED, TFT_BLACK);
      TFT_print("Bonds Erased");
      DBG_println("Bonds Erased");
      RGBLed(CRGB::Red);
      delay(1000);
      ESP.restart();
      });
#if 0
  button.attachLongPressStart([] {
      TFT_color(TFT_MAGENTA, TFT_BLACK);
      TFT_print("Button long press start");
      DBG_println("Button long press start");
      RGBLed(CRGB::Magenta);
      });
  button.attachDuringLongPress([] {
      TFT_color(TFT_YELLOW, TFT_BLACK);
      TFT_print("Button during long press");
      DBG_println("Button during long press");
      RGBLed(CRGB::Yellow);
      });
  button.attachLongPressStop([] {
      TFT_color(TFT_WHITE, TFT_BLACK);
      TFT_print("Button long press stop");
      DBG_println("Button long press stop");
      RGBLed(CRGB::White);
      });
#endif

#endif  // defined(ARDUINO_LILYGO_T_DISPLAY_S3) || defined(ARDUINO_M5Stack_ATOMS3)

#if USB_DEBUG
  while (!Serial && (millis() < 3000)) delay(10);   // wait for native usb
#endif
  DBG_println("Starting XAC Joystick");

  DBG_println("Starting NimBLE HID Client");
  /** Initialize NimBLE, no device name spcified as we are not advertising */
  NimBLEDevice::init("");
  if (NimBLEDevice::setMTU(512)) {
    DBG_printf("%s: setMTU(512) failed\n", __func__);
  }
  DBG_printf("%s: getMTU %d\n", __func__, NimBLEDevice::getMTU());

  /** Set the IO capabilities of the device, each option will trigger a different pairing method.
   *  BLE_HS_IO_KEYBOARD_ONLY    - Passkey pairing
   *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
   *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
   */
  //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY); // use passkey
  //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison

  /** 2 different ways to set security - both calls achieve the same result.
   *  no bonding, no man in the middle protection, secure connections.
   *
   *  These are the default values, only shown here for demonstration.
   */
  NimBLEDevice::setSecurityAuth(true, true, true);
  //NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);

  /** Optional: set the transmit power, default is 3db */
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */

  /** Optional: set any devices you don't want to get advertisments from */
  // NimBLEDevice::addIgnored(NimBLEAddress ("aa:bb:cc:dd:ee:ff"));

  /** create new scan */
  NimBLEScan* pScan = NimBLEDevice::getScan();

  /** create a callback that gets called when advertisers are found */
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());

  /** Set scan interval (how often) and window (how long) in milliseconds */
  pScan->setInterval(22);
  pScan->setWindow(11);

  /** Active scan will gather scan response data from advertisers
   *  but will use more energy from both devices
   */
  pScan->setActiveScan(false);
  /** Start scanning for advertisers for the scan time specified (in seconds) 0 = forever
   *  Optional callback for when scanning stops.
   */
  TFT_color(TFT_YELLOW, TFT_BLACK);
  TFT_print("Scanning");
  RGBLed(CRGB::Yellow);
  pScan->start(scanTime, scanEndedCB);
}

static inline int smin(int x, int y) {return (x < y) ? x : y;}
static inline int smax(int x, int y) {return (x > y) ? x : y;}

void loop ()
{
#if defined(ARDUINO_LILYGO_T_DISPLAY_S3) || defined(ARDUINO_M5Stack_ATOMS3)
  button.tick();
#endif
  /** Loop here until we find a device we want to connect to */
  if (doConnect) {
    TFT_println(advDevice->toString().c_str());
    doConnect = false;

    /** Found a device we want to connect to, do it now */
    if(connectToServer()) {
      DBG_println("Success! we should now be getting notifications!");
      TFT_color(TFT_GREEN, TFT_BLACK);
      TFT_print("Mouse to XAC");
      RGBLed(CRGB::Green);
    } else {
      DBG_println("Failed to connect, starting scan");
      TFT_color(TFT_YELLOW, TFT_BLACK);
      TFT_print("Connect fail\nScanning");
      RGBLed(CRGB::Yellow);
      NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
    }
  }
  else if (Mouse_xfer.available) {
#if 0
    DBG_printf("%s:", __func__);
    for (size_t i = 0; i < Mouse_xfer.report_len; i++) {
      DBG_printf(" %02x", Mouse_xfer.report[i]);
    }
    DBG_println();
#endif
    mouse_values_t ble_mouse;
    extract_mouse_values((const uint8_t *)Mouse_xfer.report, &ble_mouse);
    Mouse_xfer.available = false;
    // DBG_printf("id %d buttons %x, x %d, y %d\n", ble_mouse.report_id,
    //    ble_mouse.buttons, ble_mouse.x, ble_mouse.y);
    Mouse_xfer.joyRpt.buttons_a = ble_mouse.buttons;
    Mouse_xfer.xmin = smin(ble_mouse.x, Mouse_xfer.xmin);
    Mouse_xfer.xmax = smax(ble_mouse.x, Mouse_xfer.xmax);
    Mouse_xfer.ymin = smin(ble_mouse.y, Mouse_xfer.ymin);
    Mouse_xfer.ymax = smax(ble_mouse.y, Mouse_xfer.ymax);
    Mouse_xfer.joyRpt.x = map(ble_mouse.x,
        Mouse_xfer.xmin, Mouse_xfer.xmax, 0, 1023);
    Mouse_xfer.joyRpt.y = map(ble_mouse.y,
        Mouse_xfer.ymin, Mouse_xfer.ymax, 0, 1023);
    FSJoy.write((void *)&Mouse_xfer.joyRpt, sizeof(Mouse_xfer.joyRpt));
  }
  else {
    if ((millis() - Mouse_xfer.last_millis) > 31) {
      // Center x,y if no HID report for 32 ms. Preserve the buttons.
      Mouse_xfer.joyRpt.x = 511;
      Mouse_xfer.joyRpt.y = 511;
      FSJoy.write((void *)&Mouse_xfer.joyRpt, sizeof(Mouse_xfer.joyRpt));
      Mouse_xfer.last_millis = millis();
    }
  }
}

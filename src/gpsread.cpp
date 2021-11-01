#if (HAS_GPS)

#include "globals.h"
#include "gpsread.h"

// Local logging tag
static const char TAG[] = __FILE__;

// we use NMEA ZDA sentence field 1 for time synchronization
// ZDA gives time for preceding pps pulse
// downsight is that it does not have a constant offset
// thus precision is only +/- 1 second

TinyGPSPlus gps;
TinyGPSCustom gpstime(gps, "GPZDA", 1);  // field 1 = UTC time (hhmmss.ss)
TinyGPSCustom gpsday(gps, "GPZDA", 2);   // field 2 = day (01..31)
TinyGPSCustom gpsmonth(gps, "GPZDA", 3); // field 3 = month (01..12)
TinyGPSCustom gpsyear(gps, "GPZDA", 4);  // field 4 = year (4-digit)
static const String ZDA_Request = "$EIGPQ,ZDA*39\r\n";
TaskHandle_t GpsTask;

#ifdef GPS_SERIAL
HardwareSerial GPS_Serial(1); // use UART #1
static uint16_t nmea_txDelay_ms =
    (tx_Ticks(NMEA_FRAME_SIZE, GPS_SERIAL) / portTICK_PERIOD_MS);
#else
static uint16_t nmea_txDelay_ms = 0;
#endif

// initialize and configure GPS
int gps_init(void) {

  if (!gps_config()) {
    ESP_LOGE(TAG, "GPS chip initializiation error");
    return 0;
  }

#ifdef GPS_SERIAL
  ESP_LOGI(TAG, "Opening serial GPS");
  GPS_Serial.begin(GPS_SERIAL);
#elif defined GPS_I2C
  ESP_LOGI(TAG, "Opening I2C GPS");
  Wire.begin(GPS_I2C, 400000); // I2C connect to GPS device with 400 KHz
  Wire.beginTransmission(GPS_ADDR);
  Wire.write(0x00); // dummy write
  if (Wire.endTransmission()) {
    ESP_LOGE(TAG, "Quectel L76 GPS chip not found");
    return 0;
  } else
    ESP_LOGI(TAG, "Quectel L76 GPS chip found");
#endif

  return 1;
} // gps_init()

// detect gps chipset type and configure it with device specific settings
int gps_config() {
  int rslt = 1; // success
#if defined GPS_SERIAL

  /* insert user configuration here, if needed */

#elif defined GPS_I2C

  /* insert user configuration here, if needed */

#endif
  return rslt;
}

// store current GPS location data in struct
void gps_storelocation(gpsStatus_t *gps_store) {
  if (gps.location.isUpdated() && gps.location.isValid() &&
      (gps.location.age() < 1500)) {
    gps_store->latitude = (int32_t)(gps.location.lat() * 1e6);
    gps_store->longitude = (int32_t)(gps.location.lng() * 1e6);
    gps_store->satellites = (uint8_t)gps.satellites.value();
    gps_store->hdop = (uint16_t)gps.hdop.value();
    gps_store->altitude = (int16_t)gps.altitude.meters();
  }
}

bool gps_hasfix() {
  // adapted from source:
  // https://github.com/hottimuc/Lora-TTNMapper-T-Beam/blob/master/fromV08/gps.cpp
  return (gps.location.isValid() && gps.location.age() < 4000 &&
          gps.hdop.isValid() && gps.hdop.value() <= 600 &&
          gps.hdop.age() < 4000 && gps.altitude.isValid() &&
          gps.altitude.age() < 4000);
}

// function to poll current time from GPS data; note: this is costly
time_t get_gpstime(uint16_t *msec) {

  // poll NMEA ZDA sentence
#ifdef GPS_SERIAL
  GPS_Serial.print(ZDA_Request);
  // wait for gps NMEA answer
  // vTaskDelay(tx_Ticks(NMEA_FRAME_SIZE, GPS_SERIAL));
#elif defined GPS_I2C
  Wire.print(ZDA_Request);
#endif

  // did we get a current date & time?
  if (gpstime.isValid()) {

    time_t t = 0;
    tmElements_t tm;
    uint32_t delay_ms =
        gpstime.age() + nmea_txDelay_ms + NMEA_COMPENSATION_FACTOR;
    uint32_t zdatime = atof(gpstime.value());

    // convert time to maketime format and make time
    tm.Second = zdatime % 100;              // second
    tm.Minute = (zdatime / 100) % 100;      // minute
    tm.Hour = zdatime / 10000;              // hour
    tm.Day = atoi(gpsday.value());          // day
    tm.Month = atoi(gpsmonth.value());      // month
    tm.Year = atoi(gpsyear.value()) - 1970; // year offset from 1970
    t = makeTime(tm);

    ESP_LOGD(TAG, "GPS date/time: %s",
             UTC.dateTime(t, "d.M Y H:i:s T").c_str());

    // add protocol delay with millisecond precision
    t += delay_ms / 1000 - 1; // whole seconds
    *msec = delay_ms % 1000;  // fractional seconds

    return t;
  }

  ESP_LOGD(TAG, "no valid GPS time");

  return 0;

} // get_gpstime()

time_t get_gpstime(void) {
  uint16_t msec;
  return get_gpstime(&msec);
}

// GPS serial feed FreeRTos Task
void gps_loop(void *pvParameters) {

  _ASSERT((uint32_t)pvParameters == 1); // FreeRTOS check

  while (1) {

    if (cfg.payloadmask & GPS_DATA) {
#ifdef GPS_SERIAL
      // feed GPS decoder with serial NMEA data from GPS device
      while (GPS_Serial.available()) {
        gps.encode(GPS_Serial.read());
      }
#elif defined GPS_I2C
      Wire.requestFrom(GPS_ADDR, 32); // caution: this is a blocking call
      while (Wire.available()) {
        gps.encode(Wire.read());
        delay(2); // 2ms delay according L76 datasheet
      }
#endif

      // (only) while device time is not set or unsynched, and we have a valid
      // GPS time, we trigger a device time update to poll time from GPS
      if ((timeSource == _unsynced || timeSource == _set) &&
          (gpstime.isUpdated() && gpstime.isValid() && gpstime.age() < 1000)) {
        now();
        calibrateTime();
      }

    } // if

    // show NMEA data in verbose mode, useful only for debugging GPS, very noisy
    // ESP_LOGV(TAG, "GPS NMEA data: passed %u / failed: %u / with fix: %u",
    //         gps.passedChecksum(), gps.failedChecksum(),
    //         gps.sentencesWithFix());

    delay(2); // yield to CPU

  } // end of infinite loop

} // gps_loop()

#endif // HAS_GPS

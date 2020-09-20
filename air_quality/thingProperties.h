#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char THING_ID[] = "7bc24c98-59a3-45ff-bb5a-0eeede40233f";

const char SSID[]     = SECRET_SSID;    // Network SSID (name)
const char PASS[]     = SECRET_PASS;    // Network password (use for WPA, or use as key for WEP)

float pm_10_0_avg;
float pm_2_5_avg;
int air_quality;

void initProperties(){
  ArduinoCloud.setThingId(THING_ID);
  ArduinoCloud.addProperty(pm_10_0_avg, READ, 1 * SECONDS, NULL);
  ArduinoCloud.addProperty(pm_2_5_avg, READ, 1 * SECONDS, NULL);
  ArduinoCloud.addProperty(air_quality, READ, 1 * SECONDS, NULL);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

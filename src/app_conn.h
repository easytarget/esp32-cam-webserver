#ifndef app_conn_h
#define app_conn_h

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <time.h>

#include "parsebytes.h"
#include "app_component.h"
#include "app_cam.h"

#define MAX_KNOWN_STATIONS  5

/* NTP
 *  Uncomment the following to enable the on-board clock
 *  Pick a nearby pool server from: https://www.ntppool.org/zone/@
 *  Set the GMT offset to match your timezone IN SECONDS;
 *    see https://en.wikipedia.org/wiki/List_of_UTC_time_offsets
 *    1hr = 3600 seconds; do the math ;-)
 *    Default is CET (Central European Time), eg GMT + 1hr
 *  The DST offset is usually 1 hour (again, in seconds) if used in your country.
 */
#define NTPSERVER "pool.ntp.org"
#define NTP_GMT_OFFSET 14400
#define NTP_DST_OFFSET 0


struct Station { char ssid[65]; char password[65]; bool dhcp;};

struct StaticIP { IPAddress *ip; IPAddress *netmask; IPAddress *gateway; IPAddress *dns1; IPAddress *dns2; };

class CLAppConn : public CLAppComponent {
    public:
        CLAppConn();

        int loadPrefs();
        int savePrefs();
        int start();
        bool stop() {return WiFi.disconnect();};

        void enableOTA(bool enable = true);
        void handleOTA() {if(otaEnabled) ArduinoOTA.handle();};

        void configMDNS();
        void handleDNSRequest(){if (captivePortal) dnsServer.processNextRequest();};

        void configNTP();

        char * getNTPServer() { return ntpServer;};
        long getGmtOffset_sec() {return gmtOffset_sec;};
        int getDaylightOffset_sec() {return daylightOffset_sec;};

        bool isOTAEnabled() {return otaEnabled;};

        wl_status_t wifiStatus() {return (accesspoint?ap_status:WiFi.status());};

        char * getHTTPUrl(){ return httpURL;};
        char * getStreamUrl(){ return streamURL;};
        int getPort() {return httpPort;};

        char * getApName() {return apName;};

        bool isAccessPoint() {return accesspoint;};
        bool isCaptivePortal() {return captivePortal;};

        char * getLocalTimeStr();
        char * getUpTimeStr();
        void printLocalTime(bool extraData=false);

    private:
        void calcURLs();

        // Known networks structure. Max number of known stations limited for memory considerations
        Station *stationList[MAX_KNOWN_STATIONS]; 
        int stationCount = 0;

        // Static IP structure
        StaticIP staticIP;

        char mdnsName[20];

        bool accesspoint = false;
        char apName[20];
        char apPass[20];
        int ap_channel=1;
        StaticIP apIP;
        bool ap_dhcp=true;
        wl_status_t ap_status = WL_DISCONNECTED;

        // DNS server
        const byte DNS_PORT = 53;
        DNSServer dnsServer;
        bool captivePortal = false;

        // HOST_NAME
        char hostName[64]="";

        // The app and stream URLs (initialized during WiFi setup)
        char httpURL[64];
        char streamURL[64];

        // HTTP Port. Can be overriden during IP setup
        int httpPort = 80;

        // OTA parameters
        bool otaEnabled = false;
        char otaPassword[20] = "";

        // NTP parameters
        char ntpServer[20] = NTPSERVER;
        long  gmtOffset_sec = NTP_GMT_OFFSET;
        int  daylightOffset_sec = NTP_DST_OFFSET;

};

extern CLAppConn AppConn;

#endif
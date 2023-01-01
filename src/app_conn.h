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

/**
 * @brief WiFi connectivity details (SSID/password).
 * 
 */
struct Station { char ssid[64]; char password[64]; };

/**
 * @brief Static IP structure for configuring AP and WiFi parameters
 * 
 */
struct StaticIP { IPAddress *ip; IPAddress *netmask; IPAddress *gateway; IPAddress *dns1; IPAddress *dns2; };

enum StaticIPField {IP, NETMASK, GATEWAY, DNS1, DNS2};

/**
 * @brief Connection Manager
 * This class manages everything related to connectivity of the application: WiFi, OTA etc.
 * 
 */
class CLAppConn : public CLAppComponent {
    public:
        CLAppConn();

        int loadPrefs();
        int savePrefs();
        int start();
        bool stop() {return WiFi.disconnect();};

        void startOTA();
        void handleOTA() {if(otaEnabled) ArduinoOTA.handle();};
        bool isOTAEnabled() {return otaEnabled;};        
        void setOTAEnabled(bool val) {otaEnabled = val;};
        void setOTAPassword(const char * str) {snprintf(otaPassword, sizeof(otaPassword), str);};

        void configMDNS();
        void handleDNSRequest(){if (captivePortal) dnsServer.processNextRequest();};
        char * getMDNSname() {return mdnsName;};
        void setMDNSName(const char * str) {snprintf(mdnsName, sizeof(mdnsName), str);};

        void configNTP();
        char * getNTPServer() { return ntpServer;};
        void setNTPServer(const char * str) {snprintf(ntpServer, sizeof(ntpServer), str);};
        long getGmtOffset_sec() {return gmtOffset_sec;};
        void setGmtOffset_sec(long sec) {gmtOffset_sec = sec;};
        int getDaylightOffset_sec() {return daylightOffset_sec;};
        void setDaylightOffset_sec(int sec) {daylightOffset_sec = sec;};

        char * getSSID() {return ssid;};
        void setSSID(const char * str) {snprintf(ssid, sizeof(ssid), str);};
        void setPassword(const char * str) {snprintf(password, sizeof(password), str);};;

        bool isDHCPEnabled() {return dhcp;};
        void setDHCPEnabled(bool val) {dhcp = val;};
        StaticIP * getStaticIP() {return &staticIP;};
        void setStaticIP(IPAddress ** address, const char * strval);

        wl_status_t wifiStatus() {return (accesspoint?ap_status:WiFi.status());};

        char * getHTTPUrl(){ return httpURL;};
        char * getStreamUrl(){ return streamURL;};
        int getPort() {return httpPort;};
        void setPort(int port) {httpPort = port;};

        char * getApName() {return apName;};
        void setApName(const char * str) {snprintf(apName, sizeof(apName), str);};
        void setApPass(const char * str) {snprintf(apPass, sizeof(apPass), str);};

        bool isAccessPoint() {return accesspoint;};
        void setAccessPoint(bool val) {accesspoint = val;};
        bool getAPDHCP() {return ap_dhcp;};
        void setAPDHCP(bool val) {ap_dhcp = val;};
        StaticIP * getAPIP() {return &apIP;};
        int getAPChannel() {return ap_channel;};
        void setAPChannel(int channel) {ap_channel = channel;};

        bool isCaptivePortal() {return captivePortal;};

        char * getLocalTimeStr() {return localTimeString;};
        char * getUpTimeStr() {return upTimeString;};
        void updateTimeStr();

        void printLocalTime(bool extraData=false);
    
        
    private:
        int getSSIDIndex();
        void calcURLs();
        void readIPFromJSON(jparse_ctx_t * context, IPAddress ** ip_address, char * token);

        // Known networks structure. Max number of known stations limited for memory considerations
        Station *stationList[MAX_KNOWN_STATIONS]; 
        // number of known stations
        int stationCount = 0;

        // Static IP structure
        StaticIP staticIP;

        bool dhcp=false;

        char ssid[64];
        char password[64];

        char mdnsName[20]="";

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
        char ntpServer[20] = "";
        long  gmtOffset_sec;
        int  daylightOffset_sec;

        char localTimeString[50];
        char upTimeString[50];

};

extern CLAppConn AppConn;

#endif
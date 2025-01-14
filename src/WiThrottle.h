/**
 * Based on https://github.com/positron96/withrottle
 * 
 * Also, see JMRI sources, start at java\src\jmri\jmrit\withrottle\DeviceServer.java
 */

#pragma once

#include <WiFi.h>
#include <WiFiServer.h>
#include <ESPmDNS.h>
#include <etl/map.h>
#include <etl/utility.h>

#include "CommandStation.h"


#define WT_DEBUG_

#ifdef WT_DEBUG
#define WT_LOGI(format, ...)  log_printf(ARDUHAL_LOG_FORMAT(I, format), ##__VA_ARGS__)
#else
#define WT_LOGI(...)
#endif


class WiThrottleServer {
public:

    WiThrottleServer(uint16_t port=44444) : port(port), server(port) {}

    void begin() {

        WT_LOGI("WiThrottleServer::begin");

        server.begin();

        //MDNS.begin(hostString);
        MDNS.addService("withrottle","tcp", port);
        //MDNS.setInstanceName("DCC++ Network Interface");

        notifyPowerStatus();

    }

    void end() {
        server.end();
    }

    
    void notifyPowerStatus(int8_t iClient=-1) {
        bool v = CS.getPowerState();
        powerStatus = v ? '1' : '0';
        if(iClient==-1) {
            for (int p=0; p<MAX_CLIENTS; p++) {
                if (clients[p]) wifiPrintln(p, String("PPA")+powerStatus);
            }
        } else if(clients[iClient]) wifiPrintln(iClient, String("PPA")+powerStatus);
    }

    void loop();

private:

    const uint16_t port;

    const static int MAX_CLIENTS = 3;
    const static int MAX_THROTTLES_PER_CLIENT = 6;
    const static int MAX_LOCOS_PER_THROTTLE = 2;

    WiFiServer server;
    WiFiClient clients[MAX_CLIENTS];

    struct ClientData {
        bool connected;
        uint16_t heartbeatTimeout = 30;
        bool heartbeatEnabled;
        uint32_t lastHeartbeat;

        char cmdline[100];
        size_t cmdpos = 0;

        // each client can have up to 6 multi throttles, each MT can have multiple locos (and slots)
        etl::map< char, etl::map<LocoAddress, uint8_t, MAX_LOCOS_PER_THROTTLE>, MAX_THROTTLES_PER_CLIENT> slots;
        uint8_t slot(char thr, LocoAddress addr) { 
            auto it = slots.find(thr);
            if(it == slots.end()) return 0;
            auto iit = it->second.find(addr);
            if(iit == it->second.end() ) return 0;
            return iit->second;
        }

    };

    ClientData clientData[MAX_CLIENTS];

    void processCmd(int iClient);

    char powerStatus = '0';

    void turnPower(char v) {
        CS.setPowerState(v=='1');
        notifyPowerStatus();
    }


    void wifiPrintln(int iClient, String v) {
        clients[iClient].println(v);
        //WT_LOGI("WTTX %s", v.c_str() );
    }
    void wifiPrint(int iClient, String v) {
        clients[iClient].print(v);
        //WT_LOGI("WFTX %s", v.c_str() );
    }

    void clientStart(int iClient) ;

    void clientStop(int iClient);

    void locoAdd(char th, String sLocoAddr, int iClient);

    void locoRelease(char th, String sLocoAddr, int iClient);
    void locoRelease(char th, LocoAddress addr, int iClient);

    void locoAction(char th, String sLocoAddr, String actionVal, int iClient);
    void locoAction(char th, LocoAddress addr, String actionVal, int iClient);

    void checkHeartbeat(int iClient);

    void accessoryToggle(int aAddr, char aStatus, bool namedTurnout);
};
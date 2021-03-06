#ifndef _H_ACNODE
#define _H_ACNODE

// #include <Ethernet.h>
#ifdef  ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "WiredEthernet.h"
#include <esp32-hal-gpio.h> // digitalWrite and friends L	.
#else
#include <ESP8266WiFi.h>
#endif
#include <PubSubClient.h>        // https://github.com/knolleary/
#include <SPI.h>

#include <base64.hpp>
#include <Crypto.h>
#include <SHA256.h>

#include <list>
#include <vector>
#include <algorithm>    // std::find

#include <common-utils.h>
#include <ACBase.h>
#include <LED.h>

extern char * strsepspace(char **p);

// typedef unsigned long beat_t;
// extern beat_t beatCounter;      // My own timestamp - manually kept due to SPI timing issues.

typedef enum {
    ACNODE_ERROR_FATAL,
} acnode_error_t;

typedef enum {
    ACNODE_FATAL,
    ACNODE_ERROR,
    ACNODE_WARN,
    ACNODE_INFO,
    ACNODE_VERBOSE,
    ACNODE_DEBUG
} acnode_loglevel_t;

// #define HAS_MSL
// #define HAS_SIG1
#define HAS_SIG2

typedef enum { PROTO_SIG2, PROTO_SIG1, PROTO_MSL, PROTO_NONE } acnode_proto_t;

class ACLog : public ACBase, public Print { // We should prolly split this in an aACLogger and a logging class
public:
    void addPrintStream(const std::shared_ptr<ACLog> &_handler) {
    	auto it = find(handlers.begin(), handlers.end(), _handler);
        if ( handlers.end() == it) 
        	handlers.push_back(_handler);
    };
    virtual void begin() {
        for (auto it = handlers.begin(); it != handlers.end(); ++it) {
            (*it)->begin();
        }
    };
    virtual void loop() {
        for (auto it = handlers.begin(); it != handlers.end(); ++it) {
            (*it)->loop();
        }
    };
    virtual void stop() {
        for (auto it = handlers.begin(); it != handlers.end(); ++it) {
            (*it)->stop();
        }
    };
    size_t write(byte a) {
        for (auto it = handlers.begin(); it != handlers.end(); ++it) {
            (*it)->write(a);
        }
        return Serial.write(a);
    }
private:
    std::vector<std::shared_ptr<ACLog> > handlers;
};

class ACNode : public ACBase {
public:
    ACNode(const char * machine, const char * ssid, const char * ssid_passwd, acnode_proto_t proto = PROTO_SIG2);
    ACNode(const char * machine, bool wired = true, acnode_proto_t proto = PROTO_SIG2);

    const char * name() { return "ACNode"; }

    void set_mqtt_host(const char *p);
    void set_mqtt_port(uint16_t p);
    void set_mqtt_prefix(const char *p);
    void set_mqtt_log(const char *p);

    void set_moi(const char *p);
    void set_machine(const char *p);
    void set_master(const char *p);

    uint16_t mqtt_port;
    char moi[MAX_NAME];
    char mqtt_server[MAX_HOST];
    char machine[MAX_NAME];
    char master[MAX_NAME];
    char logpath[MAX_NAME];
    char mqtt_topic_prefix[MAX_NAME];
    
    IPAddress localIP() { if (_wired) return ETH.localIP(); else return WiFi.localIP(); };
    
    // Callbacks.
    typedef std::function<void(acnode_error_t)> THandlerFunction_Error;
    ACNode& onError(THandlerFunction_Error fn)
    { _error_callback = fn; return *this; };
    
    typedef std::function<void(void)> THandlerFunction_Connect;
    ACNode& onConnect(THandlerFunction_Connect fn)
	    { _connect_callback = fn; return *this; };
    
    typedef std::function<void(void)> THandlerFunction_Disconnect;
    ACNode& onDisconnect(THandlerFunction_Disconnect fn)
	    { _disconnect_callback = fn; return *this; };
    
    typedef std::function<cmd_result_t(const char *cmd, const char * rest)> THandlerFunction_Command;
    ACNode& onValidatedCmd(THandlerFunction_Command fn)
	    { _command_callback = fn; return *this; };

    typedef std::function<void(const char *msg)> THandlerFunction_SimpleCallback;
    ACNode& onApproval(THandlerFunction_SimpleCallback fn)
	    { _approved_callback = fn; return *this; };
    ACNode& onDenied(THandlerFunction_SimpleCallback fn)
	    { _denied_callback = fn; return *this; };
    
    void loop();
    void begin();
    cmd_result_t handle_cmd(ACRequest * req);
    
    void addHandler(ACBase *handler);
    void addSecurityHandler(ACSecurityHandler *handler);
   
    void request_approval(const char * tag, const char * operation = NULL, const char * target = NULL);

    char * cloak(char *tag);
    
    void set_debugAlive(bool debug);
    bool isConnected(); // ethernet/wifi is up with valid IP.
    bool isUp(); // MQTT et.al also running.
    
    // Public - so it can be called from our fake
    // singleton. Once that it solved it should really
    // become private again.
    //
    void send( const char * payload) { send(NULL, payload, false); };
    void send(const char * topic, const char * payload, bool raw = false);
    
    // This function should be private - but we're calling
    // it from a C callback in the mqtt subsystem.
    //
    void process(const char * topic, const char * payload);
    
private:
    
    bool _debug_alive;
    THandlerFunction_Error _error_callback;
    THandlerFunction_Connect _connect_callback;
    THandlerFunction_Disconnect _disconnect_callback;
    THandlerFunction_SimpleCallback _approved_callback, _denied_callback;
    THandlerFunction_Command _command_callback;

    beat_t _lastSwipe;    
    WiFiClient _espClient;
    PubSubClient _client;
    
    void configureMQTT();
    void reconnectMQTT();
    void mqttLoop();
    void pop();

    const char * state2str(int state);
    
    // We register a bunch of handlers - rather than calling them
    // directly with a flag trigger -- as this allows the linker
    // to not link in unused functionality. Thus making the firmware
    // small enough for the ESP and ENC+Arduino versions.
    //
    std::list<ACBase *> _handlers;
    std::list<ACSecurityHandler*> _security_handlers;
protected:
    const char * _ssid;
    const char * _ssid_passwd;
    bool _wired;
    acnode_proto_t _proto;
};

// Unfortunately - MQTT callbacks cannot yet pass
// a pointer. So we need a 'global' variable; and
// sort of treat this class as a singleton. And
// contain this leakage to just a few functions.
//
extern ACNode *_acnode;
extern ACLog Log;
extern ACLog Debug;

extern void send(const char * topic, const char * payload);

extern const char ACNODE_CAPS[];

#include <MSL.h>
#include <SIG1.h>
#include <SIG2.h>

#include <Beat.h>
#include <OTA.h>

#include <SyslogStream.h>
#include <MqttLogStream.h>
#include <TelnetSerialStream.h>

#endif

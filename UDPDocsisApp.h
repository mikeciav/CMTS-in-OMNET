//UDPDocsisApp.h
//Created By Michael Ciavarella



#ifndef __INET_UDPDOCSISAPP_H
#define __INET_UDPDOCSISAPP_H

#include <vector>

#include "INETDefs.h"

#include "AppBase.h"
#include "UDPSocket.h"


class INET_API UDPDocsisApp : public AppBase
{
  protected:
    enum SelfMsgKinds { START = 1, SEND, STOP };

    UDPSocket socket;
    int localPort, destPort;
    std::vector<IPvXAddress> destAddresses;
    simtime_t startTime;
    simtime_t stopTime;
    cMessage *selfMsg;

    // statistics
    int numSent;
    int numReceived;
    int curTimeSlot;
    int totalSlots;

    static simsignal_t sentPkSignal;
    static simsignal_t rcvdPkSignal;

    // chooses random destination address
    virtual IPvXAddress chooseDestAddr();
    virtual IPvXAddress chooseDestAddr2();
    virtual void sendPacket();
    virtual void processPacket(cPacket *msg);
    virtual void setSocketOptions();

  public:
    UDPDocsisApp();
    ~UDPDocsisApp();
    virtual int getCurTimeSlot();
    virtual int getNumSlots();

  protected:
    virtual int numInitStages() const {return 4;}
    virtual void initAddrArray();
    virtual void initialize(int stage);
    virtual void handleMessageWhenUp(cMessage *msg);
    virtual void finish();

    virtual void processStart();
    virtual void processSend();
    virtual void processStop();

    //AppBase:
    bool startApp(IDoneCallback *doneCallback);
    bool stopApp(IDoneCallback *doneCallback);
    bool crashApp(IDoneCallback *doneCallback);
};

#endif


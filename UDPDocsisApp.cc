//UDPDocsisApp.cc
//Created by Michael Ciavarella

//This is the C++ class for the CM and CMTS. They are distinguished by
//	setting the "isCMTS" attribute in the ini file. TDMA is implemented
//  in this class.
//UDPDocsisApp is a heavily modified version of the UDPEchoApp provided by INET.


#include "UDPDocsisApp.h"

#include "InterfaceTableAccess.h"
#include "IPvXAddressResolver.h"
#include "NodeOperations.h"
#include "UDPControlInfo_m.h"


Define_Module(UDPDocsisApp);

simsignal_t UDPDocsisApp::sentPkSignal = SIMSIGNAL_NULL;
simsignal_t UDPDocsisApp::rcvdPkSignal = SIMSIGNAL_NULL;

bool *addrsChosen;


UDPDocsisApp::UDPDocsisApp()
{
    selfMsg = NULL;
}

UDPDocsisApp::~UDPDocsisApp()
{
    cancelAndDelete(selfMsg);
}

void UDPDocsisApp::initialize(int stage)
{

    srand(time(NULL));

    AppBase::initialize(stage);

    // because of IPvXAddressResolver, we need to wait until interfaces are registered,
    // address auto-assignment takes place etc.
    if (stage == 0)
    {
        numSent = 0;
        numReceived = 0;
        WATCH(numSent);
        WATCH(numReceived);
        sentPkSignal = registerSignal("sentPk");
        rcvdPkSignal = registerSignal("rcvdPk");

        localPort = par("localPort");
        destPort = par("destPort");
        startTime = par("startTime").doubleValue();
        stopTime = par("stopTime").doubleValue();
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            error("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("sendTimer");
        int numCMS = (int)par("numCMs").longValue();
        addrsChosen = new bool[numCMS];
        initAddrArray();
        curTimeSlot = 0;
        totalSlots = 0;


    }
}

void UDPDocsisApp::initAddrArray(){
    for(int i=0;i<par("numCMs").longValue();i++){
        addrsChosen[i] = false;
    }
}
void UDPDocsisApp::finish()
{
    recordScalar("packets sent", numSent);
    recordScalar("packets received", numReceived);
    AppBase::finish();
}

void UDPDocsisApp::setSocketOptions()
{
    int timeToLive = par("timeToLive");
    if (timeToLive != -1)
        socket.setTimeToLive(timeToLive);

    int typeOfService = par("typeOfService");
    if (typeOfService != -1)
        socket.setTypeOfService(typeOfService);

    const char *multicastInterface = par("multicastInterface");
    if (multicastInterface[0])
    {
        IInterfaceTable *ift = InterfaceTableAccess().get(this);
        InterfaceEntry *ie = ift->getInterfaceByName(multicastInterface);
        if (!ie)
            throw cRuntimeError("Wrong multicastInterface setting: no interface named \"%s\"", multicastInterface);
        socket.setMulticastOutputInterface(ie->getInterfaceId());
    }

    bool receiveBroadcast = par("receiveBroadcast");
    if (receiveBroadcast)
        socket.setBroadcast(true);

    bool joinLocalMulticastGroups = par("joinLocalMulticastGroups");
    if (joinLocalMulticastGroups)
        socket.joinLocalMulticastGroups();
}

//Randomly chooses an address from those not assigned a slot
IPvXAddress UDPDocsisApp::chooseDestAddr()
{
    int k;
    do{
	k = rand() % destAddresses.size();
    }
    while (addrsChosen[k]);
	addrsChosen[k] = true;
	return destAddresses[k];
}

IPvXAddress UDPDocsisApp::chooseDestAddr2(){
    int k = intrand(destAddresses.size());
    return destAddresses[k];
}

void UDPDocsisApp::sendPacket()
{
	char msgName[32];
	IPvXAddress destAddr;
	if(par("isCMTS")) //If CMTS, assign slots
	{
	    int i;
	    for(i=0;i<par("numCMs").longValue() && i<255;i++)
	    {
            destAddr = chooseDestAddr();
            sprintf(msgName, ":%lu:%d:", par("numCMs").longValue(),i);

            cPacket *payload = new cPacket(msgName);
            payload->setByteLength(par("messageLength").longValue());
            emit(sentPkSignal, payload);
            socket.sendTo(payload, destAddr, destPort);
            numSent++;
	    }
	    for(;i<par("numCMs").longValue();i++){
	        destAddr = chooseDestAddr();
            sprintf(msgName, ":0:0:");

            cPacket *payload = new cPacket(msgName);
            payload->setByteLength(par("messageLength").longValue());
            emit(sentPkSignal, payload);
            socket.sendTo(payload, destAddr, destPort);
            numSent++;
	    }

	    initAddrArray();

	}
	else //if CM, calculate time to send and schedule packet
	{
		destAddr = chooseDestAddr2();
		sprintf(msgName, "my slot is %i", curTimeSlot);
		EV <<curTimeSlot <<endl;

		cPacket *payload = new cPacket(msgName);
        payload->setByteLength(par("messageLength").longValue());
        emit(sentPkSignal, payload);
        socket.sendTo(payload, destAddr, destPort);
        numSent++;
	}
	

}

void UDPDocsisApp::processStart()
{
    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort);
    setSocketOptions();

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    while ((token = tokenizer.nextToken()) != NULL) {
        IPvXAddress result;
        IPvXAddressResolver().tryResolve(token, result);
        if (result.isUnspecified())
            EV << "cannot resolve destination address: " << token << endl;
        else
            destAddresses.push_back(result);
    }

    if (!destAddresses.empty())
    {
        selfMsg->setKind(SEND);
        processSend();
    }
    else
    {
        if (stopTime >= SIMTIME_ZERO)
        {
            selfMsg->setKind(STOP);
            scheduleAt(stopTime, selfMsg);
        }
    }
}

void UDPDocsisApp::processSend()
{
    sendPacket();
    simtime_t d = simTime() + par("sendInterval").doubleValue();
    if (stopTime < SIMTIME_ZERO || d < stopTime)
    {
        selfMsg->setKind(SEND);
        scheduleAt(d, selfMsg);
    }
    else
    {
        selfMsg->setKind(STOP);
        scheduleAt(stopTime, selfMsg);
    }
}

void UDPDocsisApp::processStop()
{
    socket.close();
}

void UDPDocsisApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        ASSERT(msg == selfMsg);
        switch (selfMsg->getKind()) {
            case START: processStart(); break;
            case SEND:  processSend(); break;
            case STOP:  processStop(); break;
            default: throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    }
    else if (msg->getKind() == UDP_I_DATA)
    {
        // process incoming packet
        processPacket(PK(msg));
    }
    else if (msg->getKind() == UDP_I_ERROR)
    {
        EV << "Ignoring UDP error report\n";
        delete msg;
    }
    else
    {
        error("Unrecognized message (%s)%s", msg->getClassName(), msg->getName());
    }

    if (ev.isGUI())
    {
        char buf[40];
        sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
        getDisplayString().setTagArg("t", 0, buf);
    }
}

void UDPDocsisApp::processPacket(cPacket *pk)
{
    emit(rcvdPkSignal, pk);
    EV << "Received packet: " << UDPSocket::getReceivedPacketInfo(pk) << endl;

    if(par("isCMTS")){
        //par("numCMs") = (par("numCMs").longValue() + 1);
    }
    else{
        std::stringstream payload;
        std::string junk, smodem, sslot;
        int numModems = par("numCMs"), thisSlotNum=0;

        payload << pk;
        EV << payload.str() << endl;
        getline(payload,junk,':');
        getline(payload,smodem,':');
        getline(payload,sslot,':');

        std::istringstream s1(smodem);
        s1 >> numModems;
        std::istringstream s2(sslot);
        s2 >> thisSlotNum;
        if (numModems > 0 && numModems >= thisSlotNum){
            curTimeSlot = thisSlotNum;
            totalSlots = numModems;
        }

        EV << "numModems = " << totalSlots << " thisSlotNum = " << curTimeSlot << endl;
    }

    delete pk;
    numReceived++;
}

bool UDPDocsisApp::startApp(IDoneCallback *doneCallback)
{
    simtime_t start = std::max(startTime, simTime());
    if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime))
    {
        selfMsg->setKind(START);
        scheduleAt(start, selfMsg);
    }
    return true;
}

bool UDPDocsisApp::stopApp(IDoneCallback *doneCallback)
{
    if (selfMsg)
        cancelEvent(selfMsg);
    //TODO if(socket.isOpened()) socket.close();
    return true;
}

bool UDPDocsisApp::crashApp(IDoneCallback *doneCallback)
{
    if (selfMsg)
        cancelEvent(selfMsg);
    return true;
}

//gets the time slot the CM is using
int UDPDocsisApp::getCurTimeSlot(){
    return curTimeSlot;
}

//Gets the total number of slots in the frame
int UDPDocsisApp::getNumSlots(){
    return totalSlots;
}


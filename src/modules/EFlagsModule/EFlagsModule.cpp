#include "EFlagsModule.h"
#include "MeshService.h"
#include "oledHelper.h"

#define DELAY_INTERVAL 8000 // Send a flag change command every 4 sec
#ifndef BASE_STATION
#define BASE_STATION 0 // Flag stations only receive and respond to command. Only base unit sends commands
#endif

uint8_t flagState = 0;
uint16_t carNum = UINT16_MAX;
// NodeNum nodenums[5] = {0x08ab5ecc, 0x08ab5810, 0x08ad6334, 0x08ad6334, 0x08ab61bc};
NodeNum nodenums[1] = {0x08ab5810};
char message[64] = {'\0'};

EFlagsModule::EFlagsModule() : SinglePortModule("EFlagsModule", meshtastic_PortNum_PRIVATE_APP), OSThread("EFlagsModule")
{
    setup();
}

void EFlagsModule::setup()
{
    LOG_INFO("Starting up EFlagsModule...\n");
    setupOLEDDisplay();
    char message[64] = "";

    if (BASE_STATION) // base station - sends flag commands
    {
        firstTime = false;
        flagState = FLAG_NONE;
        // sendFlagCommand(NODENUM_BROADCAST, flagState, UINT16_MAX);
        sprintf(message, "Sent %s to node:%X", FlagStateNames[flagState], flagState);
        displayData(String(message));
        LOG_INFO("Intialized Base Station\n");
    } else {
        LOG_INFO("Initialized flag stn %X", myNodeInfo.my_node_num);
    }
}

ProcessMessage EFlagsModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!BASE_STATION) {
        if (mp.to == myNodeInfo.my_node_num || isBroadcast(mp.to)) {
            setFlagState(mp);
            sendFlagState(mp);
            memset(message, '\0', sizeof(message));
            sprintf(message, "Set Flag: %s\nCar: %i\n", FlagStateNames[myFlagState], myFlagCarNum);
            displayData(String(message));
            // memset(message, '\0', sizeof(message));
            // sprintf(message, "Rcvd state %s\n from: %X\n", FlagStateNames[myFlagState], mp.from);
            // displayData(String(message));
        }
        // auto &p = mp.decoded;
        // // LOG_INFO("Received flags msg from=0x%0x, id=0x%x, msg=%.*s\n", mp.from, mp.id, p.payload.size, p.payload.bytes);
        // uint16_t car_num = (p.payload.bytes[3] << 8) | p.payload.bytes[4];
        // LOG_INFO("Received flag command %s\n", FlagStateNames[p.payload.bytes[1]]);
        // setFlagState(p.payload.bytes[FLAG_MESSAGE_CMD], car_num);

    } else { // BASE_STATION
        auto &flagMsg = mp.decoded.payload.bytes;
        if (flagMsg[FLAG_MESSAGE_TYPE] == MSG_TYPE_STATE) {
            LOG_INFO("Received FlagState from node: %x Flag: %s", mp.from, FlagStateNames[flagMsg[FLAG_MESSAGE_CMD]]);
        }
    }
    return ProcessMessage::STOP;
}

int32_t EFlagsModule::runOnce()
{

    if (BASE_STATION) // base station - sends'\0'} flag commands
    {
        // let's pick a random station
        // NodeNum nodenum = nodenums[(int)random(0, sizeof(nodenums) / sizeof(nodenums[0]) - 1)];
        NodeNum nodenum = nodenums[0];
        // let's pick a random flag state to command
        flagState = random(1, sizeof(FlagState));
        if (flagState == BLACK_FLAG || flagState == MEATBALL_FLAG) {
            // let's pick a random car number
            carNum = int(random(0, 1000));
        } else {
            carNum = UINT16_MAX;
        }
        sendFlagCommand(nodenum, flagState, carNum);
        memset(message, '\0', sizeof(message));
        sprintf(message, "Sent cmd %s\n to Stn: %X\n", FlagStateNames[flagState], nodenum);
        displayData(String(message));
        flagState++;
        if (flagState == sizeof(FlagState)) {
            flagState = 0;
        }
        return DELAY_INTERVAL;
    }

    else // flag station - init led
    {
        if (firstTime) {
            // setupOLEDDisplay();
            displayData("Init: No Flags");
            // setFlagState(FLAG_NONE, UINT16_MAX);
            firstTime = false;
            LOG_INFO("runOnce() has been called firstTime in client");
        }
        // LOG_INFO("runOnce() has been called in station client");
        return 0;
    }
}

void EFlagsModule::sendFlagCommand(NodeNum dest, uint8_t cmd, uint16_t car_num)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = dest;
    // p->want_ack = true;
    p->decoded.payload.bytes[FLAG_MESSAGE_TYPE] = MSG_TYPE_CMD;        // set message type
    p->decoded.payload.bytes[FLAG_MESSAGE_CMD] = cmd;                  // set commanded flag state
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM1] = car_num & 0xFF00; // car_num is is two bytes
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM2] = car_num & 0xFF;
    p->decoded.payload.size = FLAG_MESSAGE_BYTECOUNT;
    service->sendToMesh(p);
    LOG_INFO("Sent command %s to node %X", FlagStateNames[cmd], dest);
}

void EFlagsModule::setFlagState(const meshtastic_MeshPacket &mp)
{
    auto &flagMsg = mp.decoded.payload.bytes;
    myFlagState = flagMsg[FLAG_MESSAGE_CMD];
    myFlagCarNum = (flagMsg[3] << 8) | flagMsg[4];
    LOG_INFO("Set current state %s Car:%i", FlagStateNames[myFlagState], myFlagCarNum);
}

void EFlagsModule::sendFlagState(const meshtastic_MeshPacket &mp)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = mp.from;
    p->decoded.payload.bytes[FLAG_MESSAGE_TYPE] = MSG_TYPE_STATE;           // set message type
    p->decoded.payload.bytes[FLAG_MESSAGE_CMD] = myFlagState;               // set commanded flag state
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM1] = myFlagCarNum & 0xFF00; // car_num is is two bytes
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM2] = myFlagCarNum & 0xFF;
    p->decoded.payload.size = FLAG_MESSAGE_BYTECOUNT;
    service->sendToMesh(p);
    LOG_INFO("Sent current state %s", FlagStateNames[myFlagState]);
}
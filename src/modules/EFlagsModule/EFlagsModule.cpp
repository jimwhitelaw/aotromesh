#include "EFlagsModule.h"
#include "MeshService.h"
#include "oledHelper.h"

#define DELAY_INTERVAL 8000 // Send a flag change command every 4 sec
#ifndef BASE_STATION
#define BASE_STATION 1 // Flag stations only receive and respond to command. Only base unit sends commands
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
            memset(message, '\0', sizeof(message));
            sprintf(message, "Received state cmd\n%s\nCar: %lu\n", FlagStateNames[myFlagState], myFlagCarNum);
            displayData(String(message));
            sendFlagState(mp);
            displayLowerData("Sent state message");
        }

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
        flagState = (int)random(1, STATE_COUNT);
        if (flagState == BLACK_FLAG || flagState == MEATBALL_FLAG) {
            // let's pick a random car number
            carNum = (uint16_t)(random(1, 1000));
        } else {
            carNum = UINT16_MAX;
        }
        sendFlagCommand(nodenum, flagState, carNum);
        memset(message, '\0', sizeof(message));
        sprintf(message, "Sent command \n%s\nCar: %u\nStn: %X\n", FlagStateNames[flagState], carNum, nodenum);
        displayData(String(message));
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
    p->decoded.payload.bytes[FLAG_MESSAGE_TYPE] = MSG_TYPE_CMD;            // set message type
    p->decoded.payload.bytes[FLAG_MESSAGE_CMD] = cmd;                      // set commanded flag state
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM1] = (byte)(car_num >> 8); // car_num is is two bytes
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM2] = (byte)car_num;
    p->decoded.payload.size = FLAG_MESSAGE_BYTECOUNT;
    service->sendToMesh(p);
    LOG_INFO("Sent command: %s Car: %lu to node %X", FlagStateNames[cmd], car_num, dest);
}

void EFlagsModule::setFlagState(const meshtastic_MeshPacket &mp)
{
    auto &flagMsg = mp.decoded.payload.bytes;
    myFlagState = flagMsg[FLAG_MESSAGE_CMD];
    myFlagCarNum = (uint16_t)(flagMsg[FLAG_MESSAGE_CARNUM1] << 8) + flagMsg[FLAG_MESSAGE_CARNUM2];
    LOG_INFO("Set current state %s Car: %lu", FlagStateNames[myFlagState],
             (uint16_t)(flagMsg[FLAG_MESSAGE_CARNUM1] * 256) + flagMsg[FLAG_MESSAGE_CARNUM2]);
}

void EFlagsModule::sendFlagState(const meshtastic_MeshPacket &mp)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = mp.from;
    p->decoded.payload.bytes[FLAG_MESSAGE_TYPE] = MSG_TYPE_STATE;               // set message type
    p->decoded.payload.bytes[FLAG_MESSAGE_CMD] = myFlagState;                   // set commanded flag state
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM1] = (byte)(myFlagCarNum >> 8); // car_num is is two bytes
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM2] = (byte)myFlagCarNum;
    p->decoded.payload.size = FLAG_MESSAGE_BYTECOUNT;
    service->sendToMesh(p);
    LOG_INFO("Sent current state %s", FlagStateNames[myFlagState]);
}
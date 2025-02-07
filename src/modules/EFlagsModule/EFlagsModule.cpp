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
NodeNum nodenums[2] = {0x08ab5810, 0x7c5b178c};
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
    firstTime = false;
    flagState = FLAG_NONE;
    carNum = UINT16_MAX;
    if (BASE_STATION) // base station - sends flag commands
    {
        displayData("Base Station init.");
        LOG_INFO("Base Station initialized.\n");
    } else {
        displayData("Init: No Flags");
        LOG_INFO("Client init.");
    }
}

int32_t EFlagsModule::runOnce()
{
    if (BASE_STATION) {
        carNum = UINT16_MAX;
        NodeNum nodenum = nodenums[(int)random(2)];
        flagState = (int)random(STATE_COUNT);
        switch (flagState) {
        case BLACK_FLAG:
        case MEATBALL_FLAG:
        case FLAG_WARNING:
            carNum = (uint16_t)(random(1000));
            break;
        case RED_FLAG:
        case BLACK_FLAG_ALL:
        case CHECKERED_FLAG:
        case DOUBLE_YELLOW:
            nodenum = NODENUM_BROADCAST;
            break;
        default:
            break;
        }
        sendFlagCommand(nodenum, flagState, carNum);
        memset(message, '\0', sizeof(message));
        sprintf(message, "Sent command \n%s\nCar: %u\nStn: %X\n", FlagStateNames[flagState], carNum, nodenum);
        displayData(String(message));
        return DELAY_INTERVAL;
    } else {
        return 0;
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
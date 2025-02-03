#include "EFlagsModule.h"
#include "MeshService.h"
#include "oledHelper.h"

#define DELAY_INTERVAL 8000 // Send a flag change command every 4 sec
#ifndef BASE_STATION
#define BASE_STATION 1 // Flag stations only receive and respond to command. Only base unit sends commands
#endif

uint8_t flagState = 0;
// NodeNum nodenums[5] = {0x08ab5ecc, 0x08ab5810, 0x08ad6334, 0x08ad6334, 0x08ab61bc};
NodeNum nodenums[1] = {0x08ab5810};

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
        auto &p = mp.decoded;
        // LOG_INFO("Received flags msg from=0x%0x, id=0x%x, msg=%.*s\n", mp.from, mp.id, p.payload.size, p.payload.bytes);
        uint16_t car_num = (p.payload.bytes[3] << 8) | p.payload.bytes[4];
        LOG_INFO("Received flag command %s\n", FlagStateNames[p.payload.bytes[1]]);
        setFlagState(p.payload.bytes[FLAG_MESSAGE_CMD], car_num);
        sendFlagState(p.source, p.payload.bytes[FLAG_MESSAGE_CMD], car_num);
    } else { // BASE_STATION
        auto &p = mp.decoded;
        if (p.payload.bytes[FLAG_MESSAGE_TYPE] == FLAG_STATE) {
            LOG_INFO("Received FlagState from node: %x Flag: %s", mp.from, FlagStateNames[p.payload.bytes[FLAG_MESSAGE_CMD]]);
        }
    }
    return ProcessMessage::STOP;
}

int32_t EFlagsModule::runOnce()
{
    char message[64] = {'\0'};

    if (BASE_STATION) // base station - sends'\0'} flag commands
    {
        // let's pick a random station
        // NodeNum nodenum = nodenums[(int)random(0, sizeof(nodenums) / sizeof(nodenums[0]) - 1)];
        NodeNum nodenum = nodenums[0];
        // let's pick a random flag state to command
        flagState = random(1, sizeof(FlagState));
        sendFlagCommand(nodenum, flagState);
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
    p->want_ack = true;
    p->decoded.payload.bytes[FLAG_MESSAGE_TYPE] = FLAG_COMMAND;        // set message type
    p->decoded.payload.bytes[FLAG_MESSAGE_CMD] = cmd;                  // set commanded flag state
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM1] = car_num & 0xFF00; // car_num is is two bytes
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM2] = car_num & 0xFF;
    p->decoded.payload.size = FLAG_MESSAGE_BYTECOUNT;
    service->sendToMesh(p);
    LOG_INFO("Sent command %s to node %X", FlagStateNames[cmd], dest);
}

void EFlagsModule::setFlagState(uint8_t state, uint16_t car_num)
{
    displayData(FlagStateNames[state]);
}

void EFlagsModule::sendFlagState(NodeNum dest, uint8_t state, uint16_t car_num)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = dest;
    p->decoded.payload.bytes[FLAG_MESSAGE_TYPE] = FLAG_STATE;          // set message type
    p->decoded.payload.bytes[FLAG_MESSAGE_CMD] = state;                // set commanded flag state
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM1] = car_num & 0xFF00; // car_num is is two bytes
    p->decoded.payload.bytes[FLAG_MESSAGE_CARNUM2] = car_num & 0xFF;
    p->decoded.payload.size = FLAG_MESSAGE_BYTECOUNT;
    service->sendToMesh(p);
    LOG_INFO("Sent current state %s", FlagStateNames[state]);
}
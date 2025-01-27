#include "EFlagsModule.h"
#include "MeshService.h"
// #include "configuration.h"
// #include "main.h"
#include "oledHelper.h"

#define DELAY_INTERVAL 8000 // Send a flag change command every 4 sec
#ifndef BASE_STATION
#define BASE_STATION 1 // Flag stations only receive and respond to command. Only base unit sends commands
#endif

// EFlagsModule *eflagsModule;
uint8_t flagState = 0;
NodeNum nodenums[5] = {0x08ab5ecc, 0x08ab5810, 0x08ad6334, 0x08ad6334, 0x08ab61bc};

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
        sendFlagCommand(NODENUM_BROADCAST, flagState, UINT16_MAX);
        sprintf(message, "Sent %s to node:%d", FlagStateNames[flagState], flagState);
        displayData(String(message));
        LOG_INFO("Intialized Base Station\n");
    }
}

ProcessMessage EFlagsModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!BASE_STATION) // Base (non-Flag Station) will ignore messages TODO: change this later? so that stations send an ack when
                       // they have changed state
    {
        auto &p = mp.decoded;
        LOG_INFO("Received flags msg from=0x%0x, id=0x%x, msg=%.*s\n", mp.from, mp.id, p.payload.size, p.payload.bytes);
        uint16_t car_num = (p.payload.bytes[2] << 8) | p.payload.bytes[3];
        LOG_INFO("Received flag command %d\n", p.payload.bytes[0]);
        unsigned long flagStateTimer = micros();
        setFlagState(p.payload.bytes[0], car_num);

        LOG_DEBUG("Flagstate change took %fus\n", micros() - flagStateTimer);
    }
    return ProcessMessage::STOP;
}

int32_t EFlagsModule::runOnce()
{
    char message[24] = {'\0'};

    if (BASE_STATION) // base station - sends'\0'} flag commands
    {
        // if (firstTime) {
        //     setupOLEDDisplay();
        //     firstTime = false;
        // }
        // let's pick a random station
        NodeNum nodenum = nodenums[(int)random(0, sizeof(nodenums) / sizeof(nodenums[0]) - 1)];

        // let's pick a random flag state to command
        flagState = random(1, sizeof(FlagStates));

        sendFlagCommand(nodenum, flagState);
        memset(message, '\0', sizeof(message));
        sprintf(message, "Sent cmd %s\n", FlagStateNames[flagState]);
        displayData(String(message));
        flagState++;
        if (flagState == sizeof(FlagStates)) {
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
        }
        // LOG_INFO("runOnce() has been called in station client");
        return 0;
    }
}

void EFlagsModule::sendFlagCommand(NodeNum dest, uint8_t cmd, uint16_t car_num)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    if (p) {
        p->to = dest;
        p->want_ack = true;
        p->decoded.payload.bytes[0] = FLAG_COMMAND;     // set message type
        p->decoded.payload.bytes[1] = cmd;              // set commanded flag state
        p->decoded.payload.bytes[2] = car_num & 0xFF00; // car_num is is two bytes
        p->decoded.payload.bytes[3] = car_num & 0xFF;
        p->decoded.payload.size = 4;
    } else {
        p->decoded.payload.size = 1;
    }
    service.sendToMesh(p);
    LOG_INFO("Sent command to mesh.\n");
}

void EFlagsModule::setFlagState(uint8_t state, uint16_t car_num)
{
    displayData(FlagStateNames[state]);
}

void EFlagsModule::sendFlagState(NodeNum dest, uint8_t state, uint16_t car_num)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    if (p) {
        p->to = dest;
        p->decoded.payload.bytes[0] = FLAG_STATE;       // set message type
        p->decoded.payload.bytes[1] = state;            // set commanded flag state
        p->decoded.payload.bytes[2] = car_num & 0xFF00; // car_num is is two bytes
        p->decoded.payload.bytes[3] = car_num & 0xFF;
        p->decoded.payload.size = 4;
    } else {
        p->decoded.payload.size = 1;
    }
    service.sendToMesh(p);
}
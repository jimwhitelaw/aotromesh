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
        // String nodeName = nodeDB->getMeshNode(nodenum)->user.long_name;
        memset(message, '\0', sizeof(message));
        sprintf(message, "Sent: %s\nStn: %X", FlagStateNames[flagState], nodenum);
        displayData(String(message));
        return DELAY_INTERVAL;
    } else {
        return 0;
    }
}

ProcessMessage EFlagsModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!BASE_STATION) {
        if (mp.to == nodeDB->getNodeNum() || isBroadcast(mp.to)) {
            switch (mp.decoded.payload.bytes[FLAG_MESSAGE_TYPE]) {
            case MSG_TYPE_CMD:
                setFlagState(mp);
                memset(message, '\0', sizeof(message));
                sprintf(message, "Rec: Car %lu\n%s",
                        (mp.decoded.payload.bytes[FLAG_MESSAGE_CARNUM1] << 8) | mp.decoded.payload.bytes[FLAG_MESSAGE_CARNUM2],
                        FlagStateNames[mp.decoded.payload.bytes[FLAG_MESSAGE_CMD]]);
                displayData(String(message));
                sendFlagState(mp);
                memset(message, '\0', sizeof(message));
                sprintf(message, "Rep: Car %lu\n%s", myStationCarNum, FlagStateNames[myStationState]);
                displayLowerData(message);
                break;
            case MSG_TYPE_UPDATE_REQUEST:
                sendFlagState(mp);
                memset(message, '\0', sizeof(message));
                sprintf(message, "State: Car %lu\n%s", myStationCarNum, FlagStateNames[myStationState]);
                displayLowerData(message);
                break;
            default:
                break;
            }
        } else { // BASE_STATION
            auto &flagMsg = mp.decoded.payload.bytes;
            if (flagMsg[FLAG_MESSAGE_TYPE] == MSG_TYPE_STATE) {
                memset(message, '\0', sizeof(message));
                sprintf(message, "Rec: %s\nStn: %X", FlagStateNames[mp.decoded.payload.bytes[FLAG_MESSAGE_CMD]], mp.from);
                displayLowerData(message);
                LOG_INFO("Received FlagState from node: %x Flag: %s", mp.from, FlagStateNames[flagMsg[FLAG_MESSAGE_CMD]]);
            }
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
    LOG_INFO("Sent command: %s Car: %lu to node: %X", FlagStateNames[cmd], car_num, dest);
}

void EFlagsModule::setFlagState(const meshtastic_MeshPacket &mp)
{
    auto &flagMsg = mp.decoded.payload.bytes;
    myStationState = flagMsg[FLAG_MESSAGE_CMD];
    myStationCarNum = (uint16_t)(flagMsg[FLAG_MESSAGE_CARNUM1] << 8) + flagMsg[FLAG_MESSAGE_CARNUM2];
    LOG_INFO("Set current state %s Car: %lu", FlagStateNames[myStationState],
             (uint16_t)(flagMsg[FLAG_MESSAGE_CARNUM1] << 8) + flagMsg[FLAG_MESSAGE_CARNUM2]);
}

void EFlagsModule::sendFlagState(const meshtastic_MeshPacket &mp)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = mp.from;
    auto &flagMsg = p->decoded.payload.bytes;
    flagMsg[FLAG_MESSAGE_TYPE] = MSG_TYPE_STATE;                  // set message type
    flagMsg[FLAG_MESSAGE_CMD] = myStationState;                   // set commanded flag state
    flagMsg[FLAG_MESSAGE_CARNUM1] = (byte)(myStationCarNum >> 8); // car_num is is two bytes
    flagMsg[FLAG_MESSAGE_CARNUM2] = (byte)myStationCarNum;
    p->decoded.payload.size = FLAG_MESSAGE_BYTECOUNT;
    service->sendToMesh(p);
    LOG_INFO("Sent current state %s", FlagStateNames[myStationState]);
}

/** Default is to send update req as a broadcast if no station is specified.
 *   This may prove to be a dumb idea if it triggers too many responses.
 */
void EFlagsModule::sendUpdateRequest(NodeNum stn)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = stn;
    p->decoded.payload.bytes[FLAG_MESSAGE_TYPE] = MSG_TYPE_UPDATE_REQUEST;
    service->sendToMesh(p);
}
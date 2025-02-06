#include "SinglePortModule.h"
class EFlagsModule : public SinglePortModule, private concurrency::OSThread
{
  public:
    EFlagsModule();
    // EFlagsModule() : SinglePortModule("EFlagsModule", meshtastic_PortNum_PRIVATE_APP), OSThread("EFlagsModule") {};
    // EFlagsModule(const char *_name, meshtastic_PortNum _ourPortNum);
  protected:
    virtual void setup() override;
    virtual int32_t runOnce() override;
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;

  private:
    void sendFlagCommand(NodeNum dest, uint8_t cmd, uint16_t car_num = UINT16_MAX);
    void sendFlagState(const meshtastic_MeshPacket &mp);
    void setFlagState(const meshtastic_MeshPacket &mp);
    bool firstTime = true;
    byte myFlagState = FLAG_NONE;
    uint16_t myFlagCarNum = UINT16_MAX;

    enum FlagMessageType { MSG_TYPE_CMD, MSG_TYPE_STATE };

    enum FlagState {
        FLAG_NONE,
        GREEN_FLAG,
        YELLOW_FLAG,
        BLUE_FLAG,
        WHITE_FLAG,
        BLACK_FLAG,
        RED_FLAG,
        BLACK_FLAG_ALL,
        MEATBALL_FLAG,
        SURFACE_FLAG,
        CHECKERED_FLAG,
        WAVED_YELLOW,
        DOUBLE_YELLOW,
        WAVED_WHITE,
        WHITE_AND_YELLOW,
        OPTION
    };

    String FlagStateNames[16] = {"No Flags",
                                 "Green",
                                 "Yellow",
                                 "Blue",
                                 "White",
                                 "Black",
                                 "Meatball",
                                 "Surface",
                                 "Checkered",
                                 "Waved Yellow",
                                 "Double Yellow",
                                 "Waved White",
                                 "White and Yellow"
                                 "Option"};
    struct FlagStateMessage {
        NodeNum from;
        uint32_t state;
    };

    enum FlagCommandMessageBytes {
        FLAG_MESSAGE_TYPE,
        FLAG_MESSAGE_CMD,
        FLAG_MESSAGE_CARNUM1,
        FLAG_MESSAGE_CARNUM2,
        FLAG_MESSAGE_BYTECOUNT
    };
};
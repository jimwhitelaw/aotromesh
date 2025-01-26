#include "SinglePortModule.h"
class EFlagsModule : public SinglePortModule, private concurrency::OSThread
{
  public:
    EFlagsModule() : SinglePortModule("EFlagsModule", meshtastic_PortNum_PRIVATE_APP), OSThread("EFlagsModule") {}

  protected:
    virtual void setup() override;
    virtual int32_t runOnce() override;
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;

  private:
    void sendFlagCommand(NodeNum dest, uint8_t cmd, uint16_t car_num = UINT16_MAX);
    void sendFlagState(NodeNum dest, uint8_t cmd, uint16_t car_num = UINT16_MAX);
    void setFlagState(uint8_t state, uint16_t car_num);
    bool firstTime = true;

    enum MessageTypes { FLAG_COMMAND, FLAG_STATE };

    enum FlagStates {
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
};

// extern EFlagsModule *eflagsModule;
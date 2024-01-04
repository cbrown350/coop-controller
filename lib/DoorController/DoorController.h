#ifndef DOOR_CONTROLLER_H
#define DOOR_CONTROLLER_H

#include <HasData.h>
#include <Logger.h>

// typedef struct {
//     bool door_open_sensed;
//     bool door_closed_sensed;
//     bool door_closing;
//     bool door_opening;
//     bool door_closing_overload;
// } DoorControllerData;


class DoorController : public HasData<> {
  public:
    inline static constexpr const char * TAG{"DoorController"};
    
    void setupDataVars() {
//        HasData::set("door_open_sensed", false);
//        HasData::set("door_closed_sensed", false);
//        HasData::set("door_closing", false);
//        HasData::set("door_opening", false);
//        HasData::set("door_closing_overload", false);
    }
    explicit DoorController(const std::string &instanceID, uint8_t doorIsOpenIn_b_pin, uint8_t doorIsClosedIn_b_pin,
                            uint8_t doorOpenOut_pin, uint8_t doorCloseOut_pin, uint8_t doorIsClosingOverloadADCIn_pin) : HasData(instanceID) {
        setupDataVars();
        Logger::logv(TAG, "DoorController<T>::DoorController()");
    }

  private:
      inline static int controllerID = 0;
};

#endif
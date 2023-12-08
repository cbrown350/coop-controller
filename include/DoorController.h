#ifndef DOOR_CONTROLLER_H
#define DOOR_CONTROLLER_H

#include "HasData.h"
#include "CoopLogger.h"

// typedef struct {
//     bool door_open_sensed;
//     bool door_closed_sensed;
//     bool door_closing;
//     bool door_opening;
//     bool door_closing_overload;
// } DoorControllerData;


class DoorController : public HasData<> {
  public:
    void setupDataVars() {
        setData("door_open_sensed", false);
        setData("door_closed_sensed", false);
        setData("door_closing", false);
        setData("door_opening", false);
        setData("door_closing_overload", false);
    };
    DoorController() {
        setupDataVars();
        CoopLogger::logi(TAG, "DoorController<T>::DoorController()");
    };

  private:
      inline static int controllerID = 0;
      static constexpr const char * TAG{"DoorController"};
};

#endif
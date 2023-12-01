#ifndef DOOR_CONTROLLER_H
#define DOOR_CONTROLLER_H

#include "HasData.h"

// typedef struct {
//     bool door_open_sensed;
//     bool door_closed_sensed;
//     bool door_closing;
//     bool door_opening;
//     bool door_closing_overload;
// } DoorControllerData;

using DoorControllerData = struct data{
  bool door_open_sensed;
  bool door_closed_sensed;
  bool door_closing;
  bool door_opening;
  bool door_closing_overload;
};
class DoorController;

class DoorController : public HasData<DoorControllerData>{
  public:
    DoorController();
    virtual ~DoorController() = default;

  private:
      inline static int controllerID = 0;
      static constexpr const char * TAG{"DoorController"};
};

#endif
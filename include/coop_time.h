#ifndef COOP_TIME_H
#define COOP_TIME_H

namespace coop_time {

  using data = struct {
    char * timezone;
    char * date;
    char * time;
  };

  data& getData();
}

#endif // COOP_TIME_H
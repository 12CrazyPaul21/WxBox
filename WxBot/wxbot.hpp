#pragma once
#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_WXBOT
    #define WXBOT_PUBLIC __declspec(dllexport)
  #else
    #define WXBOT_PUBLIC __declspec(dllimport)
  #endif
#else
  #ifdef BUILDING_WXBOT
      #define WXBOT_PUBLIC __attribute__ ((visibility ("default")))
  #else
      #define WXBOT_PUBLIC
  #endif
#endif

namespace wxbot {

class WXBOT_PUBLIC Wxbot {

public:
  Wxbot();
  int get_number() const;

private:

  int number;

};

}


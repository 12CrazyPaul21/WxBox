#include <wxbot.hpp>
#include <spdlog/spdlog.h>
#include <frida-gum.h>
#include <lua.hpp>

namespace wxbot {

Wxbot::Wxbot() {
    number = 6;
    spdlog::info("build wxbot");
    gum_init_embedded();
    lua_State* lua = luaL_newstate();
}

int Wxbot::get_number() const {
  return number;
}

}

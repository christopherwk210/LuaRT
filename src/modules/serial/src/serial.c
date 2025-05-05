#include <luart.h>
#include "Port.h"

MODULE_PROPERTIES(serial)
END

MODULE_FUNCTIONS(serial)
END

//----- "serial" module registration function
int __declspec(dllexport) luaopen_serial(lua_State *L)
{
    lua_regmodule(L, serial);
    lua_regobjectmt(L, Port);
    return 1;
}
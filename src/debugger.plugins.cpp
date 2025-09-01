#include "debugger.h"
#include <iostream>
#include <memory>

namespace RoboDBG {
    bool Debugger::initPlugins( )
    {
        if(hProcessGlobal == nullptr) return false;
        this->imports = std::make_unique<Imports>(hProcessGlobal);
        this->freezer = std::make_unique<Freezer>(hProcessGlobal);
        return true;
    }
}

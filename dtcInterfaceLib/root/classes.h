#include <memory> // std::shared_ptr, std::make_shared
#include <cstring> // memcpy

#include "artdaq-core-mu2e/Overlays/DTC_Types.h"
#include "artdaq-core-mu2e/Overlays/DTC_Packets.h"
#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTC_Registers.h"
#include "dtcInterfaceLib/CFOandDTC_Registers.h"
#include "dtcInterfaceLib/mu2edev.h"
#include "dtcInterfaceLib/mu2esim.h"
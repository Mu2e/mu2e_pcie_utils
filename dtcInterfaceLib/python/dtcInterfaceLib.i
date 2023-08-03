%module dtcInterfaceLib
//-----------------------------------------------------------------------------
// SWIG symbols needed to generated wrappers
//-----------------------------------------------------------------------------
%include "std_vector.i"
%include "std_string.i"
%include "stdint.i"
%include "typemaps.i"

%{
#include "../mu2e_driver/mu2e_mmap_ioctl.h"
#include "DTC_Packets.h"
#include "DTC_Registers.h"
#include "DTC.h"
#include "DTCSoftwareCFO.h"
%}
//------------------------------------------------------------------------------
// this is for mu2edev::read_register to return the result as the second number
// in the tuple (the first one is the return code)
//-----------------------------------------------------------------------------
%apply uint32_t *OUTPUT { uint32_t *output };
//-----------------------------------------------------------------------------
// Process symbols in the headers
//-----------------------------------------------------------------------------
%include "../mu2e_driver/mu2e_mmap_ioctl.h"
%include "DTC_Types.h"
%include "DTC_Packets.h"
%include "mu2esim.h"
%include "mu2edev.h"
%include "DTC_Registers.h"
%include "DTC.h"
%include "DTCSoftwareCFO.h"

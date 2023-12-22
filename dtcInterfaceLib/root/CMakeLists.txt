# CMakeLists.txt for event package. It creates a library with dictionary and a main program
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(DtcInterface)

# You need to tell CMake where to find the ROOT installation. This can be done in a number of ways:
#   - ROOT built with classic configure/make use the provided $ROOTSYS/etc/cmake/FindROOT.cmake
#   - ROOT built with CMake. Add in CMAKE_PREFIX_PATH the installation prefix for ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
find_package(ROOT REQUIRED COMPONENTS RIO Net)

#---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
include(${ROOT_USE_FILE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ../ 
                    $ENV{TRACE_INC} 
                    $ENV{ARTDAQ_CORE_MU2E_DIR}/artdaq-core-mu2e/Overlays
                    $ENV{MESSAGEFACILITY_INC}
                    $ENV{CETLIB_INC}
                    $ENV{CETLIB_EXCEPT_INC}
                    )
message(Building ROOT interface: CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# add options, say -D=XXXXX, if needed, after OPTIONS
ROOT_GENERATE_DICTIONARY(G__DTC 
		         DTC.h DTC_Types.h DTCSoftwareCFO.h 
                         LINKDEF DTC_linkdef.h 
                         OPTIONS 
			 )

#---Create a shared library with generated dictionary

add_library(DTC SHARED G__DTC.cxx)

target_link_libraries(DTC ${ROOT_LIBRARIES} DTCInterface)

#---Create  a main program using the library
# add_executable(Main MainEvent.cxx)
# target_link_libraries(Main Event)

# ======================================================================
#  pcie_linux_kernel_module main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/artdaq-demo/ups/setup_for_development
#  buildtool
# ======================================================================


# use cmake 3.16 or later
cmake_minimum_required (VERSION 3.16)

find_package(cetmodules)
set(CET_CETBUILDTOOLS_COMPAT TRUE)
include(Compatibility)
set(CET_WARN_DEPRECATED)

project(pcie_linux_kernel_module VERSION 2.05.18)

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS VIGILANT
 WERROR
 NO_UNDEFINED
 EXTRA_FLAGS -pedantic -Wno-unused-function -Wno-unused-parameter
# EXTRA_C_FLAGS -std=gnu99
 )

string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC )
#if( ${BTYPE_UC} MATCHES "DEBUG" )
#  cet_add_compiler_flags(-fsanitize=address)
#endif()

#cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_ups_product( cetpkgsupport v1_08_03 )
find_ups_product( messagefacility v1_00_00 )
find_ups_product( TRACE v3_03_00 )

# macros for art_dictionary and simple_plugin
include(CetTest)

# source
add_subdirectory(mu2e_driver)
add_subdirectory(mcs)
add_subdirectory(dtcInterfaceLib)
add_subdirectory(cfoInterfaceLib)

# tools
add_subdirectory(tools)

# Documentation (to comment out if not needed DOXYGEN locally)
if( $ENV{OTS_DOXY} MATCHES "DOIT" )
	add_subdirectory(doc)
endif()

cet_cmake_config()


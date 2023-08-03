# Include python
find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_PATH}
                    ../			       
                    $ENV{$TRACE_INC} 
                    $ENV{$MESSAGEFACILITY_INC}
                    $ENV{CETLIB_INC}
                    $ENV{CETLIB_EXCEPT_INC}
		    )	

include(CetCMakeEnv)
cet_cmake_env()

set(CMAKE_SWIG_FLAGS "-v")
set_source_files_properties(dtcInterfaceLib.i PROPERTIES CPLUSPLUS ON)

# Add swig module
swig_add_library(dtcInterfaceLib LANGUAGE python SOURCES dtcInterfaceLib.i)

# swig_link_libraries(dtc ${PROJECT_NAME} ${PYTHON_LIBRARIES})
# ${PROJECT_NAME} translates into mu2e_pcie_utils
swig_link_libraries(dtcInterfaceLib DTCInterface ${PYTHON_LIBRARIES})

# Files to install with Python
set(PYTHON_INSTALL_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dtcInterfaceLib.py
    ${CMAKE_CURRENT_BINARY_DIR}/_dtcInterfaceLib.so
    )

# Configure setup.py and copy to output directory
set(SETUP_PY_IN ${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in)
set(SETUP_PY_OUT ${CMAKE_CURRENT_BINARY_DIR}/setup.py)
configure_file(${SETUP_PY_IN} ${SETUP_PY_OUT})

# Declare install target for python
#install(TARGETS dtcInterfaceLib
#        COMMAND "${PYTHON_EXECUTABLE} setup.py"
#        COMPONENT swig-python)

# Install target to call setup.py
add_custom_target(install-python
        DEPENDS _dtcInterfaceLib
        COMMAND python ${SETUP_PY_OUT} install)

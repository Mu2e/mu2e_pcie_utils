include_directories("${CMAKE_CURRENT_SOURCE_DIR}/basic_driver")

cet_make_exec( mcs SOURCE mcs.cc )
cet_make_exec( devl SOURCE devl.cc )

cet_script( ALWAYS_COPY mcs.sh )

install_headers()
install_source()

add_subdirectory(basic_driver)

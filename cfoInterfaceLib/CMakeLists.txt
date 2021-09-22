cet_make_library(LIBRARY_NAME CFOInterface SOURCE
		CFO_Registers.cpp
		CFO_Compiler.cpp
        LIBRARIES
		DTCInterface
        pthread
)

cet_make_exec(CFORegDump SOURCE cfoRegDump.cc LIBRARIES CFOInterface)
cet_make_exec(mu2eCompiler SOURCE mu2eCompiler.cc LIBRARIES CFOInterface)
cet_make_exec(cfoUtil SOURCE util_main.cc LIBRARIES CFOInterface)

	# install any documentation text files
	FILE(INSTALL Commands.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/doc)

# Install_headers MUST BE FIRST...for some reason
install_headers()
install_source()

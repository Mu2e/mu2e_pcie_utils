set(NODEJS_ADDON_NAME "DTC")

cet_make_library(LIBRARY_NAME DTCInterface SOURCE
            DTC.cpp
            DTCLibTest.cpp
            DTCSoftwareCFO.cpp
			DTC_Registers.cpp
			DTC_Packets.cpp
            DTC_Types.cpp
            mu2edev.cpp
	    mu2esim.cpp
        LIBRARIES
        pthread
		MF_MessageLogger
)

cet_make_exec(mu2eUtil SOURCE util_main.cc Mu2eUtil.cpp LIBRARIES DTCInterface)

cet_make_exec(rocUtil SOURCE dcs_main.cc LIBRARIES DTCInterface)

cet_make_exec(data_file_verifier SOURCE data_file_verifier.cc LIBRARIES MF_MessageLogger DTCInterface)

cet_make_exec(DTC_binary_file_compare SOURCE DTC_binary_file_compare.cc LIBRARIES MF_MessageLogger DTCInterface)

cet_make_exec(mu2eRequestSender SOURCE requestSender.cc LIBRARIES DTCInterface)

cet_make_exec(DTCRegDump SOURCE dtcRegDump.cc LIBRARIES DTCInterface)

cet_make_exec(my_cntl SOURCE my_cntl.cc LIBRARIES DTCInterface)
cet_make_exec(rick_clock_test SOURCE rick_clock_test.cc LIBRARIES DTCInterface)

# Install_headers MUST BE FIRST...for some reason
install_headers()
install_source()

cet_script(updateMu2e.sh Setup_DTC.sh JAConfig.sh)

add_subdirectory(test)

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
        LIBRARIES PUBLIC
        TRACE::MF
        
)

cet_make_exec(NAME mu2eUtil SOURCE util_main.cc Mu2eUtil.cpp LIBRARIES pcie_linux_kernel_module::DTCInterface)

cet_make_exec(NAME rocUtil SOURCE dcs_main.cc LIBRARIES pcie_linux_kernel_module::DTCInterface)

cet_make_exec(NAME data_file_verifier SOURCE data_file_verifier.cc LIBRARIES  pcie_linux_kernel_module::DTCInterface)

cet_make_exec(NAME binary_dump_to_DTC_file SOURCE binary_dump_to_DTC_file.cc LIBRARIES  pcie_linux_kernel_module::DTCInterface)

cet_make_exec(NAME DTC_binary_file_compare SOURCE DTC_binary_file_compare.cc LIBRARIES  pcie_linux_kernel_module::DTCInterface)

cet_make_exec(NAME mu2eRequestSender SOURCE requestSender.cc LIBRARIES pcie_linux_kernel_module::DTCInterface)

cet_make_exec(NAME DTCRegDump SOURCE dtcRegDump.cc LIBRARIES pcie_linux_kernel_module::DTCInterface)

cet_make_exec(NAME my_cntl SOURCE my_cntl.cc LIBRARIES pcie_linux_kernel_module::DTCInterface)
cet_make_exec(NAME rick_clock_test SOURCE rick_clock_test.cc LIBRARIES pcie_linux_kernel_module::DTCInterface)

# Install_headers MUST BE FIRST...for some reason
install_headers()
install_source()

cet_script( ALWAYS_COPY updateMu2e.sh Setup_DTC.sh JAConfig.sh)

add_subdirectory(test)

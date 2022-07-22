cet_make_exec(NAME packetGenerator SOURCE packetGenerator.cc)
cet_make_exec(NAME packetReader SOURCE packetReader.cc)

cet_make_exec(NAME dtcUnitTests SOURCE dtcUnitTests.cc LIBRARIES pcie_linux_kernel_module::DTCInterface)

cet_make_exec(NAME tester SOURCE tester.cc LIBRARIES pcie_linux_kernel_module::DTCInterface)

cet_make_exec(NAME sizeof_buffdesc SOURCE sizeof_buffdesc.cc LIBRARIES pcie_linux_kernel_module::DTCInterface)

# Install_headers MUST BE FIRST...for some reason
install_headers()
install_source()

cet_script(ALWAYS_COPY testdata.bin)

set(COMPONENT_SRCS 
		"src/ethutilities/ArpController.cpp"
		"src/ethutilities/DhcpServer.cpp"
		"src/ethutilities/DnsServer.cpp"
		"src/ethutilities/EthernetQueue.cpp"
		"src/ethutilities/EthernetRouter.cpp"
		"src/ethutilities/EthernetUtilities.cpp"
		"src/ethutilities/TcpServerChannel.cpp"
		"src/ethutilities/UdpChannel.cpp"
		"src/ethutilities/WsChannel.cpp"
		"src/ethutilities/freertos/DnsServer.cpp")
set(COMPONENT_ADD_INCLUDEDIRS 
		"include")
		
set(COMPONENT_REQUIRES jblib-platform-abstract-jbkernel)

register_component()
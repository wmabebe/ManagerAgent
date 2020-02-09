#pragma pack(push,1)
struct BEACON {
	//char endian; 
	int ID; // randomly generated during startup
	int startUpTime; // the time when the client starts              
	int timeInterval; // the time period that this beacon will be repeated 
	int cmdPort;       // the client listens to this port for manager commands
	char IP[4];     // the IP address of this client
};
#pragma pack(pop)
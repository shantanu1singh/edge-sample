// standard libraries
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib> // for exit() and srand and atof
#include <cstring> // for strcpy, strlen
#include <ctime> // for converting time_t to string
#include <map>
#include <vector>
#include <deque>
#include <iterator>
#include <chrono> // for measuring time
#include <thread> // for sleeping
#include <algorithm> // for std::remove

// Azure libraries
#include "iothub.h"
#include "iothub_client.h"
#include "iothub_device_client.h" //Change from _LL
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "parson.h"

// using mqtt
#include "iothubtransportmqtt.h"

// for certs but can prob be removed
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    #include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

using namespace std;

// Struct representing a message sent to iothub with a tracking id
struct IotHubMessage {
    IOTHUB_MESSAGE_HANDLE messageHandle;
    int id;
    IotHubMessage(IOTHUB_MESSAGE_HANDLE imh, int the_id) {
        messageHandle = imh;
        id = the_id;
    }
};

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback) {
    // do some latency calculations
}

int main(int argc, char** argv) {
    // initialize the IotHub SDK
    (void)IoTHub_Init();

    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol = MQTT_Protocol;
    IOTHUB_DEVICE_CLIENT_HANDLE device_handle = IoTHubDeviceClient_CreateFromConnectionString(argv[1], protocol); // pass conn string through cmd line

    #ifdef SET_TRUSTED_CERT_IN_SAMPLES
    IoTHubDeviceClient_SetOption(device_handle, OPTION_TRUSTED_CERT, certificates);
    #endif

    bool trace_on = true;
    IoTHubClient_SetOption(device_handle, OPTION_LOG_TRACE, &trace_on);

    bool urlEncodeOn = true;
    IoTHubDeviceClient_SetOption(device_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);

    int sendCount = 0;
    int total_msgs = 30000;

	while(sendCount < total_msgs) {
        // send message
        char *telemetry_msg = "i am a telemetry msg. usually i look like sensor data in json format";

        IotHubMessage *iothubMsg = new IotHubMessage(IoTHubMessage_CreateFromString(telemetry_msg), sendCount);

        // send the message to IoTHub
        IoTHubDeviceClient_SendEventAsync(device_handle, iothubMsg->messageHandle, send_confirm_callback, iothubMsg); // we use our 

        int intermessage = 10; // send message once every 10ms --> 100 msgs/sec

		// wait before generating data again
        this_thread::sleep_for(chrono::milliseconds(intermessage));
	}

    // Clean up the iothub sdk handle
    IoTHubDeviceClient_Destroy(device_handle);

    // Free all the sdk subsystem
    IoTHub_Deinit();
}
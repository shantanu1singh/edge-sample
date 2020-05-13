// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root
// for full license information.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/time.h>
#include <stdarg.h>

#include "iothub_module_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothubtransportmqtt.h"
#include "iothub.h"
#include "time.h"

#define UPSTREAM_MSG_SIZE	16

struct iot_timestamp {
	uint32_t id;
	uint32_t secs;
	uint32_t usecs;
	uint32_t pad;
};
struct iot_strbuf {
	char buf[UPSTREAM_MSG_SIZE];
};

struct iotmsg {
	union {
		struct iot_timestamp ts;
		struct iot_strbuf strbuf;
	};
	IOTHUB_MESSAGE_HANDLE handle;
};
struct timeval	g_starttime;
int g_startping, g_upstream;

extern char *optarg;
extern int optind, opterr, optopt;

#define TIMEOUT_MS	10
#define LOGBUF_MAX	1024

/*
 * Simple log function to include timestamp
 */
static void
logmsg(const char *fmt, ...)
{
	va_list args;
	struct timeval now;
	time_t secs;
	char *cp, logbuf[LOGBUF_MAX];
	size_t nbytes, len;

	gettimeofday(&now, NULL);
	secs = now.tv_sec;

	cp = logbuf;
	len = LOGBUF_MAX;
	nbytes = strftime(cp, len, "%Y-%m-%d %H:%M:%S.", localtime(&secs));
	cp += nbytes;
	len -= nbytes;

	nbytes = snprintf(cp, len, "%06ld ", now.tv_usec);
	cp += nbytes;
	len -= nbytes;

	va_start(args, fmt);
	nbytes = vsnprintf(cp, len, fmt, args);
	va_end(args);

	cp += nbytes;
	len -= nbytes;
	if (len > 0)
		*cp = '\0';
	else
		logbuf[LOGBUF_MAX - 1] = '\0';
	printf("%s", logbuf);
	fflush(stdout);
}

/*
 * Web timeval subtract
 */
static int
timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating @var{y}. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     @code{tv_usec} is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

/*
 * Once the message has been delivered to the hub, the message handle
 * and the allocated space can be freed
 */
static void
send_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result,
			  void *userContextCallback)
{
	struct iotmsg *msg = (struct iotmsg *)userContextCallback;
	static int startup;

	/*
	 * The hub can take a very long time at startup to acknowledge
	 * sending a message
	 */
	if (startup == 0) {
		struct timeval delta, connect_time;

		gettimeofday(&connect_time, NULL);
		if (timeval_subtract(&delta, &connect_time, &g_starttime) == 0)
			logmsg("Connect delay secs %ld usecs %ld\n", delta.tv_sec, delta.tv_usec);

		startup++;
	}

	logmsg("send confirm callback, message %d with result = %d\n",
		msg->ts.id, result);

	IoTHubMessage_Destroy(msg->handle);
	free(msg);
}

static IOTHUB_CLIENT_RESULT
send_msg(IOTHUB_MODULE_CLIENT_LL_HANDLE client_handle,
	 IOTHUB_MESSAGE_HANDLE msg_handle,
	 const char *queue,
	 void *msg)
{
	return IoTHubModuleClient_LL_SendEventToOutputAsync(client_handle,
							    msg_handle,
							    queue,
							    send_callback,
							    msg);
}

static IOTHUBMESSAGE_DISPOSITION_RESULT
InputQueue1Callback(IOTHUB_MESSAGE_HANDLE recv_msghandle,
		     void *userContextCallback)
{
	IOTHUBMESSAGE_DISPOSITION_RESULT result;
	IOTHUB_MODULE_CLIENT_LL_HANDLE client_handle =
    				(IOTHUB_MODULE_CLIENT_LL_HANDLE) userContextCallback;
	IOTHUB_MESSAGE_RESULT msg_result;
	struct iotmsg *recv_msg;
	unsigned const char *msgbody; size_t recv_size;

	msg_result = IoTHubMessage_GetByteArray(recv_msghandle, &msgbody, &recv_size);
	if (msg_result == IOTHUB_MESSAGE_OK) {
		recv_msg = (struct iotmsg *)msgbody;

		/*
		 * This is pure guesswork. A real solution would ensure that
		 * this looks like a real message before casting and deref'ing
		 */
		if (recv_size >= sizeof(struct iotmsg)) {
			struct timeval now, recv, delta;

			logmsg("received message size %d id %d timestamp: secs %d usecs %d\n",
				recv_size, recv_msg->ts.id, recv_msg->ts.secs,
				recv_msg->ts.usecs);

			gettimeofday(&now, NULL);
			recv.tv_sec = recv_msg->ts.secs;
			recv.tv_usec = recv_msg->ts.usecs;

			if (timeval_subtract(&delta, &now, &recv) == 0) {
				logmsg("msg id %d latency: secs %d usecs %d\n",
					recv_msg->ts.id, delta.tv_sec, delta.tv_usec);
			}
		}
	} else {
		recv_msg = NULL;
		logmsg("Bad received message\n");
	}

	return result;
}

/*
 * Send a ping. The routing table in the json deployment will
 * determine who gets it
 */
static IOTHUB_CLIENT_RESULT
send_ping(IOTHUB_MODULE_CLIENT_LL_HANDLE client_handle)
{
	struct iotmsg	*msg;
	IOTHUB_MESSAGE_HANDLE msg_handle;
	IOTHUB_CLIENT_RESULT send_result = IOTHUB_CLIENT_ERROR;
	static int msgcnt;
	struct timeval now;

	/*
	 * Space will be freed by the send callback
	 */
	msg = malloc(sizeof(*msg));
	if (msg) {

		/*
		 * The message handle will also be freed by the callback and is not sent
		 * to the recipient. The ID is required in the message before we create
		 * the handle via CreateFromByteArray, as that call will encapsulate
		 * everything we give to it.
		 */
		gettimeofday(&now, NULL);

		msg->ts.id = msgcnt++;
		msg->ts.secs = now.tv_sec;
		msg->ts.usecs = now.tv_usec;

		logmsg("sending ping id %d timestamp: secs %d usecs %d\n",
			msg->ts.id, msg->ts.secs, msg->ts.usecs);

        	msg_handle = IoTHubMessage_CreateFromByteArray((unsigned char *)msg,
								sizeof(*msg));
		msg->handle = msg_handle;

        	send_result = send_msg(client_handle, msg_handle, "output1", msg);
	}
	return send_result;
}

static void
iothub_module()
{
	IOTHUB_MODULE_CLIENT_LL_HANDLE client_handle;
	IOTHUB_CLIENT_RESULT res;
	static int startup;
	static int interval_us_val;
	struct timeval now;

    const char* interval_us;

    if ((interval_us = getenv("INTERVAL_IN_US")) != NULL)
    {
        interval_us_val = atoi(interval_us);
    }
	else
	{
		interval_us_val = 1000000;
	}

	logmsg("send ping interval %d\n", interval_us_val);
	int nextping_usecs;

	client_handle = IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol);
	if (client_handle == NULL) {
		logmsg("IoTHubModuleClient_LL_CreateFromEnvironment failed\n");
		goto out;
	}

	res = IoTHubModuleClient_LL_SetInputMessageCallback(client_handle,
							  "input1",
							   InputQueue1Callback,
	  						  (void *)client_handle);
	if (res != IOTHUB_CLIENT_OK) {
		logmsg("Can not setup message callbacks");
		goto out;
	}

	// if (g_startping) {
		gettimeofday(&now, NULL);
		nextping_usecs = (now.tv_sec * 1000000) + now.tv_usec + interval_us_val;

		res = send_ping(client_handle);
		if (res != IOTHUB_CLIENT_OK) {
			logmsg("Can not send ping");
			goto out;
		}
	// }

	logmsg("Waiting for incoming messages, g_startping %d.\n", g_startping);
	for (;;) {

		/*
		 * When using the 'convenience layer' API, a thread is created
		 * that will sleep for 1ms between calls to _LL_DoWork(). If we
		 * emulate that behaviour with 20 modules, then the cpu does
		 * nothing but spin, and little forward progress is made. 100ms
		 * works reasonably well -- it is a tradeoff between latency and
		 * responsiveness, but 1ms is not optimal.
		 */
		ThreadAPI_Sleep(TIMEOUT_MS);
		IoTHubModuleClient_LL_DoWork(client_handle);

		/*
		 * Send out a ping approximately once a
		 * second. This relies on our sleep period being
		 * less than one second.
		 *
		 * Note that on startup, the hub is horribly
		 * slow to send acks
		 */
		// if (g_startping) {
			if (startup == 0) {
                        	gettimeofday(&g_starttime, NULL);
				startup++;
			}
			gettimeofday(&now, NULL);

			int cur_time_us = (now.tv_sec * 1000000) + now.tv_usec;
			if (cur_time_us >= nextping_usecs) {
				nextping_usecs = cur_time_us + interval_us_val;

				res = send_ping(client_handle);
				if (res != IOTHUB_CLIENT_OK) {
					logmsg("Can not send ping");
					goto out;
				}
			}
	}

out:
	if (client_handle)
		IoTHubModuleClient_LL_Destroy(client_handle);

	IoTHub_Deinit();
}

/*
 * To pass args to the program, modify the createOptions in the manifest that
 * spawns the command. For example:
 *	"createOptions": "{ \"Cmd\": [ \"./cperf\", \"-p\" ] }"
 *
 * For a list of the options that one can change see
 * 	https://docs.docker.com/engine/api/v1.30/#operation/ContainerList
 */
int
main(int argc, char *argv[])
{
	int opt;

	/*
	 * Since this is not interactive, we don't display any error message
	 * on incorrect usage
	 */
	if (argc > 1) {
		while ((opt = getopt(argc, argv, "pu")) != -1) {
		switch (opt) {
			case 'p':
				g_startping = 1;
				break;
			case 'u':
				g_upstream = 1;
				break;
			default:
				break;
			}
		}
	}

	if (IoTHub_Init() == 0)
		iothub_module();
	else
		logmsg("Failed to initialize the platform.\n");

	return 0;
}


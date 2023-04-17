
/*
 * udp_server.c
 *
 *  Created on: Apr 16, 2023
 *      Author: Хана
*/

#include "simple_http_server.h"
#include "main.h"
#include "lwip.h"
#include "sockets.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

#if defined(USE_HTTP_SERVER) || !defined(USE_UDP_SERVER)
#define PORTNUM 80UL
#else
#ifndef PORTNUM
#define PORTNUM 5678UL
#endif
#endif

#if (USE_UDP_SERVER_PRINTF == 1)
#include <stdio.h>
#define UDP_SERVER_PRINTF(...) do { printf("[udp_server.c: %s: %d]: ",__func__, __LINE__);printf(__VA_ARGS__); } while (0)
#else
#define UDP_SERVER_PRINTF(...)
#endif

static struct sockaddr_in serv_addr, client_addr;
static int socket_fd;
static uint16_t nport;

void ServerThread(void const * argument);

const osThreadDef_t os_thread_def_server1 = { "Server1", ServerThread, osPriorityNormal, 0, 1024 + 512};

const osThreadDef_t * Servers = &os_thread_def_server1;

osThreadId ThreadId;
#include "cmsis_os.h"

static int udpServerInit(void)
{
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd == -1) {
		return -1;
	}

	nport = PORTNUM;
	nport = htons((uint16_t)nport);

	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = nport;

	if(bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1) {
		close(socket_fd);
		return -1;
	}

	return 0;
}

void StartUdpServerTask(void const * argument)
{
	osDelay(10000);// wait 5 sec to init lwip stack

	if(udpServerInit() < 0) {
		return;
	}

	bzero(&client_addr, sizeof(client_addr));

	if (ThreadId != NULL) {
	  osThreadTerminate(ThreadId);

	  ThreadId = NULL;
	}
	ThreadId = osThreadCreate (Servers, &socket_fd);

	for(;;)
	{
	}
}

void ServerThread(void const * argument)
{
	int nbytes;
	char buffer[80];

	Led_TypeDef led[4] = {LED3, LED4, LED5, LED6};
	bool led_status[4] = {false, false, false, false};

	while(1)
	{
		memset(buffer, 0, sizeof(buffer));
		int addr_len = sizeof(client_addr);

		nbytes = recvfrom(socket_fd, buffer, sizeof(buffer), 0,
						  (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);

		if (strncmp(buffer, "exit", strlen("exit")) == 0)
		{
			sendto(socket_fd, "goodby!\n", strlen("goodby!\n"), 0, (struct sockaddr *)&client_addr, addr_len);
			break;
		}
		if (strncmp(buffer, "sversion", strlen("sversion")) == 0)	// get a server version
		{
			sendto(socket_fd, "udp_srv_hana_khalil_18042023\nOK\n", strlen("udp_srv_hana_khalil_18042023\nOK\n"), 0,
					(struct sockaddr *)&client_addr, addr_len);
			continue;
		}
		if (strstr(buffer, "led") != 0)
		{
			int led_num = buffer[3] - 48;
			if (led_num < 3 || led_num > 6)
			{
				sendto(socket_fd, "ERROR\n", strlen("ERROR\n"), 0,
						(struct sockaddr *)&client_addr, addr_len);
				continue;
			}

			led_num -= 3;
			Led_TypeDef led[4] = {LED3, LED4, LED5, LED6};

			if (strstr(buffer, "on") != 0)
			{
				led_status[led_num] = true;
				BSP_LED_On(led[led_num]);
				sendto(socket_fd, "OK\n", strlen("OK\n"), 0,
						(struct sockaddr *)&client_addr, addr_len);
				continue;
			}
			if (strstr(buffer, "off") != 0)
			{
				BSP_LED_Off(led[led_num]);
				led_status[led_num] = false;
				sendto(socket_fd, "OK\n", strlen("OK\n"), 0,
						(struct sockaddr *)&client_addr, addr_len);
				continue;
			}
			if (strstr(buffer, "toggle") != 0)
			{
				led_status[led_num] ^= 1;
				BSP_LED_Toggle(led[led_num]);
				sendto(socket_fd, "OK\n", strlen("OK\n"), 0,
						(struct sockaddr *)&client_addr, addr_len);
				continue;
			}
			if (strstr(buffer, "status") != 0)
			{
				sprintf(buffer, "led%d status\nLED%d %s\nOK\n", led_num+3, led_num+3, (led_status[led_num]? "ON" : "OFF"));
				sendto(socket_fd, buffer, strlen(buffer), 0,
						(struct sockaddr *)&client_addr, addr_len);
				continue;
			}
			sendto(socket_fd, "ERROR\n", strlen("ERROR\n"), 0,
					(struct sockaddr *)&client_addr, addr_len);
			continue;
		}
		sendto(socket_fd, "unknown_operation\nERROR\n", strlen("unknown_operation\nERROR\n"), 0,
				(struct sockaddr *)&client_addr, addr_len);
	}

	close(socket_fd);

	osThreadId id = osThreadGetId ();

	if (ThreadId == id) {
		ThreadId = NULL;
	  }
	osThreadTerminate(NULL);
}

/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if PRJCONF_NET_EN

#include "cmd_util.h"
#include "cmd_ping.h"
#include "lwip/netdb.h"
#include "net/ping/ping.h"

//struct ping_data pdata;
static OS_Thread_t g_ping_thread;
#define PING_THREAD_STACK_SIZE		(1 * 1024)
#define PING_THREAD_EXIT OS_ThreadDelete

void ping_run(void *arg)
{
    struct ping_data *data = (struct ping_data *)arg;
    ping(data);
	if (data)
		free(data);
    PING_THREAD_EXIT(&g_ping_thread);
}

int ping_start(struct ping_data *data)
{
	struct ping_data *ping_arg = NULL;
    if (OS_ThreadIsValid(&g_ping_thread)) {
            CMD_ERR("PING task is running\r\n");
            return -1;
    }

	if (data) {
		ping_arg = malloc(sizeof(struct ping_data));
		if (!ping_arg) {
			CMD_ERR("ping arg malloc err\r\n");
			return -1;
		}
		memcpy(ping_arg, data, sizeof(struct ping_data));
	}

    if (OS_ThreadCreate(&g_ping_thread,
                            "",
                            ping_run,
                            (void *)ping_arg,
                            OS_THREAD_PRIO_APP,
                            PING_THREAD_STACK_SIZE) != OS_OK) {
            CMD_ERR("PING task create failed\r\n");
			if (ping_arg)
				free(ping_arg);
            return -1;
    }

    return 0;
}

int ping_get_host_by_name(char *name, unsigned int *address)
{
	struct hostent *host_entry;

	host_entry = gethostbyname(name);
        if(host_entry) {
                *(address) = *((u_long*)host_entry->h_addr_list[0]);
		return 0; // OK
	} else {
		return 1; // Error
	}
}

enum cmd_status cmd_ping_exec(char *cmd)
{
        int argc;
        char *argv[4];
        struct ping_data pdata;
        memset((void*) &pdata, 0, sizeof(pdata));

        argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
        if (argc < 1 || argc > 3) {
                CMD_ERR("invalid ping cmd, argc %d\r\n", argc);
                return CMD_STATUS_INVALID_ARG;
        }
        unsigned int address = 0;

        if (ping_get_host_by_name(argv[0], &address) != 0) {
                CMD_ERR("invalid ping host.\r\n");
                return CMD_STATUS_INVALID_ARG;
        }

#ifdef __CONFIG_LWIP_V1
        ip4_addr_set_u32(&pdata.sin_addr, address);
#elif LWIP_IPV4 /* now only for IPv4 */
        ip_addr_set_ip4_u32(&pdata.sin_addr, address);
#else
        #error "IPv4 not support!"
#endif

        if (argc > 1)
			pdata.count = atoi(argv[1]);
        else
			pdata.count = 3;
		if (argc > 2) {
			pdata.data_long = atoi(argv[2]);
			if (pdata.data_long > 65500)
				pdata.data_long = 65500;
		} else {
			pdata.data_long = 0xffff;
		}

        if (ping_start(&pdata) == 0)
                return CMD_STATUS_OK;
        else
                return CMD_STATUS_FAIL;
}

#endif /* PRJCONF_NET_EN */

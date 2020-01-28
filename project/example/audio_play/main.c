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

#include <stdio.h>
#include "common/framework/platform_init.h"
#include "common/framework/net_ctrl.h"

extern int audio_play_start();


static void net_cb(uint32_t event, uint32_t data, void *arg)
{
	uint16_t type = EVENT_SUBTYPE(event);

	switch (type) {
		case NET_CTRL_MSG_NETWORK_UP:
		audio_play_start();
			// if (!OS_ThreadIsValid(&httpc_demo_thread)) {
			// 	OS_ThreadCreate(&httpc_demo_thread,
			// 						"httpc_demo_thread",
			// 						httpc_demo_fun,
			// 						(void *)NULL,
			// 						OS_THREAD_PRIO_APP,
			// 						HTTPC_DEMO_THREAD_STACK_SIZE);
			//}
			break;

		case NET_CTRL_MSG_NETWORK_DOWN:
			break;

		default:
			break;
	}

}


int main(void)
{
	observer_base *net_ob;

	platform_init();

	printf("audio play start.\r\n");

	audio_play_start();

	// wlan_sta_set("realthread", strlen("realthread"),"02158995663");
	// //wlan_sta_set("realthread", strlen("KHUA_0808"),"20070707");
	// wlan_sta_enable();

	// 	/* create an observer to monitor the net work state */
	// net_ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK,
	// 								     NET_CTRL_MSG_ALL,
	// 								     net_cb,
	// 								     NULL);
	// if(net_ob == NULL)
	// 	return -1;

	// if(sys_ctrl_attach(net_ob) != 0)
	// 	return -1;

	// audio_play_start();

	return 0;
}

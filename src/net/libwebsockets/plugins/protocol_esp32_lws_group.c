/*
 * ESP32 Group protocol handler
 *
 * Copyright (C) 2017 Andy Green <andy@warmcat.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation:
 *  version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA*
 *
 */
#include <string.h>
#include <nvs.h>
#include <esp_ota_ops.h>

typedef enum {
	GROUP_STATE_NONE,
	GROUP_STATE_INITIAL,
	GROUP_STATE_MEMBERS,
	GROUP_STATE_FINAL
} group_state;

struct per_session_data__lws_group {
	struct per_session_data__lws_group *next;
	group_state group_state;

	struct lws_group_member *member;

	unsigned char subsequent:1;
	unsigned char changed_partway:1;
};

struct per_vhost_data__lws_group {
	struct per_session_data__lws_group *live_pss_list;
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;
	int count_live_pss;
};

static void render_ip4(char *dest, int len, uint8_t *ip)
{
	snprintf(dest, len, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}



static int
callback_lws_group(struct lws *wsi, enum lws_callback_reasons reason,
		   void *user, void *in, size_t len)
{
	struct per_session_data__lws_group *pss =
			(struct per_session_data__lws_group *)user;
	struct per_vhost_data__lws_group *vhd =
			(struct per_vhost_data__lws_group *)
			lws_protocol_vh_priv_get(lws_get_vhost(wsi),
					lws_get_protocol(wsi));
	char buffer[1024 + LWS_PRE], ipv4[20];
	char *start = buffer + LWS_PRE - 1, *p = start,
		      *end = buffer + sizeof(buffer) - 1;
	struct lws_group_member *mbr;
	int n, m;

	switch (reason) {

	case LWS_CALLBACK_PROTOCOL_INIT:
		vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
				lws_get_protocol(wsi),
				sizeof(struct per_vhost_data__lws_group));
		vhd->context = lws_get_context(wsi);
		vhd->protocol = lws_get_protocol(wsi);
		vhd->vhost = lws_get_vhost(wsi);
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		if (!vhd)
			break;
		break;

	case LWS_CALLBACK_ESTABLISHED:
		lwsl_notice("%s: ESTABLISHED\r\n", __func__);
		vhd->count_live_pss++;
		pss->next = vhd->live_pss_list;
		vhd->live_pss_list = pss;
		pss->group_state = GROUP_STATE_INITIAL;
		lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:

		switch (pss->group_state) {

		case GROUP_STATE_NONE:
			/* fallthru */

		case GROUP_STATE_INITIAL:

			p += snprintf((char *)p, end - p,
				      "{\r\n"
				      " \"group\":\"%s\","
				      " \"members\":[\r\n",
				      lws_esp32.group);

			n = LWS_WRITE_TEXT | LWS_WRITE_NO_FIN;
			pss->group_state = GROUP_STATE_MEMBERS;
			pss->subsequent = 0;
			pss->changed_partway = 0;
			pss->member = lws_esp32.first;
			break;

		case GROUP_STATE_MEMBERS:

			/* confirm pss->member is still in the list... */

			mbr = lws_esp32.first;
			while (mbr && mbr != pss->member)
				mbr = mbr->next;

			if (!mbr) { /* no longer exists... */
				if (lws_esp32.first || pss->member)
					pss->changed_partway = 1;
				*p++ = ' ';
				pss->member = NULL;

				/*
				 * finish the list where we got to, then
				 * immediately reissue it
				 */
			}
			
			while (end - p > 100 && pss->member) {

				if (pss->subsequent)
					*p++ = ',';

				pss->subsequent = 1;
				render_ip4(ipv4, sizeof(ipv4), (uint8_t *)&pss->member->addr);

				p += snprintf((char *)p, end - p,
					      " {\r\n"
					      "  \"mac\":\"%s\",\r\n"
					      "  \"model\":\"%s\",\r\n"
					      "  \"role\":\"%s\",\r\n"
					      "  \"width\":\"%d\",\r\n"
					      "  \"height\":\"%d\",\r\n"
					      "  \"ipv4\":\"%s\"\r\n"
					      " }\r\n",
					        pss->member->mac,
					      	pss->member->model,
						pss->member->role,
						pss->member->width,
						pss->member->height,
						ipv4
					      );
				pss->member = pss->member->next;
			}

			lwsl_notice("%s\r\n", p);

			n = LWS_WRITE_CONTINUATION | LWS_WRITE_NO_FIN;
			if (!pss->member)
				pss->group_state = GROUP_STATE_FINAL;
			break;

		case GROUP_STATE_FINAL:
			n = LWS_WRITE_CONTINUATION;
			p += sprintf((char *)p, "],\r\n \"discard\":\"%d\"}\r\n",
					pss->changed_partway);
			if (pss->changed_partway)
				pss->group_state = GROUP_STATE_INITIAL;
			else
				pss->group_state = GROUP_STATE_NONE;
			break;
		default:
			return 0;
		}
//		lwsl_notice("issue: %d (%d)\r\n", p - start, n);
		m = lws_write(wsi, (unsigned char *)start, p - start, n);
		if (m < 0) {
			lwsl_err("ERROR %d writing to di socket\r\n", m);
			return -1;
		}

		if (pss->group_state != GROUP_STATE_NONE)
			lws_callback_on_writable(wsi);

		break;

	case LWS_CALLBACK_RECEIVE:
		{
			break;
		}

	case LWS_CALLBACK_CLOSED:
		{
			struct per_session_data__lws_group **p = &vhd->live_pss_list;

			while (*p) {
				if ((*p) == pss) {
					*p = pss->next;
					continue;
				}

				p = &((*p)->next);
			}

			vhd->count_live_pss--;
		}
		break;

	case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
		/* called when our wsi user_space is going to be destroyed */
		break;

	default:
		break;
	}

	return 0;
}

#define LWS_PLUGIN_PROTOCOL_LWS_GROUP \
	{ \
		"lws-group", \
		callback_lws_group, \
		sizeof(struct per_session_data__lws_group), \
		1024, 0, NULL, 900 \
	}


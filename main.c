#include "generated/index.html.h"
#include "generated/main.js.h"
#include "generated/pdf.js.h"
#include "generated/style.css.h"

#include <string.h>

#include "external/dhcpserver.h"
#include "external/dnsserver.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#define TCP_PORT 80
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nContent-Length: %d\ncharset=utf-8\nConnection: close\n\n"
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s/index.html\n\n"

typedef struct TCP_SERVER_T_ {
	struct tcp_pcb* server_pcb;
	bool complete;
	ip_addr_t gw;
} TCP_SERVER_T;

typedef struct TCP_CONNECT_STATE_T_ {
	struct tcp_pcb* pcb;
	int sent_len;
	char headers[16384];
	char result[16384];
	int header_len;
	int result_len;
	ip_addr_t* gw;
} TCP_CONNECT_STATE_T;

static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T* con_state, struct tcp_pcb* client_pcb, err_t close_err) {
	if (client_pcb) {
		assert(con_state && con_state->pcb == client_pcb);
		tcp_arg(client_pcb, NULL);
		tcp_poll(client_pcb, NULL, 0);
		tcp_sent(client_pcb, NULL);
		tcp_recv(client_pcb, NULL);
		tcp_err(client_pcb, NULL);
		err_t err = tcp_close(client_pcb);
		if (err != ERR_OK) {
			printf("close failed %d, calling abort\n", err);
			tcp_abort(client_pcb);
			close_err = ERR_ABRT;
		}
		if (con_state) {
			free(con_state);
		}
	}
	return close_err;
}

static void tcp_server_close(TCP_SERVER_T* state) {
	if (state->server_pcb) {
		tcp_arg(state->server_pcb, NULL);
		tcp_close(state->server_pcb);
		state->server_pcb = NULL;
	}
}

static err_t tcp_server_sent(void* arg, struct tcp_pcb* pcb, u16_t len) {
	TCP_CONNECT_STATE_T* con_state = (TCP_CONNECT_STATE_T*)arg;
	printf("tcp_server_sent %u\n", len);
	con_state->sent_len += len;
	if (con_state->sent_len >= con_state->header_len + con_state->result_len) {
		printf("all done\n");
		return tcp_close_client_connection(con_state, pcb, ERR_OK);
	}
	return ERR_OK;
}

err_t tcp_server_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err) {
	TCP_CONNECT_STATE_T* con_state = (TCP_CONNECT_STATE_T*)arg;
	if (!p) {
		printf("connection closed\n");
		return tcp_close_client_connection(con_state, pcb, ERR_OK);
	}
	assert(con_state && con_state->pcb == pcb);
	if (p->tot_len > 0) {
		printf("tcp_server_recv %d err %d\n", p->tot_len, err);
#if 0
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            printf("in: %.*s\n", q->len, q->payload);
        }
#endif
		// Copy the request into the buffer
		pbuf_copy_partial(p, con_state->headers, p->tot_len > sizeof(con_state->headers) - 1 ? sizeof(con_state->headers) - 1 : p->tot_len, 0);

		// Handle GET request
		if (strncmp("GET", con_state->headers, 3) == 0) {
			char* request = con_state->headers + 4; // + space
			char* params	= strchr(request, ' ');
			if (params) {
				if (*params) {
					char* space = strchr(request, ' ');
					*params++		= 0;
					if (space) {
						*space = 0;
					}
				} else {
					params = NULL;
				}
			}

			// Generate content
			if (strncmp(request, "/index.html", sizeof("/index.html") - 1) == 0)
				con_state->result_len = snprintf(con_state->result, sizeof(con_state->result), FRONTEND_INDEX_HTML);
			if (strncmp(request, "/main.js", sizeof("/main.js") - 1) == 0)
				con_state->result_len = snprintf(con_state->result, sizeof(con_state->result), FRONTEND_MAIN_JS);
			if (strncmp(request, "/pdf.js", sizeof("/pdf.js") - 1) == 0)
				con_state->result_len = snprintf(con_state->result, sizeof(con_state->result), FRONTEND_PDF_JS);
			if (strncmp(request, "/style.css", sizeof("/style.css") - 1) == 0)
				con_state->result_len = snprintf(con_state->result, sizeof(con_state->result), FRONTEND_STYLE_CSS);

			printf("Request: %s\n", request);
			printf("Result: %d\n", con_state->result_len);

			// Check we had enough buffer space
			if (con_state->result_len > sizeof(con_state->result) - 1) {
				printf("Too much result data %d\n", con_state->result_len);
				return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
			}

			// Generate web page
			if (con_state->result_len > 0) {
				con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADERS, 200, con_state->result_len);
				if (con_state->header_len > sizeof(con_state->headers) - 1) {
					printf("Too much header data %d\n", con_state->header_len);
					return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
				}
			} else {
				// Send redirect
				con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_REDIRECT, ipaddr_ntoa(con_state->gw));
				printf("Sending redirect %s", con_state->headers);
			}

			// Send the headers to the client
			con_state->sent_len = 0;
			err_t err						= tcp_write(pcb, con_state->headers, con_state->header_len, 0);
			if (err != ERR_OK) {
				printf("failed to write header data %d\n", err);
				printf("\t\t\tcon_state->result_len = %d\n", con_state->result_len);
				return tcp_close_client_connection(con_state, pcb, err);
			}

			// Send the body to the client
			if (con_state->result_len) {
				err = tcp_write(pcb, con_state->result, con_state->result_len, 0);
				if (err != ERR_OK) {
					printf("failed to write result data %d\n", err);
					return tcp_close_client_connection(con_state, pcb, err);
				}
			}
		}
		tcp_recved(pcb, p->tot_len);
	}
	pbuf_free(p);
	return ERR_OK;
}

static err_t tcp_server_poll(void* arg, struct tcp_pcb* pcb) {
	TCP_CONNECT_STATE_T* con_state = (TCP_CONNECT_STATE_T*)arg;
	printf("tcp_server_poll_fn\n");
	return tcp_close_client_connection(con_state, pcb, ERR_OK); // Just disconnect clent?
}

static void tcp_server_err(void* arg, err_t err) {
	TCP_CONNECT_STATE_T* con_state = (TCP_CONNECT_STATE_T*)arg;
	if (err != ERR_ABRT) {
		printf("tcp_client_err_fn %d\n", err);
		tcp_close_client_connection(con_state, con_state->pcb, err);
	}
}

static err_t tcp_server_accept(void* arg, struct tcp_pcb* client_pcb, err_t err) {
	TCP_SERVER_T* state = (TCP_SERVER_T*)arg;
	if (err != ERR_OK || client_pcb == NULL) {
		printf("failure in accept\n");
		return ERR_VAL;
	}
	printf("client connected\n");

	// Create the state for the connection
	TCP_CONNECT_STATE_T* con_state = calloc(1, sizeof(TCP_CONNECT_STATE_T));
	if (!con_state) {
		printf("failed to allocate connect state\n");
		return ERR_MEM;
	}
	con_state->pcb = client_pcb; // for checking
	con_state->gw	 = &state->gw;

	// setup connection to client
	tcp_arg(client_pcb, con_state);
	tcp_sent(client_pcb, tcp_server_sent);
	tcp_recv(client_pcb, tcp_server_recv);
	tcp_poll(client_pcb, tcp_server_poll, 10);
	tcp_err(client_pcb, tcp_server_err);

	return ERR_OK;
}

static bool tcp_server_open(void* arg) {
	TCP_SERVER_T* state = (TCP_SERVER_T*)arg;
	printf("starting server on port %u\n", TCP_PORT);

	struct tcp_pcb* pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		printf("failed to create pcb\n");
		return false;
	}

	err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
	if (err) {
		printf("failed to bind to port %d\n");
		return false;
	}

	state->server_pcb = tcp_listen_with_backlog(pcb, 1);
	if (!state->server_pcb) {
		printf("failed to listen\n");
		if (pcb) {
			tcp_close(pcb);
		}
		return false;
	}

	tcp_arg(state->server_pcb, state);
	tcp_accept(state->server_pcb, tcp_server_accept);

	return true;
}

int main() {
	stdio_init_all();

	TCP_SERVER_T* state = calloc(1, sizeof(TCP_SERVER_T));
	if (!state) {
		printf("failed to allocate state\n");
		return 1;
	}

	if (cyw43_arch_init()) {
		printf("failed to initialise\n");
		return 1;
	}
	const char* ap_name = "Pico W WiFi";

	cyw43_arch_enable_ap_mode(ap_name, NULL, CYW43_AUTH_OPEN);

	ip4_addr_t mask;
	IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
	IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

	// Start the dhcp server
	dhcp_server_t dhcp_server;
	dhcp_server_init(&dhcp_server, &state->gw, &mask);

	// Start the dns server
	dns_server_t dns_server;
	dns_server_init(&dns_server, &state->gw);

	if (!tcp_server_open(state)) {
		printf("failed to open server\n");
		return 1;
	}

	while (!state->complete) {
		cyw43_arch_poll();
		cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
		// sleep_ms(1000);
	}
	dns_server_deinit(&dns_server);
	dhcp_server_deinit(&dhcp_server);
	cyw43_arch_deinit();
	return 0;
}

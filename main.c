#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef TOKEN
	#define TOKEN
#endif

typedef struct request {
	char* method;
	char* path;
	char* host;
	char* port;
	size_t content_len;
	char* boundary;
	char* content_type;
	char* content;
	char* body;
	char* reply;
} request_t;

void print_req(request_t* req) {
	if (!req) return;
	printf("typedef struct request {\n");
	printf("  char* path = \"%s\";\n", req->path);
	printf("  char* method = \"%s\";\n", req->method);
	printf("  char* host = \"%s\";\n", req->host);
	printf("  char* port = \"%s\";\n", req->port);
	printf("  size_t content_len = %ld;\n", req->content_len);
	printf("  char* boundary = \"%s\";\n", req->boundary);
	printf("  char* content_type = \"%s\";\n", req->content_type);
	printf("  char* content = \"%s\";\n", req->content);
	printf("  char* body = \"");

	for (size_t i = 0; i < strlen(req->body); i++) {
		bool isn = req->body[i] == '\n';
		bool isr = req->body[i] == '\r';
		bool isq = req->body[i] == '"';
		printf("%c%c", isn || isr || isq ? '\\' : req->body[i], isn ? 'n' : isr ? 'r'
																																		: isq		? '"'
																																						: '\0');
	}
	// printf("%s", req->body);
	printf("\";\n");
	printf("  char* reply = \"%s\";\n", req->reply);
	printf("} request_t;\n");
}

request_t* gen_req(char* url, char* method, char* content) {
	request_t* req = malloc(sizeof(request_t));
	bzero(req, sizeof(request_t));
	req->method = method;
	if (strstr(url, "https://"))
		url += 8;
	else if (strstr(url, "http://"))
		url += 7;
	char* urlp = url;
	req->path	 = url;
	while (*urlp != 0 && *(urlp) != '/' && *(urlp++) != ':')
		;
	while (*req->path != 0 && *(req->path++) != '/')
		;

	int len = req->path - urlp - 1;
	if (len > 0) {
		req->port = malloc(len + 1);
		for (int i = 0; i < len; i++)
			req->port[i] = urlp[i];
	} else {
		req->port = malloc(4);
		sprintf(req->port, "443");
	}
	len = urlp - url;
	if (len > 0) {
		req->host = malloc(len + 1);
		for (int i = 0; i < len; i++)
			req->host[i] = url[i];
		if (req->host[len - 1] == ':') req->host[len - 1] = 0;
	}
	urlp			= req->path;
	req->path = malloc(strlen(urlp) + 1);
	strcpy(req->path, urlp);

	char cnt_ty_bou[128] = "";
	if (strlen(content) > 0) {
		req->boundary = malloc(41);
		strcpy(req->boundary, "------------------------b73b76428e9a90f6");
		req->content_type = malloc(20);
		strcpy(req->content_type, "multipart/form-data");
		sprintf(cnt_ty_bou, "Content-Type: %s; boundary=%s\r\n", req->content_type, req->boundary);
		req->content = malloc((strlen(req->boundary) * 3) + strlen(content) + 173);
		sprintf(req->content, "--%s\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n84961485\r\n--%s\r\nContent-Disposition: form-data; name=\"document\"; filename=\"test.txt\"\r\nContent-Type: text/plain\r\n\r\n%s\r\n\r\n--%s--\r\n", req->boundary, req->boundary, content, req->boundary);
	}

	char cnt_len[32] = "";
	if (req->content)
		req->content_len = strlen(req->content);
	else
		req->content_len = 0;
	sprintf(cnt_len, "%lu", req->content_len);
	req->body = malloc(
			60 + // strlen(" / HTTP/1.1\r\nHost: \r\nContent-Length: \r\nConnection: close\r\n\r\n") +
			strlen(req->method) +
			strlen(req->path) +
			strlen(req->host) +
			strlen(cnt_len) +
			strlen(cnt_ty_bou) +
			req->content_len + 1);
	sprintf(req->body, "%s /%s HTTP/1.1\r\nHost: %s\r\nContent-Length: %ld\r\nConnection: close\r\n%s\r\n%s",
					req->method,
					req->path ? req->path : "",
					req->host,
					req->content_len,
					cnt_ty_bou,
					req->content ? req->content : "");
	return req;
}

void free_req(request_t* req) {
	if (!req) return;
#define FRIF(x) \
	if (x) free(x)
	FRIF(req->path);
	FRIF(req->host);
	FRIF(req->port);
	FRIF(req->boundary);
	FRIF(req->content_type);
	// FRIF(req->content);
	FRIF(req->body);
	FRIF(req->reply);
	free(req);
}

void https_req(request_t* req) {
	if (!req) return;
	size_t sz	 = 65536;
	req->reply = malloc(sz);
	bzero(req->reply, sz);
	struct hostent* hp;
	struct sockaddr_in addr;
	int on = 1, sock;

	SSL_library_init();
	SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());

	hp = gethostbyname(req->host);
	bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
	addr.sin_port		= htons(atoi(req->port));
	addr.sin_family = AF_INET;
	sock						= socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(int));

	size_t r = connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));

	SSL* ssl = SSL_new(ctx);
	SSL_set_fd(ssl, sock);
	SSL_connect(ssl);
	SSL_write(ssl, req->body, strlen(req->body));

	bzero(req->reply, sz);

	r = 0;
	while ((r = SSL_read(ssl, req->reply + r, sz - 1)) > 0)
		;

	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

	shutdown(sock, SHUT_RDWR);
	close(sock);
}

int main() {
#define TELEGRAM_STR "https://api.telegram.org/bot" TOKEN
	// char GET_UP_URL[]		= TELEGRAM_STR "/getUpdates";
	// char POST_MSG_URL[] = TELEGRAM_STR "/sendMessage?chat_id=84961485&text=Test";
	char DOC_MSG_URL[] = TELEGRAM_STR "/sendDocument";
	// while (true) {
	request_t* req_doc = gen_req(DOC_MSG_URL, "POST", "ciao");
	https_req(req_doc);
	print_req(req_doc);
	free_req(req_doc);
	// sleep(1);
	// }
	return 0;
}

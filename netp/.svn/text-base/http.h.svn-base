#ifndef HTTP_H
#define HTTP_H

//~ STRUCT DECLARATIONS ========================================= ~//

//- HTTP_request -----------------------------------------=
//
struct HTTP_request {
	char * raw;
	char * method;
	char * uri;
	char * version;
	char * filename;
	char * cgi_args;
};
//
struct HTTP_request* new_HTTP_request(rio_t);
void dealloc_HTTP_request(struct HTTP_request*);
struct HTTP_request * get_HTTP_request(void);
void print_HTTP_request(struct HTTP_request *);

//- HTTP_response ----------------------------------------=
//
struct HTTP_response {
	char * num;
	char * title;
	char * type;
	char * payload;
};
//
struct HTTP_response * gen_HTTP_response(char*, char*, char*, char*);
void send_HTTP_response(int, struct HTTP_response*, char*);
void dealloc_HTTP_response(struct HTTP_response*);


void buffered_file_transmit(int, struct HTTP_response*, FILE*, char*);




//~ IMPLEMENTATION ============================================== ~//

struct HTTP_request * new_HTTP_request(rio_t rio) {
	struct HTTP_request * req;

	req = malloc(sizeof(struct HTTP_request));

	req->raw      = malloc(sizeof(char) * MAXLINE);
	req->method   = malloc(sizeof(char) * MAXLINE);
	req->uri      = malloc(sizeof(char) * MAXLINE);
	req->version  = malloc(sizeof(char) * MAXLINE);
	req->filename = malloc(sizeof(char) * MAXLINE);
	req->cgi_args = malloc(sizeof(char) * MAXLINE);

	char buf[MAXLINE];

	// scrub!
	int i = 0;
	for (; i < MAXLINE; i++) { 
		req->raw[i]      = '\0'; 
		req->method[i]   = '\0';
		req->uri[i]      = '\0';
		req->version[i]  = '\0';
		req->filename[i] = '\0';
		req->cgi_args[i] = '\0';

		buf[i] = '\0';
	}

	int rln = rio_readlineb(&rio, buf, MAXLINE);
	int len = 0;
	while (rln > 2) {
		i = 0;
		while (len < rln && len < MAXLINE) {
			req->raw[len] = buf[i];
			len++, i++;
		}
		rln = rio_readlineb(&rio, buf, MAXLINE);
	}

	/*if (strlen(req->raw) == 0) {
		dealloc_HTTP_request(req);
		return NULL;
	}*/
	if (rln == 0) {//} || strlen(req->raw) == 0) {
		dealloc_HTTP_request(req);
		return NULL;
	}

	sscanf(req->raw, "%s %s %s", req->method, req->uri, req->version);
	parse_uri(req->uri, req->filename, req->cgi_args);
	
	return req;
}

void dealloc_HTTP_request(struct HTTP_request * req) {
	free(req->raw);
	free(req->method);
	free(req->uri);
	free(req->version);
	free(req->filename);
	free(req->cgi_args);
	free(req);
}

void print_HTTP_request(struct HTTP_request * req) {
	printf("/+ ============================\n");
	printf(" - Raw:             '%s'\n", req->raw);
	printf(" - Method:          '%s'\n", req->method);
	printf(" - URI:             '%s'\n", req->uri);
	printf(" - Version:         '%s'\n", req->version);
	printf(" - Filename:        '%s'\n", req->filename);
	printf(" - CGI Args:        '%s'\n", req->cgi_args);
	printf("\\+ ============================\n");
}

struct HTTP_response * gen_HTTP_response(char * num, char * title, char * type, char * payload) {
	struct HTTP_response * response = malloc(sizeof(struct HTTP_response));

	response->num = malloc(sizeof(char) * MAXLINE);
	response->title = malloc(sizeof(char) * MAXLINE);
	response->type = malloc(sizeof(char) * MAXLINE);
	response->payload = malloc(sizeof(char) * MAXBUF);

	sprintf(response->num    , "%s", num);
	sprintf(response->title  , "%s", title);
	sprintf(response->type   , "%s", type);
	sprintf(response->payload, "%s", payload);

	return response;
}

void send_HTTP_response(int fd, struct HTTP_response * res, char* ver) {
	char buf[MAXLINE];

	int content_length = strlen(res->payload);
	// HTTP MetaData
	sprintf(buf, "%s %s %s\r\n", ver, res->num, res->title);
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: %s\r\n", res->type);
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", content_length);
	rio_writen(fd, buf, strlen(buf));
	//
	// payload
	rio_writen(fd, res->payload, content_length);

	sprintf(buf, "\r\n");
	rio_writen(fd, buf, 2);

	//sprintf(buf, "\r\n");
	//rio_writen(fd, buf, 2);
}

void dealloc_HTTP_response(struct HTTP_response * response) {
	free(response->num);
	free(response->title);
	free(response->type);
	free(response->payload);
	free(response);
}



void buffered_file_transmit(int fd, struct HTTP_response * res, FILE * file, char* ver) {
	if (file == NULL) {
		return;
	}

	fseek(file, 0L, SEEK_END);
	int content_length = ftell(file);
	fseek(file, 0L, SEEK_SET);

	if (g_echo) { printf("\\>CONTENT-TYPE:   %s\n", res->type); }
	if (g_echo) { printf("\\>CONTENT-LENGTH: %d\n", content_length); }

	char buf[MAXBUF];
	sprintf(buf, "%s %s %s\r\n", ver, res->num, res->title);
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: %s\r\n", res->type);
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", content_length);
	rio_writen(fd, buf, strlen(buf));
	//
	// payload
	char * line = malloc(sizeof(char) * MAXLINE);
	size_t len;
	while (getline(&line, &len, file) != -1) {
		rio_writen(fd, line, strlen(line));
		if(g_echo) { printf("\\> %s", line); }
	}
	if(g_echo) { printf("\n"); }
	free(line);

	sprintf(buf, "\r\n");
	rio_writen(fd, buf, 2);
}


#endif
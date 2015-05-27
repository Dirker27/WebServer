#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "global_header.h"
#include "signal_handler.h"
#include "threadpool.h"
#include "csapp.h"
#include "html_utils.h"
#include "parse_utils.h"
#include "http.h"
#include "uri.h"
#include "list.h"

#define DEFAULT_PORT "8000"
#define GROUP_ID "group369"
#define MIN_THREADS 16 // Optimal (based on threadlab testing)
#define MAX_NUMBER_CONNECTIONS 16 // 100?

//- List of Active Connections ---------------------------=
//
struct list * active_connections;
struct connection_dt {
	struct list_elem elem;
	int fd;
};

//- Function Prototypes ----------------------------------=
//
void relay_mode(char*, char*);
void server_mode(char*);
void * _srvr(void *);
struct addrinfo* get_ipv4_ipv6( struct addrinfo *server);
int init_ip(struct addrinfo *p);

/**
 * MAIN
 *
 * Extracts Arguments and launches server in proper mode
 */
int main (int argc, char ** argv) {
	char * port  = DEFAULT_PORT;
	char * relay = NULL;

	//- Argument Extraction ------------------------------=
	//
	int opt;
	while ((opt = getopt(argc, argv, "p:r:R:ve")) != -1) {
		switch(opt) {
			case 'p':
				port = optarg;
				break;
			case 'r':
				relay = optarg;
				break;
			case 'R':
				g_home_path = optarg;

				// trim trailing '/' if present
				int len = strlen(g_home_path);
				if (len > 0 && g_home_path[len-1] == '/') {
					g_home_path[len-1] = '\0';
				}
				
				break;
			case 'v':
				g_verbose = true;
				break;
			case 'e':
				g_echo = true;
				break;
			default:
				printf("Usage: -p [port] | -r [relayhost:port] | -R [path]\n");
				return EXIT_SUCCESS;
		}
	}
	if (relay != NULL) {
		relay_mode(relay, port);
	}
	else {
		server_mode(port);
	}

	return EXIT_SUCCESS;
}

/**
 * RELAY MODE
 *
 * Opens a socket at the designated relay port
 * 
 * Sends payload to relay server
 *  - Group ID
 *  - /loadavg query
 *  - /meminfo query
 */
void relay_mode(char* relay_host, char* port) {
	init_signals();

	//- Extract Relay Host:Port --------------------------=
	//
	char * relay_port;
	char * relay_server = strtok_r(relay_host, ":", &relay_port);

	active_connections = NULL;
	active_connections = malloc(sizeof(struct list));
	list_init(active_connections);	

	
	while (!g_shutdown) {
		//- Connect to Relay Host:Port -----------------------=
		//
		struct addrinfo hints;
		struct addrinfo *res, *address;
		memset(&hints, 0, sizeof hints);
		//
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		int relay_fd, server_fd;
		//
		getaddrinfo(relay_server, relay_port, &hints, &res);
		//
		for(address = res; address != NULL; address = address->ai_next) {

			server_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
			relay_fd = connect(server_fd, address->ai_addr, address->ai_addrlen);

			if ((relay_fd != 0) || (server_fd < 0) ) {
				close(server_fd);
				continue;
			}
			break;
		}
		//
		freeaddrinfo(res);

		//- Write Data to Relay Server -----------------------=
		//
		printf("[%i]connecting...\n", server_fd);

		// Group ID
		char buf[MAXBUF];
		sprintf(buf, "%s\r\n", GROUP_ID);
		rio_writen(server_fd, buf, strlen(buf));
		
		struct connection_dt * connection = malloc(sizeof(struct connection_dt*));
		connection->fd = server_fd;

		_srvr(connection);
		printf("disconnected.\n");
	}	
}

/**
 *
 */
void server_mode(char* port) {
	
	//- Asset Initialization -----------------------------=
	//
	// Signal Handler
	init_signals();
	//
	// Thread Pool
	server_thread_pool = thread_pool_new(MIN_THREADS);
	pthread_mutex_init(&server_thread_lock, NULL);
	//
	// Connections List
	active_connections = NULL;
	active_connections = malloc(sizeof(struct list));
	list_init(active_connections);	

	//- Establish Connection Shit --------------------------------=
	//
	// Variable Declaration
	int connfd;
	socklen_t clientlen;
	struct sockaddr_in clientaddr;
	// 
	struct addrinfo hints;
	struct addrinfo *res, *address;
	memset(&hints, 0, sizeof hints);
	//
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	//
	getaddrinfo(NULL, port, &hints, &res);
	//
	// Determines ipv4 or ipv6
	address = get_ipv4_ipv6(res);
	int server_fd = init_ip(address);
	//
	freeaddrinfo(res);

	if (g_verbose) { printf("Starting on port %s\n", port); }

	if (server_fd != -1) {
		listen(server_fd, MAX_NUMBER_CONNECTIONS);
	}

	while (! g_shutdown) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(server_fd, (SA*) &clientaddr, &clientlen);

		bool found = false;
		size_t i = 0;
		struct connection_dt * connection;

		pthread_mutex_lock(&server_thread_lock);
		size_t size = list_size(active_connections);
		if (size > 0) {
			struct list_elem * e = list_front(active_connections);
			connection = list_entry (e, struct connection_dt, elem);

			for (; i < size; i++) {
				if (connection->fd == connfd) {
					found = true;
					break;
				}
				else {
					e = list_next(e);
					connection = list_entry (e, struct connection_dt, elem);
				}
			}
		}
		pthread_mutex_unlock(&server_thread_lock);
		
		if (found) {
			continue;
		}
		else {
			pthread_mutex_lock(&server_thread_lock);
			connection = malloc(sizeof(struct connection_dt));
			connection->fd = connfd;
			list_push_back(active_connections, &connection->elem);
			pthread_mutex_unlock(&server_thread_lock);
		}

		//- Multiple Client Support ----------------------=
		//
		// Determine thread pool vacancy
		int total_threads = thread_pool_num_total_threads(server_thread_pool);
		int running_threads = thread_pool_num_running_threads(server_thread_pool);
		//
		// Dispatch 1 thread per client connection.
		if (running_threads < total_threads) {
			if (g_verbose) { printf("new connection: %i\n", connfd); }

			thread_pool_submit(server_thread_pool, _srvr, connection);
		}
		//
		// Refuse service if at capacity
		else {
			char * errmsg = client_error("Too much shit to do; too few fucks/threads to care");
			struct HTTP_response * response = 
				gen_HTTP_response("503.13", "Server Overwhelmed", "text/html", errmsg);
			send_HTTP_response(connfd, response, "HTTP/1.1");

			Close(connfd);

			// Remove Connection from Roster
			pthread_mutex_lock(&server_thread_lock);
			list_remove(&connection->elem);
			pthread_mutex_unlock(&server_thread_lock);
		}
	}

	if (g_verbose) { printf("Shutting down..."); }
	thread_pool_shutdown(server_thread_pool);
	server_thread_pool = NULL;
	if (g_verbose) { printf("done.\n"); }
}

/**
 * 
 */
void * _srvr(void * param) {
	struct connection_dt * connection = (struct connection_dt*) param;
	int fd = connection->fd;

	rio_t rio;
	rio_readinitb(&rio, fd);

	bool close_connection = false;
	char * version = malloc(sizeof(char) * 16);
	while (! g_shutdown && ! close_connection) {
		struct HTTP_request * request = NULL;
		struct HTTP_response * response = NULL;
		//sprintf(version, "HTTP/1.1");
		sprintf(version, "HTTP/1.0");
		
		request = new_HTTP_request(rio);

		if (request == NULL) {
			close_connection = true;
		}
		else {
			// Print request details
			if (g_verbose) {
				print_HTTP_request(request);
			}

			//- HTTP 1.0 Compatability -------------------=
			//
			if (strcmp(request->version, "HTTP/1.0") == 0) {
				// close connection after completion
				close_connection = true;
				sprintf(version, "%s", request->version);
			}

			//~ SERVER FEATURES ================================= ~//
			//
			//- Reject if not GET request ----------------=
			//
			if (strcasecmp(request->method, "GET")) {
				char * errmsg = client_error("Server does not support this method");
				response = gen_HTTP_response("501", "Not Supported", "text/html", errmsg);
			}
			//
			//- Load Average -----------------------------=
			//
			else if (strcmp(request->uri, "/loadavg") == 0 ||
					strncmp(request->uri, "/loadavg?", 9) == 0) {
				char* callback = get_callback(request->uri);
				response = loadavg(callback);
			}
			//
			//- Memory Info ------------------------------=
			//
			else if (strcmp(request->uri, "/meminfo") == 0 ||
					strncmp(request->uri, "/meminfo?", 9) == 0) {
				char* callback = get_callback(request->uri);
				response = meminfo(callback);
			}
			//
			//- Run Loop (increase load) -----------------=
			//
			else if (strcmp(request->uri, "/runloop") == 0) {
				response = runloop();
			}
			//
			//- Alloc Anon (Claim Application Memory)-----=
			//
			else if (strcmp(request->uri, "/allocanon") == 0) {
				response = allocanon();
			}
			//
			//- Free Anon (Release Application Memory)----=
			//
			else if (strcmp(request->uri, "/freeanon") == 0) {
				response = freeanon();
			}
			//
			//- Serve Files (Explicit) -------------------=
			//
			else if (strncmp(request->uri, "/files", 6) == 0) {
				response = files(fd, request->filename, version);
			}
			//
			//- Serve Files (Implicit) -------------------=
			//
			else {
				char * fn = malloc(sizeof(char) * MAXLINE);
				sprintf(fn, "files/%s", request->filename);
				response = files(fd, fn, version);
				free(fn);
			}
			
			//- Reset ------------------------------------=
			//
			dealloc_HTTP_request(request);
			request = NULL;
		}

		//- Respond to Client (if generated) -------------=
		//
		if (response != NULL) {
			send_HTTP_response(fd, response, version);
			if (g_echo) {
				printf("\\> %s\n", response->payload);
			}
			dealloc_HTTP_response(response);
			response = NULL;
		}
	}

	//- Close Client Connection --------------------------=
	//
	free(version);

	if (g_verbose) { printf("closing %i...", fd); }
	Close(fd);

	pthread_mutex_lock(&server_thread_lock);
	if (list_size(active_connections) > 0 ) {
		list_remove(&connection->elem);
	}
	pthread_mutex_unlock(&server_thread_lock);
	if (g_verbose) { printf("done.\n"); }

	return NULL;
}

/*
 * Determines if the address corresponds to the addrinfo struct
 * passed in as a parameter is an ipv4 or ipv6 address by 
 * looping through the struct of addresses and checking
 * to see if the address is in the AF_INET family for ipv4
 * or AF_INET6 for ipv6. If a matching ipv6 address is 
 * found it is returned (ipv6 takes precedence over ipv4).
 * otherwise the ipv4 address is found.
 */
struct addrinfo* get_ipv4_ipv6( struct addrinfo *address ) {

        struct addrinfo *ipv4 = NULL;
        struct addrinfo *ipv6 = NULL;

        for (; address != NULL; address = address->ai_next) {

                if (address->ai_family == AF_INET) {
                        ipv4 = address;
                } else if (address->ai_family == AF_INET6) {
                        ipv6 = address;
                }
        }

	if (ipv6 != NULL) {
		return ipv6;
	}

        return ipv4;
}

/*
 * Given an address we first check to see that an unbound socket connection 
 * can be made, if false we exit and return -1. Next we  attempt to set the
 * ipv6 socket options for an ipv6 address if this isn't possible we attempt 
 * to set the socket level options; if both fail we exit and return -1. If
 * a socket has been successfully created and the options have been set we
 * can then bind the file descriptor to the address; if the bind is a 
 * success we return the file descriptor otherwise we return -1.
 */
int init_ip(struct addrinfo *address) {
	int socket_fd;

	int option_Yes = 1;
	int option_No = 0;

	if ( (socket_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol)) == -1) {
		
		return -1;
	}

	if ((setsockopt(socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&option_No, sizeof(int)) == -1) ||
	    (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option_Yes, sizeof(int)) == -1)) {

		return -1;
	}

	if (bind(socket_fd, address->ai_addr, address->ai_addrlen) == -1) {
		close(socket_fd);
		return -1;
	}

	return socket_fd;
}

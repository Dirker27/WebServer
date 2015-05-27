#ifndef URI_H
#define URI_H

#include <sys/mman.h>

struct HTTP_response* loadavg(char*);
struct HTTP_response* meminfo(char*);
struct HTTP_response* runloop(void);
struct HTTP_response* allocanon(void);
struct HTTP_response* freeanon(void);
struct HTTP_response* files(int, char*, char*);
void* _loop(void*);

// SIZE is defined to be 64 MB NOT the value 64
#define SIZE 64*1024*1024

//- Memory Roster for Mem Fnc's --------------------------=
//
struct allocated_block *block_list = NULL;
struct allocated_block {
	struct allocated_block * next;
	void * memory_address;
};

/**
 * Returns a JSON representation of the host's load statistics
 * 
 * Load Statistics retrieved from /proc/loadavg
 *
 * Outputs as JavaScript callback if one is provided
 */
struct HTTP_response * loadavg(char * callback) {
	char * json = malloc(sizeof(char) * MAXLINE);
	char * output = malloc(sizeof(char) * MAXLINE);

	FILE * fp = fopen("/proc/loadavg", "r");

	char * line;
	size_t len = 0;
	if ((getline(&line, &len, fp) != -1)) {
		double avg_1m, avg_5m, avg_15m;
		int running_threads, total_threads;
		int last_pid;

		sscanf(line, "%lf %lf %lf %i/%i %i", &avg_1m, &avg_5m, &avg_15m, 
			&running_threads, &total_threads, &last_pid);

		sprintf(json, "{");
		sprintf(json, "%s\"total_threads\": \"%i\", ", json, total_threads);
		sprintf(json, "%s\"loadavg\": [\"%.2f\",\"%.2f\",\"%.2f\"], ", json,
			avg_1m, avg_5m, avg_15m);
		sprintf(json, "%s\"running_threads\": \"%i\"}", json, running_threads);
	}
	fclose(fp);

	char * type;
	if (callback != NULL) {
		sprintf(output, "%s(%s)", callback, json);
		type = "text/javascript";
	} else {
		sprintf(output, "%s", json);
		//type = "text/javascript";
		type = "application/json";
	}

	struct HTTP_response * response = gen_HTTP_response("200", "OK", type, output);
	free(json);
	free(output);

	return response;
}

/**
 * Returns a JSON representation of the host's memory statistics
 * 
 * Memory Statistics retrieved from /proc/meminfo
 *
 * Outputs as JavaScript callback if one is provided
 */
struct HTTP_response * meminfo(char * callback) {
	char * json = malloc(sizeof(char) * MAXLINE);
	char * output = malloc(sizeof(char) * MAXLINE);
	sprintf(json, "{");

	FILE * fp = fopen("/proc/meminfo", "r");

	char * line;
	size_t len = 0;
	char * key = malloc(sizeof(char) * 32);
	char * val = malloc(sizeof(char) * 32);
	while ((getline(&line, &len, fp) != -1)) {
		// scrub key/val
		int i;
		for (i = 0; i < 32; i++) { key[i] = '\0'; }
		for (i = 0; i < 32; i++) { val[i] = '\0'; }

		int bytes;
		char field[32];
		sscanf(line, "%s %i kB\n", field, &bytes);
		
		strncpy(key, field, strlen(field) - 1); //remove ':'
		sprintf(val, "\"%i\"", bytes);

		sprintf(json, "%s\"%s\": %s, ", json, key, val);
	}
	fclose(fp);
	free(key);
	free(val);

	// finish up json
	int l = strlen(json);
	json[l-2] = '}';
	json[l-1] = '\0';

	char * type;
	if (callback != NULL) {
		sprintf(output, "%s(%s)", callback, json);
		type = "text/javascript";
	} else {
		sprintf(output, "%s", json);
		//type = "text/javascript";
		type = "application/json";
	}

	struct HTTP_response * response = gen_HTTP_response("200", "OK", type, output);
	free(json);
	free(output);

	return response;
}

/**
 * Dispatches a thread to spin for 15 seconds
 */
struct HTTP_response * runloop() {
	thread_pool_submit(server_thread_pool, _loop, NULL);

	return gen_HTTP_response("200", "OK", "text/html", "Spinning for 15 seconds...");
}
/**
 * HELPER FUNCTION: runloop
 *
 * Thread body for spinning thread (terminates after 15 seconds)
 */
void * _loop(void * param) {
	long start = time(0);
	long current = start;
	while (current - start < 15) {
		//printf("%ld\n", current - start);
		current = time(0);
	}

	return NULL;
}

/**
 * Allocates 64MB of Application Memory to increase Memory Load
 */
struct HTTP_response * allocanon() {
	struct allocated_block *new_block = malloc(sizeof(struct allocated_block));

	new_block->memory_address = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	//- If could not allocate memory ---------------------=
	//
	if (new_block->memory_address == (void *) -1) {
		new_block->memory_address = MAP_FAILED;
	}

	//- If memory successfully allocated -----------------=
	//
	if( new_block->memory_address != MAP_FAILED) {
		new_block->next = block_list;
		block_list = new_block;

		// Mark Territory
		char * garbage = new_block->memory_address;
		int i = 0;
		for (; i < SIZE; i += 128) {
			garbage[i] = 'x';
		}
	}

	return gen_HTTP_response("200", "OK", "text/html", "Alloc'd 64 MB");
}

/**
 * Frees 64 MB of Application Memory (if any have been allocated)
 */
struct HTTP_response * freeanon() {
	struct allocated_block *new_block = block_list;

	if (block_list != NULL) {
		block_list = block_list->next;
                munmap(new_block->memory_address, SIZE);
	}

	return gen_HTTP_response("200", "OK", "text/html", "Free'd 64 MB");
}

/**
 * Serves a file to the client
 *
 * Only serves files from the designated files/ directory (can be redirected
 *  using the -R launch switch).
 *
 * Returns 403's and 404's as necessary.
 *
 * Note: Due to the fact that the file may very likely exceed the size of
 * 	our buffers, we must use a buffered writer to load the file's data
 *  directly to the shared file descriptor. This is done through the function
 *  "buffered_file_transmit" in http.h
 */
struct HTTP_response * files(int fd, char * filename, char* ver) {
	struct HTTP_response * response = NULL;

	//- [403] Forbidden ----------------------------------=
	//
	if (strstr("..", filename) != NULL) {
		char * errmsg = client_error("Access to parent directories not allowed!");
		response = gen_HTTP_response("403", "Forbidden", "text/html", errmsg);
		free(errmsg);

		printf(">> [403] - Fobidden\n");

		return response;
	}

	//- [404] Not Found ----------------------------------=
	//
	FILE * file;
	char * fn = malloc(sizeof(char) * MAXLINE);
	sprintf(fn, "%s/%s", g_home_path, filename);
	if ( (file = fopen(fn, "r")) == 0 ) {
		char * errmsg = client_error("Specified file does not exist in given path.");
		response = gen_HTTP_response("404", "Not Found", "text/html", errmsg);
		free(errmsg);
		free(fn);

		return response;
	}
	free(fn);

	//- Serve File ---------------------------------------=
	//
	// Extract File Extension
	char * file_ext = NULL;
	char * fe = strtok(filename, ".");
	while (fe != NULL) {
		file_ext = fe;
		fe = strtok(NULL, ".");
	}
	if (file_ext == NULL) {
		file_ext = "html";
	}
	//
	// Convert to HTTP Content-Type
	char * type = get_content_type(file_ext);
	// 
	// Send the HTTP response directly from here 
	// 		(need to use buffered file write)
	response = gen_HTTP_response("200", "OK", type, "");
	buffered_file_transmit(fd, response, file, ver);
	fclose(file);
	dealloc_HTTP_response(response);

	return NULL;
}

#endif

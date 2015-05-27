#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

int parse_uri(char *uri, char *filename, char *cgiargs) {
	char *ptr;

	if (!strstr(uri, "cgi-bin")) {  /* Static content */ //line:netp:parseuri:isstatic
		strcpy(cgiargs, "");                             //line:netp:parseuri:clearcgi
		strcpy(filename, ".");                           //line:netp:parseuri:beginconvert1
		strcat(filename, uri);                           //line:netp:parseuri:endconvert1
		if (uri[strlen(uri)-1] == '/') {                 //line:netp:parseuri:slashcheck
		    strcat(filename, "index.html");               //line:netp:parseuri:appenddefault
		}
		return 1;
	}
	else {  /* Dynamic content */                        //line:netp:parseuri:isdynamic
		ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
		if (ptr) {
			strcpy(cgiargs, ptr+1);
			*ptr = '\0';
		} else {
			strcpy(cgiargs, "");                         //line:netp:parseuri:endextract
		}
		strcpy(filename, ".");                           //line:netp:parseuri:beginconvert2
		strcat(filename, uri);                           //line:netp:parseuri:endconvert2
		return 0;
	}
}

char* get_content_type(char * ext) {

	if (strcmp(ext, "html") == 0) {
		return "text/html";
	}
	else if (strcmp(ext, "js") == 0) {
		return "text/javascript";
	}
	else if (strcmp(ext, "css") == 0) {
		return "text/css";
	}
	else if (strcmp(ext, "ico") == 0) {
		return "image/x-icon";
	}
	else if (strcmp(ext, "jpg") == 0) {
		return "image/jpeg";
	}
	else if (strcmp(ext, "png") == 0) {
		return "image/png";
	}

	//return NULL;
	return "text/plain";
}

char* get_callback(char * uri) {
	char * callback = NULL;

	char * variable = strtok(uri, "?&");
	while (variable != NULL) {
		if (strncmp(variable, "callback", 8) == 0) {
			callback = strtok(variable, "=");
			callback = strtok(NULL, "=");
			break;
		} else {
			variable = strtok(NULL, "?&");
		}
	}

	if (callback != NULL) {
		int i = 0;
		size_t len = strlen(callback);
		for (; i < len; i++) {
			if ( (! isalnum(callback[i])) &&
				(callback[i] != '_') &&
				(callback[i] != '.') ) {
				callback = NULL;
				break;
			}
		}
	}

	return callback;
}

#endif
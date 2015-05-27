#ifndef HTML_UTILS_H
#define HTML_UTILS_H

char * client_error(char * message) 
{
	char *body, *head, *doc;
	body = malloc(sizeof(char) * MAXBUF);
	head = malloc(sizeof(char) * MAXBUF);
	doc = malloc(sizeof(char) * MAXBUF);
	
	//- HTML Head ----------------------------------------=
	//
	sprintf(head, "<head>\r\n");
	sprintf(head, "%s<title>WebServer Error</title>\r\n", head);
	sprintf(head, "%s</head>\r\n", head);

	//- HTML Body ----------------------------------------=
	//
	sprintf(body, "<body bgcolor=""ffffff"">\r\n");
	//sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s</p>\r\n", body, message);
	sprintf(body, "%s<hr><em>The Ugly-Ass Web Server</em></hr>\r\n", body);
	sprintf(body, "%s</body>\r\n", body);

	//- Doc Compilation ----------------------------------=
	//
	sprintf(doc, "<html>\r\n");
	sprintf(doc, "%s%s", doc, head);
	sprintf(doc, "%s%s", doc, body);
	sprintf(doc, "%s</html>\r\n", doc);	

	free(body);
	free(head);

	return doc;
}

#endif

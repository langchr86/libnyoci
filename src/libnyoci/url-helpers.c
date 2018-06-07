/*	@file url-helpers.c
**	@author Robert Quattlebaum <darco@deepdarc.com>
**
**	Originally published 2010-8-31.
**
**	This file was written by Robert Quattlebaum <darco@deepdarc.com>.
**
**	This work is provided as-is. Unless otherwise provided in writing,
**	Robert Quattlebaum makes no representations or warranties of any
**	kind concerning this work, express, implied, statutory or otherwise,
**	including without limitation warranties of title, merchantability,
**	fitness for a particular purpose, non infringement, or the absence
**	of latent or other defects, accuracy, or the present or absence of
**	errors, whether or not discoverable, all to the greatest extent
**	permissible under applicable law.
**
**	To the extent possible under law, Robert Quattlebaum has waived all
**	copyright and related or neighboring rights to this work. This work
**	is published from the United States.
**
**	I, Robert Quattlebaum, dedicate any and all copyright interest in
**	this work to the public domain. I make this dedication for the
**	benefit of the public at large and to the detriment of my heirs and
**	successors. I intend this dedication to be an overt act of
**	relinquishment in perpetuity of all present and future rights to
**	this code under copyright law. In jurisdictions where this is not
**	possible, I hereby release this code under the Creative Commons
**	Zero (CC0) license.
**
**	 * <http://creativecommons.org/publicdomain/zero/1.0/>
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <string.h>
#include "assert-macros.h"

#include "nyoci-helpers.h"
#include "nyoci-missing.h"

#include "url-helpers.h"

#ifdef __SDCC
#include <malloc.h>
#endif

#ifndef __SDCC
#ifndef HAVE_C99_VLA
#define HAVE_C99_VLA	1
#endif

#ifndef HAVE_STRSEP
#define HAVE_STRSEP     1
#endif

#ifndef HAVE_STRDUP
#define HAVE_STRDUP     1
#endif
#endif // ifndef __SDCC

#if !defined(strsep) && !HAVE_STRSEP
/* ---------------------------------------------------------------- */
/* BEGIN BSD SECTION */
/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
char *
strsep_x(char **stringp, const char *delim)
{
	char *s;
	const char *spanp;
	int c, sc;
	char *tok;

	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}
#define strsep	strsep_x
/* END BSD SECTION */
/* ---------------------------------------------------------------- */
#endif

#if !HAVE_STRDUP && !defined(strdup)
#define strdup strdup__
static inline char*
strdup(const char* cstr) {
	size_t len = strlen(cstr);
	char* ret = (char*)malloc(len+1);
	memcpy(ret,cstr,len);
	ret[len]=0;
	return ret;
}
#endif

static bool
isurlchar(char src_char) {
	return isalnum(src_char)
	   || (src_char == '$')
	   || (src_char == '-')
	   || (src_char == '_')
	   || (src_char == '.')
	   || (src_char == '+')
	   || (src_char == '!')
	   || (src_char == '*')
	   || (src_char == '\'')
	   || (src_char == '(')
	   || (src_char == ')')
	   || (src_char == ',');
}

#if __AVR__
#include <avr/pgmspace.h>
static char int_to_hex_digit(uint8_t x) {
	return pgm_read_byte_near(
		PSTR("0123456789ABCDEF") + (x & 0xF)
	);
}
#else
static char int_to_hex_digit(uint8_t x) {
	return "0123456789ABCDEF"[x & 0xF];
}
#endif

size_t
url_encode_cstr(
	char *dest, const char*src, size_t max_size
) {
	size_t ret = 0;

	if(!max_size--)
		return 0;

	while(true) {
		const char src_char = *src++;
		if(src_char==0)
			break;

		if(max_size==0) {
			ret++;
			break;
		}

		if(isurlchar(src_char)) {
			*dest++ = src_char;
			ret++;
			max_size--;
#if URL_ENCODE_SPACES_AS_PLUSES
		} else if(src_char == ' ') {
			*dest++ = '+';  // Stupid legacy space encoding.
			ret++;
			max_size--;
#endif
		} else {
			if(max_size < 3) {
				// Dest buffer too small for the next character.
				ret++;
				break;
			}

			*dest++ = '%';
			*dest++ = int_to_hex_digit(src_char >> 4);
			*dest++ = int_to_hex_digit(src_char & 0xF);
			ret += 3;
			max_size -= 3;
		}
	}

	*dest = 0;

	return ret;
}

static char
hex_digit_to_int(char c) {
	if(c>='0' && c<='9')
		return c-'0';
	if(c>='A' && c<='F')
		return 10+c-'A';
	if(c>='a' && c<='f')
		return 10+c-'a';
	return 0;
}

size_t
url_decode_str(
	char *dest,
	size_t max_size,
	const char* src,		// Length determined by src_len.
	size_t src_len
) {
	size_t ret = 0;

	if(!max_size--)
		return 0;

	while(src_len--) {
		const char src_char = *src++;
		if(!src_char)
			break;

		if(!max_size) {
			ret++;
			break;
		}

		if((src_char == '%')
			&& (src_len>=2)
			&& src[0]
			&& src[1]
		) {
			*dest++ = (char)(hex_digit_to_int(src[0]) << 4)
				+ hex_digit_to_int(src[1]);
			src += 2;
			src_len -= 2;
		} else if(src_char == '+') {
			*dest++ = ' ';  // Stupid legacy space encoding.
		} else {
			*dest++ = src_char;
		}

		ret++;
		max_size--;
	}

	*dest = 0;

	return ret;
}

size_t
url_decode_cstr(
	char *dest,
	const char*src,
	size_t max_size
) {
	size_t ret = 0;
#if DEBUG
	char*const dest_check = dest;
#endif

	if(!max_size--)
		goto bail;

	while(true) {
		const char src_char = *src++;
		if(!src_char) {
			break;
		}

		if(!max_size) {
			ret++;
			break;
		}

		if((src_char == '%')
			&& src[0]
			&& src[1]
			&& !(src[0] == '0' && src[1] == '0')
		) {
			*dest++ = (char)(hex_digit_to_int(src[0]) << 4)
				+ hex_digit_to_int(src[1]);
			src += 2;
		} else if(src_char == '+') {
			*dest++ = ' ';  // Stupid legacy space encoding.
		} else {
			*dest++ = src_char;
		}

		ret++;
		max_size--;
	}

bail:
	*dest = 0;
#if DEBUG
	assert(strlen(dest_check)==ret);
#endif
	return ret;
}

void
url_decode_cstr_inplace(char *str) {
	url_decode_cstr(str, str, -1);
}

size_t
quoted_cstr(
	char *dest,
	const char* src,		// Must be zero-terminated.
	size_t dest_max_size
) {
	char* ret = dest;

	require(dest_max_size,bail);
	dest_max_size--;		// For zero termination.

	require(dest_max_size,bail);
	*dest++ = '"';
	dest_max_size--;

	require(dest_max_size,bail);

	while(dest_max_size-1) {
		char src_char = *src++;

		if(!src_char)
			break;

		if((src_char == '/') && src[0] == '"')
			src_char = *src++;
		*dest++ = src_char;
		dest_max_size--;
	}

	*dest++ = '"';
	dest_max_size--;
bail:
	*dest = 0;

	return dest-ret;
}

size_t
url_form_next_value(
	char** form_string, char** key, char** value
) {
	size_t bytes_parsed = 0;
	char c = **form_string;
	char* v = NULL;

	if(!c) {
		*key = NULL;
		goto bail;
	}

	*key = *form_string;

	while(true) {
		c = **form_string;

		if(!c)
			goto bail;

		if(c == '=')
			break;

		bytes_parsed++;

		if((c == ';') || (c == '&')) {
			*(*form_string)++ = 0;
			goto bail;
		}

		(*form_string)++;
	}

	// Zero terminate the key.
	if(value)
		**form_string = 0;

	(*form_string)++;

	bytes_parsed++;

	v = *form_string;

	while(true) {
		c = **form_string;

		if(!c || (c == ';') || (c == '&'))
			break;

		bytes_parsed++;
		(*form_string)++;
	}

	// Zero terminate the value
	if(*form_string[0]) {
		*(*form_string)++ = 0;
		bytes_parsed++;
	}

bail:
	if(v)
		url_decode_cstr_inplace(v);
	if(value)
		*value = v;

	return bytes_parsed;
}

size_t
url_path_next_component(
	char** path_string, char** component
) {
	size_t bytes_parsed = 0;
	char c = **path_string;

	if(!c) {
		*component = NULL;
		goto bail;
	}

	*component = *path_string;

	while(true) {
		c = **path_string;

		if(!c || (c == '/'))
			break;

		bytes_parsed++;
		(*path_string)++;
	}

	// Zero terminate the value
	if(*path_string[0]) {
		*(*path_string)++ = 0;
		bytes_parsed++;
	}

bail:
	if(*component)
		url_decode_cstr_inplace(*component);

	return bytes_parsed;
}

int
url_parse(
	char* uri,
	struct url_components_s* components
) {
	int bytes_parsed = 0;
	char tmp;

	if(!url_is_absolute(uri))
		goto skip_absolute_url_stuff;

	check_string(uri, "NULL URI parameter");

	components->protocol = uri;

	while(*uri != ':') {
		require_string(*uri, bail, "unexpected end of string");
		uri++;
		bytes_parsed++;
	}

	// Zero terminate the protocol;
	*uri++ = 0;
	bytes_parsed++;

	require_string(*uri, bail, "unexpected end of string");

	if(uri[0] == '/' && uri[1] == '/') {
		char* addr_end;
		char* addr_begin;
		bool got_port = false;

		// Contains a hostname. Parse it.
		uri += 2;
		bytes_parsed += 2;

		addr_begin = uri;

		while( (tmp=*uri) != '/'
			&& ( isurlchar(tmp)
				|| (tmp == '[')
				|| (tmp == ']')
				|| (tmp == ':')
				|| (tmp == '%')		// Necessary for scoped addresses
				|| (tmp == '@')
			)
		) {
			uri++;
			bytes_parsed++;
		}

		addr_end = uri - 1;

		if(*uri) {
			*uri++ = 0;
			bytes_parsed++;
		}

		for(;
			(addr_end >= addr_begin) && (*addr_end != '@') &&
			(*addr_end != '[');
			addr_end--
		) {
			if(*addr_end == ']') {
				*addr_end = 0;
				got_port = true;
			} else if(!got_port && (*addr_end == ':')) {
				components->port = addr_end + 1;
				*addr_end = 0;
				got_port = true;
			}
		}
		components->host = addr_end + 1;

		if(*addr_end=='@') {
			*addr_end = 0;
			for(;
				(addr_end >= addr_begin) && (*addr_end != '/');
				addr_end--
			) {
				if(*addr_end==':') {
					*addr_end = 0;
					components->password = addr_end + 1;
				}
			}
			if(*addr_end=='/') {
				components->username = addr_end + 1;
			}
		}
	}

skip_absolute_url_stuff:

	components->path = uri;

	// Move to the end of the path.
	while( ((tmp=*uri) != '#')
		&& (tmp != '?')
		&& ( isurlchar(tmp)
			|| (tmp == '[')
			|| (tmp == ']')
			|| (tmp == ':')
			|| (tmp == '/')
			|| (tmp == '=')
			|| (tmp == '&')
			|| (tmp == '%')
		)
	) {
		uri++;
		bytes_parsed++;
	}

	// Handle query component
	if(*uri == '?') {
		*uri++ = 0;
		bytes_parsed++;

		components->query = uri;

		// Move to the end of the query.
		while( ((tmp=*uri) != '#')
			&& ( isurlchar(tmp)
				|| (tmp == '[')
				|| (tmp == ']')
				|| (tmp == ':')
				|| (tmp == '/')
				|| (tmp == '=')
				|| (tmp == '&')
				|| (tmp == '%')
				|| (tmp == ';')
			)
		) {
			uri++;
			bytes_parsed++;
		}
	}

	// Zero terminate
	if(*uri) {
		*uri = 0;
		bytes_parsed++;
	}

bail:
	return bytes_parsed;
}

bool
url_is_absolute(const char* uri) {
	bool ret = false;
	int bytes_parsed = 0;

	require(uri, bail);

	while(*uri && (isalpha(*uri) || *uri=='-') && (*uri != ':')) {
		require(0 != *uri, bail);
		uri++;
		bytes_parsed++;
	}

	if(bytes_parsed && *uri == ':')
		ret = true;

bail:
	return ret;
}

bool
url_is_root(const char* url) {
	bool ret = false;

	require(url, bail);

	if(!isalpha(url[0])) goto bail;

	if(url_is_absolute(url)) {
		while(*url && isalpha(*url) && (*url != ':')) {
			require(*url, bail);
			url++;
		}
		if(	(url[0] != ':')
			|| (url[1] != '/')
			|| (url[2] != '/')
		)	goto bail;

		url+=3;

		while(*url && (*url != '/')) {
			url++;
		}
	}

	while(url[0]=='/' && url[1]=='/') url++;
	ret = (url[0]==0) || ((url[0]=='/') && (url[1]==0));

bail:
	return ret;
}

//!< Used to identify IPv6 address strings
bool
string_contains_colons(const char* str) {
	if(!str)
		return false;
	for(; (*str != ':') && *str; str++) ;
	return *str == ':';
}

#if !NYOCI_EMBEDDED && !defined(__SDCC) && !defined(__AVR__)
bool
url_change(
	char* url, const char* new_url_
) {
	// TODO: This function is inefficient. Optimize it!

	bool ret = false;
#if !HAVE_C99_VLA
	char *new_url = NULL;
#endif
	if(url_is_absolute(new_url_)) {
		strcpy(url, new_url_);
		ret = true;
		goto bail;
	}

	{
		bool has_trailing_slash = new_url_[0]?('/' == new_url_[strlen(new_url_)-1]):false;
		char current_path[MAX_URL_SIZE];
		struct url_components_s components = {0};

#if HAVE_C99_VLA
		char new_url[strlen(new_url_) + 1];
		strcpy(new_url, new_url_);
#else
		new_url = strdup(new_url_);
#endif

		strncpy(current_path, url,MAX_URL_SIZE);

		url_parse(current_path,&components);

#if VERBOSE_DEBUG
		fprintf(
			stderr,
			"url_change: proto=\"%s\", host=\"%s\", port=\"%s\", path=\"%s\"\n",
			components.protocol,
			components.host,
			components.host,
			components.path
		);
#endif
		url[0] = 0;

		if(components.protocol && components.host) {
			strcat(url, components.protocol);
			strcat(url, "://");
			if(components.username) {
				strcat(url, components.username);
				if(components.password) {
					strcat(url, ":");
					strcat(url, components.password);
				}
				strcat(url, "@");
			}

			// Path is absolute. Must drop the path from the URL.
			if(string_contains_colons(components.host)) {
				strcat(url, "[");
				strcat(url, components.host);
				strcat(url, "]");
			} else {
				strcat(url, components.host);
			}

			if(components.port) {
				strcat(url, ":");
				strcat(url, components.port);
			}
		}

		if(new_url_[0]!='?' && new_url_[0]!='#') {
			// Remove the basename if the starting URL doesn't end with a slash.
			size_t path_len = strlen(components.path);
			for(;path_len && components.path[path_len-1]!='/';path_len--);
		}

		{
			char* path_components[URL_HELPERS_MAX_URL_COMPONENTS];
			char** pp = path_components;
			int path_items = 0;

			if(new_url[0] != '/') {
				while((*pp = strsep(&components.path, "/"))) {
					if(strcmp(*pp, "..") == 0) {
						if(path_items) {
							pp--;
							path_items--;
						}
					} else if(strcmp(*pp, ".") == 0) {
					} else if(**pp != '\0') {
						pp++;
						path_items++;
					}
				}
			}

			components.path = new_url;
			while((*pp = strsep(&components.path, "/"))) {
				if(strcmp(*pp, "..") == 0) {
					if(path_items) {
						pp--;
						path_items--;
					}
				} else if(strcmp(*pp, ".") == 0) {
				} else if(**pp != '\0') {
					pp++;
					path_items++;
				}
			}


			for(pp = path_components; path_items; path_items--, pp++) {
				strcat(url, "/");
				strcat(url, *pp);
			}
			if(has_trailing_slash)
				strcat(url,"/");
		}

		ret = true;
	}
bail:
#if !HAVE_C99_VLA
	free(new_url);
#endif

#if VERBOSE_DEBUG
	fprintf(stderr, "url_change: final-url=\"%s\"\n", url);
#endif

	return ret;
}


void
url_shorten_reference(
	const char* current_url, char* new_url
) {
	// TODO: This function is a mess! Clean it up!
#if !HAVE_C99_VLA
	char* temp2 = NULL;
#endif

	if(url_is_absolute(new_url)) {
		int i;
		if((0 ==
				strncmp(current_url, new_url,
					7)) && (0 == strncmp(current_url + 4, "://", 3))) {
			for(i = 7;
					(current_url[i] == new_url[i]) &&
					(current_url[i] != '/') &&
				current_url[i];
				i++) {
			}
			if(current_url[i] == new_url[i]) {
				if(current_url[i] == 0) {
					new_url[0] = '/';
					new_url[1] = 0;
					goto bail;
				} else if(current_url[i] == '/') {
					size_t len = strlen(new_url);
					memmove(new_url, new_url + i, len - i);
					new_url[len - i] = 0;
					goto make_relative;
				}
			}
		}
		goto bail;
	}

make_relative:
	{
		char temp[256]; // Yuck!
		char* new_path;
		char* new_query = NULL;
		char* curr_path;
#if HAVE_C99_VLA
		char temp2[strlen(current_url) + 1];
		strcpy(temp2, current_url);
#else
		uint16_t len = strlen(current_url);
		temp2 = malloc(len + 1);
		memcpy(temp2,current_url,len);
		temp2[len]=0;
#endif

		strncpy(temp, current_url, sizeof(temp));
		url_change(temp, new_url);

		{
			struct url_components_s components = {0};
			url_parse(temp,&components);
			new_path = components.path;
			new_query = components.query;
			components.path = NULL;
			url_parse(temp2, &components);
			curr_path = components.path;
		}

		while(true) {
			char* curr_item = strsep(&curr_path, "/");
			char* new_item;

			if(!curr_item || (curr_item[0] == 0)) {
				if(!new_path) {
					new_url[0] = '.';
					new_url[1] = 0;
				} else if((strlen(new_path) +
							(new_query ? 1 + strlen(new_query) : 0)) <
					strlen(new_url)) {
					strcpy(new_url, new_path);
					if(new_query) {
						strlcat(new_url, "?", sizeof(temp));
						strlcat(new_url, new_query, sizeof(temp));
					}
				}
				goto bail;
			}

			new_item = strsep(&new_path, "/");
			if(!new_item || (new_item[0] == 0)) {
				strcpy(temp2, "..");
				while(strsep(&curr_path, "/"))
					strlcat(temp2, "/..", sizeof(temp));
				if(strlen(temp2) +
						(new_query ? 1 + strlen(new_query) : 0) <
					strlen(new_url)) {
					strcpy(new_url, temp2);
					if(new_query) {
						strlcat(new_url, "?", sizeof(temp));
						strlcat(new_url, new_query, sizeof(temp));
					}
				}
				goto bail;
			}

			if(0 == strcmp(curr_item, new_item))
				continue;

			strcpy(temp, "..");
			while(strsep(&curr_path, "/"))
				strcat(temp, "/..");
			strlcat(temp, "/", sizeof(temp));
			memmove(temp+strlen(temp),new_item,strlen(new_item)+1);
			if(new_path) {
				strlcat(temp, "/", sizeof(temp));
				memmove(temp+strlen(temp),new_path,strlen(new_path)+1);
			}

			if(strlen(temp) + (new_query ? 1 + strlen(new_query) : 0) <
				strlen(new_url)) {
				strcpy(new_url, temp);
				if(new_query) {
					strlcat(new_url, "?", sizeof(temp));
					strlcat(new_url, new_query, sizeof(temp));
				}
			}

			break;
		}
	}
bail:
#if !HAVE_C99_VLA
	free(temp2);
#endif
	return;
}

#endif

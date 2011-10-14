/* -*- coding: utf-8; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: t -*- */

// This code and all comments, written by Daniel Trebbien, are hereby entered into the Public Domain by their author.

#include <assert.h>
#include <string.h>

#include "string_utils.h"

size_t x_strlcpy(char *__restrict dest, const char *__restrict src, size_t dest_len)
{
	if (dest_len == 0) {
		return strlen(src);
	} else {
		char *const dest_str_end = dest + dest_len - 1;
		char *__restrict d = dest;

		while (d != dest_str_end)
		{
			if ((*d++ = *src++) == '\0') {
				assert(*(src - 1) == '\0');
				assert(*(d - 1) == '\0');
				return (d - 1 - dest);
			}
		}

		*d = '\0';
		return (dest_len - 1) + strlen(src);
	}
}

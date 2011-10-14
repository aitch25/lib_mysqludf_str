/* -*- coding: utf-8; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: t -*- */

// This code and all comments, written by Daniel Trebbien, are hereby entered into the Public Domain by their author.

#include <algorithm>

#define BOOST_TEST_DYN_LINK 1
#define BOOST_TEST_MODULE "string_utils tests"
#include <boost/test/unit_test.hpp>

#include "../../string_utils.h"

BOOST_AUTO_TEST_CASE(test_x_strlcpy)
{
	char buf[10];

	// No room for the NUL terminator
	buf[0] = 'a';
	BOOST_CHECK_EQUAL(x_strlcpy(buf, "test", 0), 4);
	BOOST_CHECK_EQUAL(buf[0], 'a');

	// Only room for the NUL terminator and `*src` is a NUL character
	buf[0] = buf[1] = 'a';
	BOOST_CHECK_EQUAL(x_strlcpy(buf, "\0", 1), 0);
	BOOST_CHECK_EQUAL(buf[0], '\0');
	BOOST_CHECK_EQUAL(buf[1], 'a');

	// `*src` is a NUL character and `dest` has space for characters other than the NUL terminator.
	buf[0] = buf[1] = 'a';
	BOOST_CHECK_EQUAL(x_strlcpy(buf, "\0", 10), 0);
	BOOST_CHECK_EQUAL(buf[0], '\0');
	BOOST_CHECK_EQUAL(buf[1], 'a');

	// Only room for the NUL terminator and `*src` is not a NUL character
	buf[0] = buf[1] = 'a';
	BOOST_CHECK_EQUAL(x_strlcpy(buf, "test", 1), 4);
	BOOST_CHECK_EQUAL(buf[0], '\0');
	BOOST_CHECK_EQUAL(buf[1], 'a');

	// Only room to copy one character from `src`
	std::fill(buf, buf + (sizeof buf), 'a');
	BOOST_CHECK_EQUAL(x_strlcpy(buf, "test", 2), 4);
	BOOST_CHECK_EQUAL(buf[0], 't');
	BOOST_CHECK_EQUAL(buf[1], '\0');
	BOOST_CHECK_EQUAL(buf[2], 'a');

	// Only room to copy three characters from `src`
	std::fill(buf, buf + (sizeof buf), 'a');
	BOOST_CHECK_EQUAL(x_strlcpy(buf, "test", 4), 4);
	BOOST_CHECK_EQUAL(buf[0], 't');
	BOOST_CHECK_EQUAL(buf[1], 'e');
	BOOST_CHECK_EQUAL(buf[2], 's');
	BOOST_CHECK_EQUAL(buf[3], '\0');
	BOOST_CHECK_EQUAL(buf[4], 'a');

	// `dest` is just big enough to copy `src` and the NUL terminator.
	std::fill(buf, buf + (sizeof buf), 'a');
	BOOST_CHECK_EQUAL(x_strlcpy(buf, "test", 5), 4);
	BOOST_CHECK_EQUAL(buf[0], 't');
	BOOST_CHECK_EQUAL(buf[1], 'e');
	BOOST_CHECK_EQUAL(buf[2], 's');
	BOOST_CHECK_EQUAL(buf[3], 't');
	BOOST_CHECK_EQUAL(buf[4], '\0');
	BOOST_CHECK_EQUAL(buf[5], 'a');

	// `dest` has space for more than `strlen(src) + 1` characters.
	std::fill(buf, buf + (sizeof buf), 'a');
	BOOST_CHECK_EQUAL(x_strlcpy(buf, "test", 10), 4);
	BOOST_CHECK_EQUAL(buf[0], 't');
	BOOST_CHECK_EQUAL(buf[1], 'e');
	BOOST_CHECK_EQUAL(buf[2], 's');
	BOOST_CHECK_EQUAL(buf[3], 't');
	BOOST_CHECK_EQUAL(buf[4], '\0');
	BOOST_CHECK_EQUAL(buf[5], 'a');
}

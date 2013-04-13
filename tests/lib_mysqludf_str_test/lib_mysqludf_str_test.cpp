/* -*- coding: utf-8; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: t -*- */
/**
 * Copyright 2013 Daniel Trebbien
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef _WIN32
#include <Windows.h>
#endif

#include <cstddef>
#include <cstdlib>
#include <mysql.h>
#include <mysqld_error.h>
#include <set>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <boost/scope_exit.hpp>

#ifdef _WIN32
#include <conio.h>
#include <iostream>

static char * getpass(const char *prompt) throw()
{
	static char pass_buf[1025];

	std::size_t i = 0;
	int c;

	std::cerr << prompt << std::flush;
	do {
		if (i >= (sizeof pass_buf)) {
			return NULL;
		}

		c = _getch();
		if (c == '\r' || c == '\n') {
			pass_buf[i] = '\0';
			break;
		} else {
			pass_buf[i++] = static_cast<char>(c);
		}
	} while (1);

	std::cerr << std::endl;
	return pass_buf;
}
#else
#include <unistd.h>
#endif

#define BOOST_TEST_DYN_LINK 1
#define BOOST_TEST_MODULE "lib_mysqludf_str tests"
#include <boost/test/unit_test.hpp>

#include "../../config.h"

static const char *g_mysql_host = "localhost";
static const char *g_mysql_user = "test";
static const char *g_mysql_password = NULL;
static const char *g_mysql_dbname = "test";

class init_mysqlclient_lib
{
public:
	init_mysqlclient_lib(int argc = 0, char **argv = NULL, char **groups = NULL)
	{
		if (mysql_library_init(argc, argv, groups) != 0) {
			throw std::runtime_error("failed to initialize the MySQL client library");
		}
	}

	~init_mysqlclient_lib() throw()
	{
		mysql_library_end();
	}
};

class get_mysql_password
{
public:
	get_mysql_password() throw()
	{
		char prompt[79];
#ifdef _WIN32
		_snprintf_s(prompt, _TRUNCATE,
#else
		snprintf(prompt, sizeof prompt,
#endif
				"Enter password for '%s'@'%s': ", g_mysql_user, g_mysql_host);
		g_mysql_password = getpass(prompt);
	}
};

BOOST_GLOBAL_FIXTURE(init_mysqlclient_lib);
BOOST_GLOBAL_FIXTURE(get_mysql_password);

BOOST_AUTO_TEST_CASE(test_str_numtowords)
{
	MYSQL *pconn = mysql_init(NULL);
	BOOST_SCOPE_EXIT( (pconn) ) {
		mysql_close(pconn);
	} BOOST_SCOPE_EXIT_END

	if (! mysql_real_connect(pconn, g_mysql_host, g_mysql_user, g_mysql_password, g_mysql_dbname, 0, NULL, 0)) {
		BOOST_FAIL("failed to connect");
	}

	if (mysql_query(pconn, "SELECT str_numtowords(123456) AS price") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pprice_field = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pprice_field->name, "price");
			BOOST_CHECK_EQUAL(pprice_field->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "one hundred twenty-three thousand four hundred fifty-six");
		}
	}

	// Check for regressions of the bug reported by Bernard Sang where the result was incorrect
	// for 100000 (should be "one hundred thousand", but was "one hundred").
	if (mysql_query(pconn, "SELECT str_numtowords(100000)") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "one hundred thousand");
		}
	}

	if (mysql_query(pconn, "CREATE TEMPORARY TABLE numbers (id INT NOT NULL AUTO_INCREMENT, num INT, PRIMARY KEY (id))") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "INSERT INTO numbers(id, num) VALUES (1, -67423), (2, NULL)") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "SELECT str_numtowords(num) FROM numbers ORDER BY id") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pfield = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pfield->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "negative sixty-seven thousand four hundred twenty-three");

			prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), static_cast<const char *>(NULL));
		}
	}
}

BOOST_AUTO_TEST_CASE(test_str_rot13)
{
	MYSQL *pconn = mysql_init(NULL);
	BOOST_SCOPE_EXIT( (pconn) ) {
		mysql_close(pconn);
	} BOOST_SCOPE_EXIT_END

	if (! mysql_real_connect(pconn, g_mysql_host, g_mysql_user, g_mysql_password, g_mysql_dbname, 0, NULL, 0)) {
		BOOST_FAIL("failed to connect");
	}

	if (mysql_query(pconn, "SELECT str_rot13('secret message') AS crypted") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pcrypted_field = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pcrypted_field->name, "crypted");
			BOOST_CHECK_EQUAL(pcrypted_field->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "frperg zrffntr");
		}
	}
}

BOOST_AUTO_TEST_CASE(regression_test_1)
{
	// http://mysqludf.lighthouseapp.com/projects/71827/tickets/1-library-only-supports-literals-not-columns

	MYSQL *pconn = mysql_init(NULL);
	BOOST_SCOPE_EXIT( (pconn) ) {
		mysql_close(pconn);
	} BOOST_SCOPE_EXIT_END

	if (! mysql_real_connect(pconn, g_mysql_host, g_mysql_user, g_mysql_password, g_mysql_dbname, 0, NULL, 0)) {
		BOOST_FAIL("failed to connect");
	}

	if (mysql_query(pconn, "CREATE TEMPORARY TABLE email (email_id INT NOT NULL AUTO_INCREMENT, email_address VARCHAR(255), PRIMARY KEY (email_id))") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "INSERT INTO email(email_id, email_address) VALUES (1, NULL), (2, 'dtrebbien@gmail.com')") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "SELECT str_rot13(email_address) FROM email ORDER BY email_id") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pfield = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pfield->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), static_cast<const char *>(NULL));

			prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "qgeroovra@tznvy.pbz");
		}
	}
}

BOOST_AUTO_TEST_CASE(test_str_shuffle)
{
	MYSQL *pconn = mysql_init(NULL);
	BOOST_SCOPE_EXIT( (pconn) ) {
		mysql_close(pconn);
	} BOOST_SCOPE_EXIT_END

	if (! mysql_real_connect(pconn, g_mysql_host, g_mysql_user, g_mysql_password, g_mysql_dbname, 0, NULL, 0)) {
		BOOST_FAIL("failed to connect");
	}

	if (mysql_query(pconn, "SELECT str_shuffle('shake me!') AS nonsense") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pnonsense_field = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pnonsense_field->name, "nonsense");
			BOOST_CHECK_EQUAL(pnonsense_field->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			const char *string0 = static_cast<const char *>(prow[0]);
			unsigned long *lengths = mysql_fetch_lengths(pres);
			const std::multiset<char> found_chars(string0, string0 + lengths[0]);
			const char lit0[] = "shake me!";
			const std::multiset<char> expected_chars(lit0, lit0 + (sizeof lit0) - 1);
			BOOST_CHECK_EQUAL_COLLECTIONS(found_chars.begin(), found_chars.end(), expected_chars.begin(), expected_chars.end());
		}
	}
}

BOOST_AUTO_TEST_CASE(test_str_translate)
{
	MYSQL *pconn = mysql_init(NULL);
	BOOST_SCOPE_EXIT( (pconn) ) {
		mysql_close(pconn);
	} BOOST_SCOPE_EXIT_END

	if (! mysql_real_connect(pconn, g_mysql_host, g_mysql_user, g_mysql_password, g_mysql_dbname, 0, NULL, 0)) {
		BOOST_FAIL("failed to connect");
	}

	if (mysql_query(pconn, "SELECT str_translate('a big string', 'ab', 'xy') AS translated") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *ptranslated_field = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(ptranslated_field->name, "translated");
			BOOST_CHECK_EQUAL(ptranslated_field->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "x yig string");
		}
	}

	if (mysql_query(pconn, "CREATE TEMPORARY TABLE strings (id INT NOT NULL AUTO_INCREMENT, str VARCHAR(255), PRIMARY KEY (id))") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "INSERT INTO strings(id, str) VALUES (1, 'a big string'), (2, NULL)") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "SELECT str_translate(str, 'ab', 'xy') FROM strings ORDER BY id") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pfield = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pfield->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "x yig string");

			prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), static_cast<const char *>(NULL));
		}
	}
}

BOOST_AUTO_TEST_CASE(test_str_ucfirst)
{
	MYSQL *pconn = mysql_init(NULL);
	BOOST_SCOPE_EXIT( (pconn) ) {
		mysql_close(pconn);
	} BOOST_SCOPE_EXIT_END

	if (! mysql_real_connect(pconn, g_mysql_host, g_mysql_user, g_mysql_password, g_mysql_dbname, 0, NULL, 0)) {
		BOOST_FAIL("failed to connect");
	}

	if (mysql_query(pconn, "SELECT str_ucfirst('sAmple strinG') AS capitalized") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pcapitalized_field = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pcapitalized_field->name, "capitalized");
			BOOST_CHECK_EQUAL(pcapitalized_field->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "SAmple strinG");
		}
	}

	if (mysql_query(pconn, "CREATE TEMPORARY TABLE strings (id INT NOT NULL AUTO_INCREMENT, str VARCHAR(255), PRIMARY KEY (id))") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "INSERT INTO strings(id, str) VALUES (1, 'sAmple strinG'), (2, NULL)") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "SELECT str_ucfirst(str) FROM strings ORDER BY id") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pfield = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pfield->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "SAmple strinG");

			prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), static_cast<const char *>(NULL));
		}
	}
}

BOOST_AUTO_TEST_CASE(test_str_ucwords)
{
	MYSQL *pconn = mysql_init(NULL);
	BOOST_SCOPE_EXIT( (pconn) ) {
		mysql_close(pconn);
	} BOOST_SCOPE_EXIT_END

	if (! mysql_real_connect(pconn, g_mysql_host, g_mysql_user, g_mysql_password, g_mysql_dbname, 0, NULL, 0)) {
		BOOST_FAIL("failed to connect");
	}

	if (mysql_query(pconn, "SELECT str_ucwords('sAmple strinG') AS capitalized") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pcapitalized_field = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pcapitalized_field->name, "capitalized");
			BOOST_CHECK_EQUAL(pcapitalized_field->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "SAmple StrinG");
		}
	}

	if (mysql_query(pconn, "CREATE TEMPORARY TABLE strings (id INT NOT NULL AUTO_INCREMENT, str VARCHAR(255), PRIMARY KEY (id))") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "INSERT INTO strings(id, str) VALUES (1, 'sAmple strinG'), (2, NULL)") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "SELECT str_ucwords(str) FROM strings ORDER BY id") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pfield = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pfield->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "SAmple StrinG");

			prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), static_cast<const char *>(NULL));
		}
	}
}

BOOST_AUTO_TEST_CASE(test_str_xor)
{
	MYSQL *pconn = mysql_init(NULL);
	BOOST_SCOPE_EXIT( (pconn) ) {
		mysql_close(pconn);
	} BOOST_SCOPE_EXIT_END

	if (! mysql_real_connect(pconn, g_mysql_host, g_mysql_user, g_mysql_password, g_mysql_dbname, 0, NULL, 0)) {
		BOOST_FAIL("failed to connect");
	}

	if (mysql_query(pconn, "SELECT UPPER(HEX(str_xor(UNHEX('0E33'), UNHEX('E0')))) AS result UNION SELECT UPPER(HEX(str_xor('Wiki', UNHEX('F3F3F3F3'))))") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *presult_field = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(presult_field->name, "result");
			BOOST_CHECK_EQUAL(presult_field->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "EE33");

			prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(static_cast<const char *>(prow[0]), "A49A989A");
		}
	}

	if (mysql_query(pconn, "CREATE TEMPORARY TABLE strings (id INT NOT NULL AUTO_INCREMENT, str VARBINARY(255), PRIMARY KEY (id))") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "INSERT INTO strings(id, str) VALUES (1, UNHEX('0E33')), (3, NULL)") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	}

	if (mysql_query(pconn, "SELECT UPPER(HEX(str_xor(str, UNHEX('E0')))), UPPER(HEX(str_xor(UNHEX('E0'), str))) FROM strings ORDER BY id") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pfield = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pfield->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(prow[0], "EE33");
			BOOST_CHECK_EQUAL(prow[1], "EE33");

			prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(prow[0], static_cast<const char *>(NULL));
			BOOST_CHECK_EQUAL(prow[1], static_cast<const char *>(NULL));
		}
	}
}

BOOST_AUTO_TEST_CASE(test_str_srand)
{
	MYSQL *pconn = mysql_init(NULL);
	BOOST_SCOPE_EXIT( (pconn) ) {
		mysql_close(pconn);
	} BOOST_SCOPE_EXIT_END

	if (! mysql_real_connect(pconn, g_mysql_host, g_mysql_user, g_mysql_password, g_mysql_dbname, 0, NULL, 0)) {
		BOOST_FAIL("failed to connect");
	}

	if (mysql_query(pconn, "SELECT LENGTH(str_srand(123)) AS len") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *plen_field = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(plen_field->name, "len");
			BOOST_CHECK_EQUAL(plen_field->type, MYSQL_TYPE_LONGLONG);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(std::atoi(prow[0]), 123);
		}
	}

	char sql0[64];
#ifdef _WIN32
	_snprintf_s(sql0, _TRUNCATE,
#else
	snprintf(sql0, sizeof sql0,
#endif
			"SELECT str_srand(%lld)", static_cast<long long>(MAX_RANDOM_BYTES + 1));

	BOOST_CHECK_NE(mysql_query(pconn, sql0), 0);
	BOOST_CHECK_EQUAL(mysql_errno(pconn), ER_CANT_INITIALIZE_UDF);
}

BOOST_AUTO_TEST_CASE(test_lib_mysqludf_str_info)
{
	MYSQL *pconn = mysql_init(NULL);
	BOOST_SCOPE_EXIT( (pconn) ) {
		mysql_close(pconn);
	} BOOST_SCOPE_EXIT_END

	if (! mysql_real_connect(pconn, g_mysql_host, g_mysql_user, g_mysql_password, g_mysql_dbname, 0, NULL, 0)) {
		BOOST_FAIL("failed to connect");
	}

	if (mysql_query(pconn, "SELECT lib_mysqludf_str_info() AS info") != 0) {
		BOOST_ERROR(mysql_error(pconn));
	} else {
		MYSQL_RES *pres = mysql_store_result(pconn);
		if (pres == NULL) {
			BOOST_ERROR(mysql_error(pconn));
		} else {
			BOOST_SCOPE_EXIT( (pres) ) {
				mysql_free_result(pres);
			} BOOST_SCOPE_EXIT_END

			MYSQL_FIELD *pinfo_field = mysql_fetch_field(pres);
			BOOST_CHECK_EQUAL(pinfo_field->name, "info");
			BOOST_CHECK_EQUAL(pinfo_field->type, MYSQL_TYPE_VAR_STRING);

			MYSQL_ROW prow = mysql_fetch_row(pres);
			BOOST_REQUIRE_NE(prow, static_cast<MYSQL_ROW>(NULL));
			BOOST_CHECK_EQUAL(prow[0], "lib_mysqludf_str version 0.5");
		}
	}
}

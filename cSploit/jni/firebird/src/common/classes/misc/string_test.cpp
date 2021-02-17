char lbl[] = "0123456789";

//#define CHECK_FATAL_RANGE_EXCEPTION
//Don't modify 3 lines upper from this - they are used in file read test
//If you plan to check range exception, you may uncomment it -
//anyway file test should not happen

/*
 *	PROGRAM:	Class library integrity tests
 *	MODULE:		string_test.cpp
 *	DESCRIPTION:	test class Firebird::string
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#if defined(FIRESTR) && defined(DEV_BUILD)
#define FULL_FIRE
#endif

#ifdef FIRESTR
#define NAME "Firebird::string"
#include "../common/classes/fb_string.h"

using namespace Firebird;
#else
#define NAME "std::basic_string"
#include <string>
typedef std::basic_string<char> string;
typedef string AbstractString;
#endif

#include <time.h>

#ifdef DEV_BUILD
void CheckStr(const AbstractString& check, const char* want, int line)
{
	if (strlen(check.c_str()) != check.length()) {
		printf("Length error at %d\n\n", line);
	}
	if (strcmp(check.c_str(), want)) {
		printf("Wanted >%s<,\n   got >%s< at %d\n\n", want, check.c_str(), line);
	}
}
#define validate(a, b) CheckStr(a, b, __LINE__)
#else
#define validate(a, b)
#define check(a, b)
#endif

void test()
{
	{
		string a;
		validate(a, "");
		a = lbl;
		string b = a;
		validate(b, lbl);
		string f = "0123456789";
		validate(f, lbl);
		string g("0123456789", 5);
		validate(g, "01234");
		string h(5, '7');
		validate(h, "77777");
#ifdef FULL_FIRE
		string i('7');
		validate(i, "7");
#endif
		string j(&lbl[3], &lbl[5]);
		validate(j, "34");
	}

	{
		string a = lbl;
		string b;
		b = a;
		validate(b, lbl);
		b = lbl;
		validate(b, lbl);
		a = 'X';
		validate(a, "X");

		a = b;
		for (string::iterator x = b.begin(); x < b.end(); x++)
			*x = 'u';
		validate(a, lbl);
		validate(b, "uuuuuuuuuu");

		char y[20], *z = y;
		const string c = a;
		for (string::const_iterator x1 = c.begin(); x1 < c.end(); x1++)
			*z++ = *x1;
		*z = 0;
		b = y;
		validate(b, lbl);
	}

#ifdef FULL_FIRE
	{
		const string a = lbl;
		string b = a.at(5);
		validate(b, "5");
	}

	{
		string a = lbl;
		string b = a.at(5);
		validate(b, "5");
	}
#endif

	// conflict with J. requirement to string class - operator const char*
	//	{
	//	string a = lbl;
	//	char c = a[5];
	//	a[5] = 'u';
	//	validate(a, "01234u6789");
	//	}
	{
		string a = lbl;
		char c = a[5];  // via operator const char*
		a.at(5) = 'u';
		a.at(7) = c;
		validate(a, "01234u6589");
	}

#ifdef CHECK_FATAL_RANGE_EXCEPTION
	{
		const string a = lbl;
		string b = a.at(15);
	}
#endif

	{
		string a = lbl;
		a.resize(15, 'u');
		validate(a, "0123456789uuuuu");
	}

	{
		string a;
		string x = lbl;
		x += lbl;
		x += lbl;
		x += lbl;
		x += lbl;
		x += lbl;
		x += lbl;
		x += lbl;
		x += lbl;
		x += lbl;
		x += lbl;
		x += lbl; //120 bytes
		a = x;
		validate(a, x.c_str());

		x = lbl;
		x += lbl;
		x += lbl;
		x += lbl; //40 bytes
		a = x;
		validate(a, x.c_str());
	}

	{
		string a = lbl;
		const string b = a;
		a += b;
		validate(a, "01234567890123456789");

		a = lbl;
		a += lbl;
		validate(a, "01234567890123456789");

		a = lbl;
		a += 'u';
		validate(a, "0123456789u");
	}

	{
		string a, b, c;

		a = "uuu";
		b = lbl;
		c = a + b;
		validate(c, "uuu0123456789");

		c = a + lbl;
		validate(c, "uuu0123456789");

		c = b + 'u';
		validate(c, "0123456789u");

		c = lbl + a;
		validate(c, "0123456789uuu");

		c = 'u' + b;
		validate(c, "u0123456789");

		validate(a, "uuu");
		validate(b, lbl);
	}

	{
		string a = lbl;
		const string b = a;
		a.append(b);
		validate(a, "01234567890123456789");

		a = lbl;
		a.append(lbl);
		validate(a, "01234567890123456789");

		a = lbl;
		a.append(lbl, 6);
		validate(a, "0123456789012345");

		a = lbl;
		a.append(b, 3, 2);
		validate(a, "012345678934");

		a = lbl;
		a.append(b, 3, 20);
		validate(a, "01234567893456789");

		a = lbl;
		a.append(3, 'u');
		validate(a, "0123456789uuu");

		a = lbl;
		a.append(b.begin(), b.end());
		validate(a, "01234567890123456789");

		a = lbl;
		a.append("Something reaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaally long");
		validate(a, "0123456789Something reaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaally long");
		a = lbl;
		validate(a, lbl);

		string c = lbl;
		c += lbl;
		c += lbl;
		c += lbl;
		a = lbl;
		a.append("Something reaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaally long");
		validate(a, "0123456789Something reaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaally long");
		a = c;
		validate(a, c.c_str());
	}

	{
		string a, b;
		b = lbl;

		a.assign(3, 'u');
		validate(a, "uuu");

		a.assign(lbl);
		validate(a, lbl);

		a.assign(lbl, 2);
		validate(a, "01");

		a.assign(b, 3, 3);
		validate(a, "345");

		a.assign(b);
		validate(a, lbl);

		a = "";
		validate(a, "");
		string::iterator x = b.begin();
		string::iterator y = b.end();
		a.assign(x, y);
		validate(a, lbl);
	}

	{
		string a, b = lbl;

		a = lbl;
		a.insert(5, 3, 'u');
		validate(a, "01234uuu56789");

		a = lbl;
		a.insert(3, lbl);
		validate(a, "01201234567893456789");

		a = lbl;
		a.insert(4, lbl, 2);
		validate(a, "012301456789");

		a = lbl;
		a.insert(5, b, 3, 3);
		validate(a, "0123434556789");

		a = lbl;
		a.insert(5, b);
		validate(a, "01234012345678956789");

		a = lbl;
		a.insert(2, "Something reaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaally long");
		validate(a, "01Something reaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaally long23456789");

		a = lbl;
		string::iterator x = a.begin();
		x++;
		a.insert(x, 5, 'u');
		validate(a, "0uuuuu123456789");

		a = lbl;
		x = a.begin();
		x += 2;
		string::iterator f = b.begin();
		string::iterator t = b.end();
		f++; t--;
		a.insert(x, f, t);
		validate(a, "011234567823456789");

		a = lbl;
		a.erase();
		validate(a, "");

		a = lbl;
		a.erase(3, 6);
		validate(a, "0129");

		a = lbl;
		a.erase(3, 16);
		validate(a, "012");

		a = lbl;
		a.erase(3);
		validate(a, "012");

		a = lbl;
		x = a.begin();
		x += 3;
		a.erase(x);
		validate(a, "012456789");

		a = lbl;
		x = a.begin();
		x += 3;
		string::iterator y = a.end();
		y -= 2;
		a.erase(x, y);
		validate(a, "01289");
	}

	{
		string a;
		const string b = lbl;
		string::iterator f0, t0;
		string::const_iterator f, t;

		a = lbl;
		a.replace(5, 2, 3, 'u');
		validate(a, "01234uuu789");

		a = lbl;
		f0 = a.begin() + 5;
		t0 = f0 + 2;
		a.replace(f0, t0, 3, 'u');
		validate(a, "01234uuu789");

		a = lbl;
		a.replace(3, 3, lbl);
		validate(a, "01201234567896789");

		a = lbl;
		f0 = a.begin() + 3;
		t0 = f0 + 3;
		a.replace(f0, t0, lbl);
		validate(a, "01201234567896789");

		a = lbl;
		a.replace(4, 4, lbl, 2);
		validate(a, "01230189");

		a = lbl;
		f0 = a.begin() + 4;
		t0 = f0 + 4;
		a.replace(f0, t0, lbl, 2);
		validate(a, "01230189");

		a = lbl;
		a.replace(5, 10, b, 3, 3);
		validate(a, "01234345");

		a = lbl;
		f0 = a.begin() + 5;
		t0 = f0 + 10;
		f = b.begin() + 3;
		t = f + 3;
		a.replace(f0, t0, f, t);
		validate(a, "01234345");

		a = lbl;
		a.replace(5, 0, b);
		validate(a, "01234012345678956789");

		a = lbl;
		f0 = a.begin() + 5;
		t0 = f0;
		a.replace(f0, t0, b);
		validate(a, "01234012345678956789");

		a = lbl;
		a.replace(2, 1, "Something reaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaally long");
		validate(a, "01Something reaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaally long3456789");
	}

	{
		string a, b = lbl;
		char s[40];
		memset(s, 0, 40);

#ifdef FULL_FIRE
		b.copyTo(s, 5);
		a = s;
		validate(a, "0123");

		b.copyTo(s, 40);
		a = s;
		validate(a, lbl);
#endif

	//	a.swap(b);
	//	validate(b, "3456789");
	//	validate(a, lbl);
	}

#ifdef DEV_BUILD
#undef check
#define check(a, b) if (a != b) printf("Wanted %d got %d at %d\n\n", b, a, __LINE__)
#endif

	{
		string a = "012345uuu345678";
						//	 9
		string b = "345";

		check(a.find(b), 3);
		check(a.find("45"), 4);
		check(a.find('5'), 5);
		check(a.find("ZZ"), string::npos);

		check(a.rfind(b), 9);
		check(a.rfind("45"), 10);
		check(a.rfind('5'), 11);
		check(a.rfind("ZZ"), string::npos);

		check(a.find("45", 8), 10);

		check(a.find_first_of("aub"), 6);
		check(a.find_first_of(b), 3);
		check(a.find_first_of("54"), 4);
		check(a.find_first_of('5'), 5);
		check(a.find_first_of("ZZ"), string::npos);

		check(a.find_last_of("aub"), 8);
		check(a.find_last_of(b), 11);
		check(a.find_last_of("54"), 11);
		check(a.find_last_of('5'), 11);
		check(a.find_last_of("ZZ"), string::npos);

		check(a.find_first_of("45", 8), 10);

		b = "010";
		check(a.find_first_not_of("aub"), 0);
		check(a.find_first_not_of(b), 2);
		check(a.find_first_not_of("0102"), 3);
		check(a.find_first_not_of('0'), 1);
		check(a.find_first_not_of(a), string::npos);

		b = "878";
		check(a.find_last_not_of("aub"), 14);
		check(a.find_last_not_of(b), 12);
		check(a.find_last_not_of("78"), 12);
		check(a.find_last_not_of('8'), 13);
		check(a.find_last_not_of(a), string::npos);

		check(a.find_first_not_of("u345", 8), 12);
	}

	{
		string a = lbl;
		string b;

		b = a.substr(3, 4);
		validate(b, "3456");

		b = a.substr(5, 20);
		validate(b, "56789");

#ifdef FULL_FIRE
		b = a.substr(50, 20);
		validate(b, "");
#endif
	}

#ifdef FULL_FIRE
	{
		string b;
		FILE *x = fopen("string_test.cpp", "rt");
		b.LoadFromFile(x);
		validate(b, "char lbl[] = \"0123456789\";");
		b.LoadFromFile(x);
		validate(b, "");
		b.LoadFromFile(x);
		validate(b, "//#define CHECK_FATAL_RANGE_EXCEPTION");
		fclose(x);
	}
#endif

	{
		string a = "Something moderaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaately long";
		string a1 = a;
		string b = a + a1;
		string b1 = b;
		string c = b + b1;
		string d = c;
		string e = c;
		string f = c;
		string g = c;
		string h = c;
		string i = c;
		string j = c;
		string k = c;
		string l = c;
		string m = c;
		string n = c;
		string o = c;
		string p = c;
	}

#ifdef DEV_BUILD
#undef check
#define check(get, want) if ( (get < 0 ? -1 : get > 0 ? 1 : 0) != want)\
	printf("Wanted %d got %d at %d\n\n", want, get, __LINE__)
#endif

#ifdef FULL_FIRE
	{
		PathName c = "Aa";
		PathName d = "AB";

		check(c.compare(d), -1);
	}
#endif

	{
		string a = lbl;
		string b = a;
		string c = "Aaa";
		string d = "AB";
		string e = "Aa";

		check(a.compare(b), 0);
		check(a.compare(c), -1);
		check(c.compare(a), 1);

		check(c.compare(d), 1);
		check(c.compare(e), 1);

		check(a.compare(1, 10, b), 1);
		check(a.compare(1, 10, b, 1, 10), 0);
		check(a.compare(lbl), 0);
		check(a.compare(1, 3, lbl + 1, 3), 0);
	}

#ifdef FULL_FIRE
	{
		string a = "  011100   ", b;

		b = a;
		b.ltrim();
		validate(b, "011100   ");

		b = a;
		b.rtrim();
		validate(b, "  011100");

		b = a;
		b.trim(" 0");
		validate(b, "111");

		b = a;
		b.alltrim("02 ");
		validate(b, "111");

		b = a;
		b.trim("012");
		validate(b, "  011100   ");

		validate(a, "  011100   ");
	}

	{
		string a = lbl;
		a += '\377';
		string b = a;
		a += " ";
		a.rtrim();
		validate(a, b.c_str());
	}

	{
		string a = "AaBbCc", b;

		b = a;
		b.lower();
		validate(b, "aabbcc");

		b = a;
		b.upper();
		validate(b, "AABBCC");

		validate(a, "AaBbCc");
	}
#endif
}

clock_t t;

void start()
{
	t = clock();
}

#ifdef DEV_BUILD
#define TEST_ITEMS 1
#else
#define TEST_ITEMS 100000
#endif

void report()
{
	clock_t d = clock();
	printf("Test of %d iterations with %s took %d milliseconds.\n",
		TEST_ITEMS, NAME, (int) (d - t) * 1000 / CLOCKS_PER_SEC);
}

int main(int ac, char **av)
{
	int n = TEST_ITEMS;
	start();
	while (n--) {
		test();
	}
	report();
#if defined(DEV_BUILD) && defined(FIRESTR)
	getDefaultMemoryPool()->print_contents(stdout, false);
#endif
	printf("Press enter to continue\n");
	getchar();
	return 0;
}

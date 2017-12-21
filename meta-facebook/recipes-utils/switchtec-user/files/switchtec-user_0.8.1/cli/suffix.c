////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 PMC-Sierra, Inc.
// Copyright (c) 2017, Microsemi Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//
//   Author: Logan Gunthorpe
//
//   Date:   Oct 23 2014
//
//   Description:
//     Functions for dealing with number suffixes
//
////////////////////////////////////////////////////////////////////////

#include "suffix.h"

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

static struct si_suffix {
	double magnitude;
	const char *suffix;
} si_suffixes[] = {
	{1e15, "P"},
	{1e12, "T"},
	{1e9, "G"},
	{1e6, "M"},
	{1e3, "k"},
	{1e0, ""},
	{1e-3, "m"},
	{1e-6, "u"},
	{1e-9, "n"},
	{1e-12, "p"},
	{1e-15, "f"},
	{0}
};

const char *suffix_si_get(double *value)
{
	struct si_suffix *s;

	for (s = si_suffixes; s->magnitude != 0; s++) {
		if (*value >= s->magnitude) {
			*value /= s->magnitude;
			return s->suffix;
		}
	}

	return "";
}

static struct binary_suffix {
	int shift;
	const char *suffix;
} binary_suffixes[] = {
	{50, "Pi"},
	{40, "Ti"},
	{30, "Gi"},
	{20, "Mi"},
	{10, "Ki"},
	{0, ""}
};

const char *suffix_binary_get(long long *value)
{
	struct binary_suffix *s;

	for (s = binary_suffixes; s->shift != 0; s++) {
		if (llabs(*value) >= (1LL << s->shift)) {
			*value =
			    (*value + (1 << (s->shift - 1))) / (1 << s->shift);
			return s->suffix;
		}
	}

	return "";
}

const char *suffix_dbinary_get(double *value)
{
	struct binary_suffix *s;

	for (s = binary_suffixes; s->shift != 0; s++) {
		if (fabs(*value) >= (1LL << s->shift)) {
			*value = *value / (1 << s->shift);
			return s->suffix;
		}
	}

	return "";
}

long long suffix_binary_parse(const char *value)
{
	char *suffix;
	errno = 0;
	long long ret = strtol(value, &suffix, 0);
	if (errno)
		return 0;

	struct binary_suffix *s;
	for (s = binary_suffixes; s->shift != 0; s++) {
		if (tolower(suffix[0]) == tolower(s->suffix[0])) {
			ret <<= s->shift;
			return ret;
		}
	}

	if (suffix[0] != '\0')
		errno = EINVAL;

	return ret;
}

/* -----------------------------------------------------------------------------
 * Author list (in alphabetic order):
 * 
 *        Eduardo Quiñones <eduardo.quinones@bsc.es>
 *        Sara Royuela <sara.royuela@bsc.es>
 *        Roberto Vargas <roberto.vargas@bsc.es>
 * 
 * -----------------------------------------------------------------------------
 * (C) Copyright 2016 Barcelona Supercomputing Center
 * 
 * This file is part of boxer (Compilation flow) of the P-SOCRATES project.
 *
 * Boxer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Boxer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with boxer. If not, see <http://www.gnu.org/licenses/>.
 * -----------------------------------------------------------------------------
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boxer.h"

static void
outmem(void)
{
	die("out of memory");
}

void *
xmalloc(size_t len)
{
	void *p = malloc(len);
	if (p == NULL)
		outmem();
	return p;
}

void *
xcalloc(size_t nelem, size_t size)
{
	void *p = calloc(nelem, size);
	if (p == NULL)
		outmem();
	return p;
}

void *
xrealloc(void *p, size_t len)
{
	p = realloc(p, len);
	if (p == NULL)
		outmem();
	return p;
}

char *
xstrdup(char *s)
{
	size_t len = strlen(s) + 1;
	return memcpy(xmalloc(len), s, len);
}


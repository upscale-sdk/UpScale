/*--------------------------------------------------------------------
  (C) Copyright 2006-2012 Barcelona Supercomputing Center
                          Centro Nacional de Supercomputacion
  
  This file is part of Mercurium C/C++ source-to-source compiler.
  
  See AUTHORS file in the top level directory for information
  regarding developers and contributors.
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.
  
  Mercurium C/C++ source-to-source compiler is distributed in the hope
  that it will be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the GNU Lesser General Public License for more
  details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with Mercurium C/C++ source-to-source compiler; if
  not, write to the Free Software Foundation, Inc., 675 Mass Ave,
  Cambridge, MA 02139, USA.
--------------------------------------------------------------------*/



/*
<testinfo>
test_generator="config/mercurium-hlt run"
</testinfo>
*/

#include <stdlib.h>
#include <string.h>

void foo(int *a, int N)
{
    int i;
    memset(a, 0, sizeof(a[0])* N);

#pragma hlt unroll(33)
    for (i=0; i<N; i+=7)
    {
        a[i] = 1;
    }

    for (i=0; i<N; i+=7)
    {
        if (a[i] != 1) abort();
        a[i] = 0;
    }

    for (i=0; i<N; i++)
    {
        if (a[i] != 0) abort();
    }

    memset(a, 0, sizeof(a[0])* N);
#pragma hlt unroll(33)
    for (i=N - 1; i>=0; i+=-7)
    {
        a[i] = 1;
    }

    for (i=N - 1; i>=0; i+=-7)
    {
        if (a[i] != 1) abort();
        a[i] = 0;
    }

    for (i=0; i<N; i++)
    {
        if (a[i] != 0) abort();
    }
}

enum { SIZE = 200 };
int x[SIZE];

int main(int argc, char *argv[])
{
    foo(x, SIZE);

    return 0;
}

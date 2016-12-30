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

#include <stdio.h>
#include <unistd.h>
#include <omp.h>

enum { N = 4, BS = 8 };

typedef unsigned long long int (*p_block_t)[BS];

#define SIZE (sizeof(square)/sizeof(long long))

#pragma omp declare target
void process_corner(p_block_t block)
{
    int i, j;
    for (i = 0; i < BS; ++i)
    {
        for (j = 0; j < BS; ++j)
        {
            if (i == 0 && j == 0)
            {
                // Do nothing because this is a corner
            }
            else if (j == 0)
            {
                block[i][j] += block[i-1][j]; // north
            }
            else if (i == 0)
            {
                block[i][j] += block[i][j-1]; // west
            }
            else // j > 0 && i > 0
            {
                block[i][j] += block[i-1][j] // north
                    + block[i][j-1]          // west
                    + block[i-1][j-1];       // north-west
            }
        }
    }
}

void process_left_edge(p_block_t north, p_block_t block)
{
    int i, j;
    for (i = 0; i < BS; ++i)
    {
        for (j = 0; j < BS; ++j)
        {
            if (i == 0 && j == 0)
            {
                block[i][j] += north[BS-1][j]; // north
            }
            else if (j == 0)
            {
                block[i][j] += block[i-1][j]; // north
            }
            else if (i == 0)
            {
                block[i][j] += north[BS-1][j] // north
                    + block[i][j-1]           // west
                    + north[BS-1][j-1];       // north-west
            }
            else // j > 0 && i > 0
            {
                block[i][j] += block[i-1][j] // north
                    + block[i][j-1]          // west
                    + block[i-1][j-1];       // north-west
            }
        }
    }
}

void process_upper_edge(p_block_t west, p_block_t block)
{
    int i, j;
    for (i = 0; i < BS; ++i)
    {
        for (j = 0; j < BS; ++j)
        {
            if (i == 0 && j == 0)
            {
                block[i][j] += west[i][BS-1]; // west
            }
            else if (j == 0)
            {
                block[i][j] += block[i-1][j] // north
                    + west[i][BS-1] // west
                    + west[i-1][BS-1]; // north-west
            }
            else if (i == 0)
            {
                block[i][j] += block[i-1][j]; // west
            }
            else // j > 0 && i > 0
            {
                block[i][j] += block[i-1][j] // north
                    + block[i][j-1]          // west
                    + block[i-1][j-1];       // north-west
            }
        }
    }
}

void process_inner(p_block_t north,
        p_block_t west,
        p_block_t north_west,
        p_block_t block)
{
    int i, j;
    for (i = 0; i < BS; ++i)
    {
        for (j = 0; j < BS; ++j)
        {
            if (i == 0 && j == 0)
            {
                block[i][j] += north[BS-1][j] // north
                    + west[i][BS-1]           // west
                    + north_west[BS-1][BS-1]; // north-west
            }
            else if (j == 0)
            {
                block[i][j] += block[i-1][j] // north
                    + west[i][BS-1]    // west
                    + west[i-1][BS-1]; // north-west
            }
            else if (i == 0)
            {
                block[i][j] += north[BS-1][j] // north
                    + block[i][j-1]           // west
                    + north[BS-1][j-1];       // north-west
            }
            else // j > 0 && i > 0
            {
                block[i][j] += block[i-1][j] // north
                    + block[i][j-1]          // west
                    + block[i-1][j-1];       // north-west
            }
        }
    }
}
#pragma omp end declare target

unsigned long long int square[N][N][BS][BS];

unsigned long long wavefront(void)
{
    int i, j;
    for (i=0; i<N; ++i) {
        for (j=0; j<N; ++j) {
            if (j == 0 && i == 0)
            {         // top left corner block
                p_block_t block = square[i][j];

#pragma omp target \
                depend(inout: block[0:BS]) \
                map(tofrom : block[0:BS]) \
                nowait
                {
                    process_corner(block);
                }
            }
            else if (j == 0)
            {              // blocks in left edge
                p_block_t north = square[i-1][j];
                p_block_t block = square[i][j];

#pragma omp target \
                depend(in:  north[0:BS]) \
                depend(inout: block[0:BS]) \
                map(to : north[0:BS]) \
                map(tofrom : block[0:BS]) \
                nowait
                {
                    process_left_edge(north, block);
                }
            }
            else if (i == 0) {            // blocks in upper edge
                p_block_t west = square[i][j-1];
                p_block_t block = square[i][j];

#pragma omp target \
                depend(in: west[0:BS])  \
                depend(inout: block[0:BS]) \
                map(to : west[0:BS]) \
                map(tofrom : block[0:BS]) \
                nowait
                {
                    process_upper_edge(west, block);
                }
            }
            else
            {                        // internal blocks
                p_block_t north = square[i-1][j];
                p_block_t west = square[i][j-1];
                p_block_t north_west = square[i-1][j-1];
                p_block_t block = square[i][j];

#pragma omp target \
                depend(in: north[0:BS])   \
                depend(in: west[0:BS])   \
                depend(in: north_west[0:BS]) \
                depend(inout: block[0:BS]) \
                map(to : north[0:BS]) \
                map(to : west[0:BS]) \
                map(to : north_west[0:BS]) \
                map(tofrom : block[0:BS]) \
                nowait
                {
                    process_inner(north, west, north_west, block);
                }
            }
        }
    }
#pragma omp taskwait
    return square[N - 1][N - 1][BS - 1][BS - 1];
}


void init_single_block(int i, int j)
{
    int x, y;

    for (x = 0; x < BS; x++)
    {
        for (y = 0; y < BS; y++)
        {
            square[i][j][x][y] =  y + BS * (x + BS * ( j + N * i));
        }
    }
}

void init_blocks(void)
{
    int i, j;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            init_single_block(i, j);
        }
    }
}


int main(int argc, char *argv[])
{
    unsigned long long r;

    init_blocks();
#pragma omp parallel
    {
#pragma omp single
        {
            r = wavefront();
        }
    }

    fprintf(stderr, "result = %llu\n", r);
    return 0;
}

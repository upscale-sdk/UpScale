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


#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>


int
main(int argc, char *argv[])
{
	FILE *fp;
	int c;

	while ((c = getchar()) != EOF) {
		ungetc(c, stdin);
		if (!(fp = popen("dot -Tpng | page >/dev/null 2>&1", "w"))) {
			perror("opening pipe");
			exit(EXIT_FAILURE);
		}
		while ((c = getchar()) != '\f' && c != EOF)
			putc(c, fp);
		pclose(fp);
	}
	return 0;
}


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

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "boxer.h"

static int ngraph;
extern void printboxes(struct box *root, FILE *fp);
extern void printjdeps(FILE *fp);
extern void printdeps(FILE *fp);
char *output, *dotprog = "xdot";
int opt_transitive, opt_flow, opt_debug;
int opt_json, opt_free, opt_parent, opt_nodes;
int opt_show;

void
die(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	fputs("boxer:", stderr);
	vfprintf(stderr, fmt, va);
	va_end(va);
	putc('\n', stderr);
	if (output)
		remove(output);
	exit(EXIT_FAILURE);
}

static void
printflow(struct box *root)
{
	FILE *fp;
	char fname[40], cmd[BUFSIZ];

	sprintf(fname, "boxer%05d.dot", ngraph++);
	if ((fp = fopen(fname, "w")) == NULL)
		goto error2;
	fprintf(fp, "digraph G%d {\n", ngraph);
	printboxes(root, fp);
	fputs("}\n", fp);
	if (ferror(fp))
		goto error1;
	if (fclose(fp))
		goto error2;
	if (opt_show) {
		snprintf(cmd, sizeof(cmd), "%s %s", dotprog, fname);
		system(cmd);
	}
	return;

error1:
	fclose(fp);
error2:
	fprintf(stderr, "error writing debug file '%s'\n", fname);
}

static void
printjson(struct box *root)
{
	FILE *fp;
	char fname[40], cmd[BUFSIZ];

	sprintf(fname, "boxer%05d.dot", ngraph++);
	if ((fp = fopen(fname, "w")) == NULL)
		goto error2;

	fprintf(fp, "digraph G%d {\n", ngraph);
	printboxes(root, fp);
	printjdeps(fp);
	fputs("}\n", fp);
	if (ferror(fp))
		goto error1;
	if (fclose(fp))
		goto error2;
	if (opt_show) {
		snprintf(cmd, sizeof(cmd), "%s %s", dotprog, fname);
		system(cmd);
	}
	return;

error1:
	fclose(fp);
error2:
	fprintf(stderr, "error writing debug file '%s'\n", fname);
}

static void
printtdg(char* function, int tdg_id)
{
	char cmd[BUFSIZ];
	extern unsigned maxI, maxT;

	printf("digraph %s {\n", function);
        printf("\t0 [label=\"tdg_id=%d\", style=invis]\n"
               "\t0 [label=\"maxI=%d\", style=invis]\n"
	       "\t0 [label=\"maxT=%d\", style=invis]\n",
	       tdg_id,maxI, maxT);
	printdeps(stdout);
	puts("}");

// 	if (fclose(stdout))
// 		goto error;
// 	if (output && opt_show) {
// 		snprintf(cmd, sizeof(cmd), "%s %s", dotprog, output);
// 		system(cmd);
// 	}
	return;

// 	fclose(stdout);
error:
	fprintf(stderr, "error writing output file '%s'\n",
	        (output) ? output : "<stdout>");
}

void
usage(void)
{
	fputs("usage: boxer "
	      "[-d number][-x cmd][-p][-m][-n][-j][-t][-f][-o output without extension][file]\n",
	      stderr);
	exit(EXIT_FAILURE);
}

int
Isnumber(char *s)
{
	while (*s) {
		if (!isdigit(*s++))
			return 0;
	}
	return 1;
}

int
main(int argc, char *argv[])
{
	int complete, c;
	char *cp;
	struct box *root;
        struct boxes *json;
	char line[80];
        char *output_arg = NULL;
        extern unsigned maxT;

	for(++argv, --argc; *argv; --argc, ++argv) {
		if (argv[0][0] != '-' || argv[0][1] == '-')
			break;
		for (cp = &argv[0][1]; c = *cp; ++cp) {
			switch (c) {
			case 'o':
				if (!*++argv)
					usage();
				--argc;
                                // Check proper output name (no extension in the file name)
                                char *c = *argv;
                                const char *invalid_characters = ".";
                                while (*c)
                                {
                                    if (strchr(invalid_characters, *c))
                                        usage();
                                    c++;
                                }
				output_arg = *argv;
				break;
			case 'd':
				if (*++argv == NULL || !Isnumber(*argv))
					die("d flag needs a numeric parameter");
				--argc;
				if ((opt_debug = atoi(*argv)) > 3)
					die("incorrect debug level");
				break;
			case 'j':
				opt_json = 1;
				break;
			case 'p':
				opt_parent = 1;
				break;
			case 'x':
				if (argv[1] && argv[1][0] != '-') {
					dotprog = *++argv;
					--argc;
				}
				opt_show = 1;
				break;
			case 'f':
				opt_flow = 1;
				break;
			case 'm':
				opt_free = 1;
				break;
			case 'n':
				opt_nodes = 1;
				break;
			case 't':
				opt_transitive = 1;
				break;
			default:
				usage();
			}
		}
	}

	if (argc > 1)
		usage();

	initeval();
        
        int ntdgs = 0;
	if ((ntdgs = initjsonfile(*argv)) == -1) {
		return EXIT_FAILURE;
	}

	int i;
	for (i = 0; i < ntdgs; i++)
        {
            if (output_arg) {
                int out_size = strlen(output_arg) + 10; 
                output = (char*) malloc(out_size);
                strcpy(output, output_arg);
                char str[10];
                sprintf(str, "_%d.dot", i);
                strcat(output, str);
		if (!freopen(output, "w", stdout)) {
			printf("boxer: Opening output file. %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
            }

            static char function[MAXTOKEN];
            root = readtdgjson(function);
            if (opt_json)
                    printjson(root);
            if (opt_flow)
                    printflow(root);
            for (;;) {
                    complete = expandbox(root);
                    if (opt_flow)
                            printflow(root);
                    if (complete)
                            break;
// 		fputs("Box diagraman non complete\n"
// 		           "A new iteration? (y/n)\n",  stderr);
// 		fgets(line, sizeof(line), stdin);
// 		if (line[0] == 'n')
// 			break;
            }

            genleafs();
            expanddep();
            printtdg(function, i);

            reset_task_globals();
        }

	return 0;
}

/************************************************************************************/
/*                                                                                  */
/* Author: Bill DuPree                                                              */
/* Name: sudoku_solver.c                                                            */
/* Language: C                                                                      */
/* Inception: Feb. 25, 2006                                                         */
/* Copyright (C) August 17, 2008, All rights reserved.                              */
/*                                                                                  */
/* This is a program that solves Su Doku (aka Sudoku, Number Place, etc.) puzzles   */
/* primarily using deductive logic. It will only resort to trial-and-error and      */
/* backtracking approaches upon exhausting all of its deductive moves.              */
/*                                                                                  */
/* Puzzles must be of the standard 9x9 variety using the (ASCII) characters '1'     */
/* through '9' for the puzzle solution set. Puzzles should be submitted as 81       */
/* character strings which, when read left-to-right will fill a 9x9 Sudoku grid     */
/* from left-to-right and top-to-bottom. In the puzzle specification, the           */
/* characters 1 - 9 represent the puzzle "givens" or clues. Any other non-blank     */
/* character represents an unsolved cell.                                           */
/*                                                                                  */
/* The puzzle solving algorithm is contained in the source file "sudoku_engine.c"   */
/* Please see that file for a description of its operation.                         */
/*                                                                                  */
/* PROGRAM INVOCATION:                                                              */
/*                                                                                  */
/* This program is a console (or command line) based utility and has the following  */
/* usage:                                                                           */
/*                                                                                  */
/*      sudoku_solver {-p puzzle | -f <puzzle_file>} [-o <outfile>]                 */
/*              [-r <reject_file>] [-1][-a][-c][-d][-g][-m][-n][-s]                 */
/*                                                                                  */
/* where:                                                                           */
/*                                                                                  */
/*        -1      Search for first solution, otherwise all solutions are returned   */
/*        -a      Requests that the answer (solution) be printed                    */
/*        -c      Print a count of solutions for each puzzle                        */
/*        -d      Print the recursive trial depth required to solve the puzzle      */
/*        -e      Print a step-by-step explanation of the solution(s)               */
/*        -f      Takes an argument which specifes an input file                    */
/*                containing one or more unsolved puzzles (default: stdin)          */
/*        -G      Print the puzzle solution(s) in a 9x9 grid format                 */
/*        -g      Print the number of given clues                                   */
/*        -m      Print an octal mask for the puzzle givens                         */
/*        -n      Number each result                                                */
/*        -o      Specifies an output file for the solutions (default: stdout)      */
/*        -p      Takes an argument giving a single inline puzzle to be solved      */
/*        -r      Specifies an output file for unsolvable puzzles                   */
/*                (default: stderr)                                                 */
/*        -s      Print the puzzle's score or difficulty rating                     */
/*        -?      Print usage information                                           */
/*                                                                                  */
/* The return code is zero if all puzzles had unique solutions,                     */
/* (or have one or more solutions when -1 is specified) and non-zero                */
/* when multiple or no solutions exist.                                             */
/*                                                                                  */
/* LICENSE:                                                                         */
/*                                                                                  */
/* This program is free software; you can redistribute it and/or modify             */
/* it under the terms of the GNU General Public License as published by             */
/* the Free Software Foundation; either version 2 of the License, or                */
/* (at your option) any later version.                                              */
/*                                                                                  */
/* This program is distributed in the hope that it will be useful,                  */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    */
/* GNU General Public License for more details.                                     */
/*                                                                                  */
/* You should have received a copy of the GNU General Public License                */
/* along with this program; if not, write to the Free Software                      */
/* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA       */
/*                                                                                  */
/* CONTACT:                                                                         */
/*                                                                                  */
/* Email: bdupree@techfinesse.com                                                   */
/* Post: Bill DuPree, 609 Wenonah Ave, Oak Park, IL 60304 USA                       */
/*                                                                                  */
/************************************************************************************/
/*                                                                                  */
/* CHANGE LOG:                                                                      */
/*                                                                                  */
/* Rev.	  Date        Init.	Description                                         */
/* -------------------------------------------------------------------------------- */
/* 1.00   2006-02-25  WD	Initial version.                                    */
/* 1.01   2006-03-13  WD	Fixed return code calc. Added signon message.       */
/* 1.10   2006-03-20  WD        Added explain option, add'l speed optimizations     */
/* 1.11   2006-03-23  WD        More simple speed optimizations, cleanup, bug fixes */
/* 1.20   2008-08-17  WD        Fix early recursion. Rewrite markup, subset and     */
/*                              box-line interaction. Add bottleneck detection and  */
/*                              other scoring enhancements. Allow linkage to        */
/*                              sudoku_engine as a reusable object module.          */
/*                              (Thanks to Giuseppe Matarazzo for his suggestions.) */
/*                                                                                  */
/************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "sudoku_engine.h"

#define VERSION "1.20"

/* Command line options */
#ifdef EXPLAIN
#define OPTIONS "?1acde:Ggmnp:s"
#else
#define OPTIONS "?1acd:Ggmnp:s"
#endif

extern char *optarg;
extern int optind, opterr, optopt;

/************************************/
/* Print hints as to command usage. */
/************************************/

static void usage(char *myname)
{
	fprintf(stderr, "Usage:\n\t%s {-p puzzle | -f <puzzle_file>} [-o <outfile>]\n", myname);
        fprintf(stderr, "\t\t[-r <reject_file>] [-1][-a][-c][-G][-g][-l][-m][-n][-s]\n");
        fprintf(stderr, "where:\n\t-1\tSearch for first solution, otherwise all solutions are returned\n"
                        "\t-a\tRequests that the answer (solution) be printed\n"
                        "\t-c\tPrint a count of solutions for each puzzle\n"
                        "\t-d\tPrint the recursive trial depth required to solve the puzzle\n"
#ifdef EXPLAIN
			"\t-e\tPrint a step-by-step explanation of the solution(s)\n"
#endif
                        "\t-G\tPrint the puzzle solution(s) in a 9x9 grid format\n"
                        "\t-g\tPrint the number of given clues\n"
                        "\t-m\tPrint an octal mask for the puzzle givens\n"
                        "\t-n\tNumber each result\n"
                        "\t-p\tTakes an argument giving a single inline puzzle to be solved\n"
                        "\t-s\tPrint the puzzle's score or difficulty rating\n"
			"\t-?\tPrint usage information\n\n");
        fprintf(stderr, "The return code is zero if all puzzles had unique solutions,\n"
                        "(or have one or more solutions when -1 is specified) and non-zero\n"
                        "when no unique solution exists.\n");
}

/*******************/
/* Mainline logic. */
/*******************/

int main(int argc, char **argv)
{
	int i, rc, bogus, opt, count, solved, unsolved, solncount, explain, first_soln_only;
        int prt_count, prt_num, prt_score, prt_answer, prt_depth, prt_grid, prt_mask, prt_givens, prt;
        char *myname, outbuf[128], mbuf[28];
        static char inbuf[1024];
	Grid *s, *g, *solved_list;
        FILE *solnfile, *rejects;

        /* Get our command name from invoking command line */
        myname = argv[0];

        /* Print sign-on message to console */
        fprintf(stderr, "%s version %s\n", myname, VERSION);

        /* Init */
        solnfile = stdout;
        rejects = stderr;
        count = solved = unsolved = 0;
        explain = rc = bogus = prt_mask = prt_grid = prt_score = prt_depth = prt_answer = prt_count = prt_num = prt_givens = 0;
        first_soln_only = 0;
        *inbuf = 0;

        /* Parse command line options */
	while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        	switch (opt) {
                        case '1':
                        	first_soln_only = 1;		/* only find first soln */
                                break;
                        case 'a':
                        	prt_answer = 1;		/* print solution */
                                break;
                        case 'c':
                        	prt_count = 1;		/* number solutions */
                                break;
                        case 'd':
                        	prt_depth = 1;
                                break;
#ifdef EXPLAIN
                        case 'e':
                        	explain = 1;
                                break;
#endif
                        case 'G':
                        	prt_grid = 1;
                                break;
                        case 'g':
                        	prt_givens = 1;
                                break;
                        case 'm':
                        	prt_mask = 1;
                                break;
                        case 'n':
                        	prt_num = 1;
                                break;
                	case 'p':
                                strncpy(inbuf, optarg, sizeof(inbuf)-1);
                                break;
                        case 's':
                        	prt_score = 1;
                                break;
                	default:
                	case '?':
                        	usage(myname);
				exit(1);
                }
        }

        /* Set prt flag if we're printing anything at all */
	prt = prt_mask | prt_grid | prt_score | prt_depth | prt_answer | prt_num | prt_givens;

        /* Anything else on the command line is bogus */
        if (argc > optind) {
        	fprintf(stderr, "Extraneous args: ");
                for (i = optind; i < argc; i++) {
                	fprintf(stderr, "%s ", argv[i]);
                }
                fprintf(stderr, "\n\n");
                usage(myname);
                exit(1);
        }

        if (first_soln_only && prt_score) {
        	fprintf(stderr, "Scoring is meaningless when multi-solution mode is disabled.\n");
        }

        init_solve_engine(NULL, solnfile, rejects, first_soln_only, explain);

        while (*inbuf) {

		count += 1;

                if ((solved_list = solve_sudoku(inbuf)) == NULL) {
                	fprintf(rejects, "%d: %s invalid puzzle format\n", count, inbuf);
	                *inbuf = 0;
                        bogus += 1;
                        continue;
                }

        	if (solved_list->solncount) {
	               	solved++;
	                for (solncount = 0, g = s = solved_list; s; s = s->next) {
                        	solncount += 1;
	        		if (prt_num) {
        	                	char nbuf[32];
                	                if (first_soln_only)
						sprintf(nbuf, "%d: ", count);
                                        else
						sprintf(nbuf, "%d:%d ", count, solncount);
					fprintf(solnfile, "%-s", nbuf);
	                        }
                                if (solncount > 1 || first_soln_only) g->score = 0;
        	                if (prt_score) fprintf(solnfile, "score: %-7d ", g->score);
                	        if (prt_depth) fprintf(solnfile, "depth: %-3d ", g->maxlvl);
                        	if (prt_answer || prt_grid) format_answer(s, outbuf);
	                        if (prt_answer) fprintf(solnfile, "%s", outbuf);
                                if (prt_mask) fprintf(solnfile, " %s", cvt_to_mask(mbuf, inbuf));
                                if (prt_givens) fprintf(solnfile, " %d", g->givens);
        	                if (prt_grid) print_grid(outbuf, solnfile);
                	        if (prt) fprintf(solnfile, "\n");
                                if (s->next == NULL && prt_count) fprintf(solnfile, "count: %d\n", solncount);
                        }
                        if (solncount > 1) {
                               	rc |= 1;
                        }
                }
                else {
                	unsolved++;
                        rc |= 1;
                	fprintf(rejects, "%d: %*.*s insoluble\n", count, PUZZLE_CELLS, PUZZLE_CELLS, inbuf);
			diagnostic_grid(solved_list, rejects);
                        #if defined(DEBUG)
			mypause();
                        #endif
                }

                free_soln_list(solved_list);

                *inbuf = 0;
	}

        if (prt)
		fprintf(solnfile, "\nPuzzles: %d, Solved: %d, Insoluble: %d, Invalid: %d\n", count, solved, unsolved, bogus);

	return rc;
}

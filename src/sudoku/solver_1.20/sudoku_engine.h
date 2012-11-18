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
/* backtracking approaches upon exhausting all of its deductive moves. See the C    */
/* source code for more detailed information.                                       */
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

#ifndef _SUDSOLVER_H_

#define _SUDSOLVER_H_

/* Baseline puzzle parameters */
#define PUZZLE_ORDER 3
#define PUZZLE_DIM (PUZZLE_ORDER*PUZZLE_ORDER)
#define PUZZLE_CELLS (PUZZLE_DIM*PUZZLE_DIM)

/* Flags for cellflags member */
#define UNSOLVED 0
#define GIVEN    1
#define SOLVED   2

/* Return codes for funcs that modify puzzle markup */
#define NOCHANGE 0
#define CHANGE   1
#define IMPASSE  3

typedef struct grd {
	short cellflags[PUZZLE_CELLS];
        short solved[PUZZLE_CELLS];
	short cell[PUZZLE_CELLS];
        short tail, givens, exposed, maxlvl, inc, reward;
        unsigned int score, solncount, pass_mods;
        struct grd *next;
} Grid;

/********************************************************/
/* Type definition for a user defined callback function */
/********************************************************/

typedef int (*RETURN_SOLN)(const Grid *g);



/*****************************************************/
/* Function prototype(s) for the solver engine API's */
/*****************************************************/

/****************************************************************************/
/* Function to print a sudoku puzzle Grid as an 81 character ASCIIZ string. */
/* The first parameter is a pointer to a Grid structure (which is the       */
/* internal representation used by the solver engine.) The second           */
/* parameter is a pointer to an 82 character output buffer which is to      */
/* receive the puzzle string. In the output string, solved puzzle cells     */
/* will be converted to their assigned number, and unsolved cells will be   */
/* represented as the period , i.e. '.', character. A pointer to the        */
/* output buffer is returned. Results are undefined if the output buffer    */
/* is less than 82 characters in length.                                    */
/****************************************************************************/

char *format_answer(const Grid *g, char *outbuf);

/*******************************************************************************************/
/* Print the (presumably solved) 81 character puzzle string, 'sud', as a standard 9x9 grid */
/* to the given file. No value is returned. Results are undefined if sud is not an 81      */
/* character string                                                                        */
/*******************************************************************************************/

void print_grid(const char *sud, FILE *h);

/**********************************************************************/
/* Print the partially solved puzzle, 'g', and all associated markup  */
/* in 9x9 fashion to the file, 'h'. Note, markup is not printed if    */
/* the puzzle is already solved. No value is returned.                */
/**********************************************************************/

void diagnostic_grid(const Grid *g, FILE *h);

/*************************************************************************/
/* Setup parameters for sudoku solver engine.                            */
/*                                                                       */
/* The first parameter is a pointer to a user supplied callback function */
/* that will be called every time a solution is found. The supplied      */
/* pointer may be NULL if no callback is desired. The callback function  */
/* is presented with a solved Grid structure when it is called. It is    */
/* expected to return an integer to the solver engine where a zero       */
/* indicates that the engine should continue enumerating solutions, and  */
/* a non-zero value indicates that the solver should cancel further      */
/* enumeration. (See RETURN_SOLN typedef defined above.)                 */
/*                                                                       */
/* The second parameter is a FILE pointer (which may be NULL) where      */
/* solution explanations are written if desired (defaults to stdout if   */
/* NULL is supplied.)                                                    */
/*                                                                       */
/* Similarly, the third parameter is a FILE pointer where diagnostics    */
/* are written when a puzzle is insoluble (defaults to stderr if NULL is */
/* supplied.)                                                            */
/*                                                                       */
/* The fourth parameter is a flag that, when non-zero, requests that the */
/* solver engine stop enumeration after finding the first solution.      */
/*                                                                       */
/* The fifth parameter is also a flag that, when non-zero, requests that */
/* the steps to a solution (i.e. an explanation) are written to the      */
/* output file (specified by the second parameter.)                      */
/*                                                                       */
/* Finally, the return value supplied by the function is a version       */
/* string for the solver engine.                                         */
/*                                                                       */
/* This function may be interleaved with calls to solve_sudoku() to      */
/* change settings as needed.                                            */
/*************************************************************************/

const char *init_solve_engine(RETURN_SOLN solution_callback, FILE *solns, 
                              FILE *reject, int first_soln_only, int explanation);

/*****************************************************************/
/* Sudoku puzzle solver engine entry point.                      */
/*                                                               */
/* Solve the supplied 81 character puzzle, if solvable. Return a */
/* list of grids which enumerate all possible solutions. If no   */
/* solution exists, the list will contain a single partially     */
/* completed grid, and the solncount member will be set to zero. */
/* Note that only the first 81 characters of the supplied puzzle */
/* argument string are examined; any excess is ignored. The      */
/* calling application should use the free_soln_list() function  */
/* to properly dispose of the returned list after it has         */
/* finished processing the results.                              */
/*****************************************************************/

Grid *solve_sudoku(const char *puzzle);

/*****************************************************************/
/* This function is used to free the allocated list of solutions */
/* returned by the solve_sudoku() function.                      */
/*****************************************************************/

void free_soln_list(Grid *soln_list);

/**************************************************************/
/* Based upon the unsolved Left-to-Right-Top-to-Bottom puzzle */
/* presented in "sbuf", create a 27 octal digit mask of the   */
/* givens in the 28 character buffer pointed to by "mbuf."    */
/* Return a pointer to mbuf after conversion or NULL if sbuf  */
/* contains less than 81 characters. Results are undefined if */
/* mbuf is less than 28 characters in length.                 */
/**************************************************************/

char *cvt_to_mask(char *mbuf, const char *sbuf);

#endif

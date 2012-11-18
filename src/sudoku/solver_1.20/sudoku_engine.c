/************************************************************************************/
/*                                                                                  */
/* Author: Bill DuPree                                                              */
/* Name: sudoku_engine.c                                                            */
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
/* The puzzle solving algorithm is "home grown." I did not borrow any of the usual  */
/* techniques from the literature, e.g. Donald Knuth's "Dancing Links." Instead     */
/* I rolled my own from scratch. As such, its performance can only be blamed        */
/* on yours truly. Still, I feel it is quite fast. On a 333 MHz Pentium II Linux    */
/* box it solves typical medium force puzzles in approximately 1100 microseconds or */
/* about 900 puzzles per second, give or take. On an Athlon 64 4000+ (2.4 GHz       */
/* San Diego core) it solves about 8000 puzzles per sec.                            */
/*                                                                                  */
/* DESCRIPTION OF ALGORITHM:                                                        */
/*                                                                                  */
/* The puzzle algorithm initially assumes every unsolved cell can assume every      */
/* possible value. It then uses the placement of the givens to refine the choices   */
/* available to each cell. This is the markup phase.                                */
/*                                                                                  */
/* After markup completes, the algorithm then looks for "singleton" cells with      */
/* values that, due to constraints imposed by the row, column, or 3x3 box, may      */
/* only assume one possible value. Once these cells are assigned values, the        */
/* algorithm returns to the markup phase to apply these changes to the remaining    */
/* candidate solutions. The markup/singleton phases alternate until either no more  */
/* changes occur, or the puzzle is solved. I call the markup/singleton elimination  */
/* loop the "Simple Solver" because in a majority of cases it solves the puzzle.    */
/*                                                                                  */
/* If the simple solver portion of the algorithm doesn't produce a solution, then   */
/* more advanced deductive rules are applied. I've implemented two additional rules */
/* as part of the deductive puzzle solver. The first is "naked/hidden" subset       */
/* elimination wherein a row/column/box is scanned for N number of cells with N     */
/* matching candidate solutions. If such subsets are found in the row, column, or   */
/* box, then the candidates values from the subset may be eliminated from all other */
/* unsolved cells within the row, column, or box, respectively.                     */
/*                                                                                  */
/* The second advanced deductive rule scans boxes looking for candidate values that */
/* exclusively align themselves along rows or columns within the boxes, aka         */
/* chutes. If candidate values are found aligning within a set of N chutes aligned  */
/* within N boxes, then those candidates may be eliminated from aligned chutes      */
/* in boxes outside of the set of N boxes.                                          */
/*                                                                                  */
/* Note that each of the advanced deductive rules calls all preceeding rules, in    */
/* order, if that advanced rule has effected a change in puzzle markup.             */
/*                                                                                  */
/* Finally, if no solution is found after iteratively applying all deductive rules, */
/* then we begin trial-and-error using recursion for backtracking. A working copy   */
/* is created from our puzzle, and using this copy the first cell with the          */
/* smallest number of candidate solutions is chosen. One of the solutions values is */
/* assigned to that cell, and the solver algorithm is called using this working     */
/* copy as its starting point. Eventually, either a solution, or an impasse is      */
/* reached.                                                                         */
/*                                                                                  */
/* If we reach an impasse, the recursion unwinds and the next trial solution is     */
/* attempted. If a solution is found (at any point) the values for the solution are */
/* added to a list. Again, so long as we are examining all possibilities, the       */
/* recursion unwinds so that the next trial may be attempted. It is in this manner  */
/* that puzzles with multiple solutions are enumerated.                             */
/*                                                                                  */
/* Note that it is certainly possible to add to the list of applied deductive       */
/* rules. The techniques known as "X-Wing" and "Swordfish" come to mind. On the     */
/* other hand, adding these additional rules will, in all likelihood, slow the      */
/* solver down by adding to the computational burden while producing very few       */
/* results.                                                                         */
/*                                                                                  */
/* PUZZLE SCORING                                                                   */
/*                                                                                  */
/* A word about puzzle scoring, i.e. rating a puzzle's difficulty, is in order.     */
/* Rating Sudoku puzzles is a rather subjective thing, and thus it is difficult to  */
/* develop an objective puzzle rating system. I, however, have attempted            */
/* this feat (several times with varying degrees of success ;-) and I think the     */
/* heuristics I've developed closely track the relative difficulty of solving a     */
/* puzzle.                                                                          */
/*                                                                                  */
/* The following is a brief rundown of how it works. The initial puzzle markup is   */
/* a "free" operation, i.e. no points are scored for the first markup pass. I feel  */
/* this is appropriate because a person solving a puzzle will always have to do     */
/* their own eyeballing and scanning of the puzzle. Subsequent passes are           */
/* scored at one point per candidate eliminated because these passes indicate       */
/* that more deductive work is required. Secondly, the "reward" for solving a cell  */
/* is set to one point, and as long as the solution does not require bifurcation,   */
/* i.e. trial-and-error or recursion, then this level of reward remains unchanged.  */
/* Also, puzzle "bottlenecks," wherein there are very few singletons (three or      */
/* less) available after a round of deduction, are scored using the the formula:    */
/*                                                                                  */
/*  (20 * unsolved_cells_before_pass) / (solved_cells * (solved_this_pass + 1)) + 1 */
/*                                                                                  */
/* Other bottleneck penalties are assessed for the various deductive techniques     */
/* applied during the solving process. They are computed as follows:                */
/*                                                                                  */
/* Tuple elimination bottlenecks occur if such an elimination occurred but there    */
/* were fewer than four changes to the puzzle markup. These are scored as:          */
/*                                                                                  */
/*      20 - 5 * number_of_markup_changes                                           */
/*                                                                                  */
/* Box-line alignment bottlenecks occur if this rule was successfully applied but   */
/* there were fewer than three changes to markup. These are scored as:              */
/*                                                                                  */
/*      15 - 5 * number_of_markup_changes                                           */
/*                                                                                  */
/* In addition, box-line rules assess a penalty of five points per application,     */
/* and naked tuple eliminations use the following calculation for their penalty:    */
/*                                                                                  */
/*      10 + 2 * (5 - abs(5 - number_of_members_in_tuple))                          */
/*                                                                                  */
/* The reason for such a calculation is that naked tuples form disjoint subsets     */
/* with hidden tuples. Thus when the number of members in a naked subset is low,    */
/* the difficulty is low. As the number increases so does the relative difficulty   */
/* of finding the naked subset. However, for larger values of membership (five or   */
/* better) it turns out that what we are really detecting are smaller and smaller   */
/* hidden subsets, thus the penalty decreases.                                      */
/*                                                                                  */
/* If a trial-and-error approach is called for, then the "reward" is set to the     */
/* recursive depth times ten. Trial solutions are also penalized by a weighting     */
/* factor that is computed as follows:                                              */
/*                                                                                  */
/*  unsolved_cells * trial_depth * trial_depth * alternatives_to_be_tried * 5       */
/*                                                                                  */
/* (I've seen a pathological puzzles require 16 or more levels of recursion score   */
/* hundreds thousands of points using this scoring system!)                         */
/*                                                                                  */
/* And that brings me to this topic: What do all these points mean?                 */
/*                                                                                  */
/* Well, who knows? This is subjective, and the weighting system I've chosen        */
/* for point scoring is is largely arbitrary and is derived from my own set of      */
/* heuristics. But based upon feedback from a number of individuals, a rough scale  */
/* of subjective difficulty plays out as follows:                                   */
/*                                                                                  */
/*   DEGREE OF DIFFICULTY   |  SCORE                                                */
/* -------------------------+------------------------------------------             */
/*   TRIVIAL                |  80 points or less                                    */
/*   EASY                   |  81 - 125 points                                      */
/*   MEDIUM                 |  126 - 250 points                                     */
/*   HARD                   |  251 - 350 points                                     */
/*   VERY HARD              |  351 - 760 points                                     */
/*   DIABOLICAL             |  761 and up                                           */
/*                                                                                  */
/* Experience shows that the hardest of the VERY HARD puzzles may require a small   */
/* amount trial-and-error (bifurcation), and the DIABOLICAL puzzles will likely     */
/* require more than one level of trial-and-error,  so why waste your time?         */
/* These are best left to masochists, savants and automated solvers. YMMV.          */
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

static inline int is_given(int c) { return (c >= '1') && (c <= '9'); }

static int add_soln(Grid *g);

/*********************************************/
/*** BEGIN configurable sudoku_engine vars ***/

static FILE *rejects = NULL;
static int enumerate_all = 1;
static RETURN_SOLN soln_callback = NULL;

#ifdef EXPLAIN
static FILE *solnfile = NULL;
static int explain = 0;
#endif

/***  END configurable sudoku_engine vars  ***/
/*********************************************/

static int lvl = 0;
static int abort_mission = 0;

static Grid *soln_list = NULL;

/* This is the list of cell coordinates specified on a row basis */

static int const row[PUZZLE_DIM][PUZZLE_DIM] = {
 {  0,  1,  2,  3,  4,  5,  6,  7,  8 },
 {  9, 10, 11, 12, 13, 14, 15, 16, 17 },
 { 18, 19, 20, 21, 22, 23, 24, 25, 26 },
 { 27, 28, 29, 30, 31, 32, 33, 34, 35 },
 { 36, 37, 38, 39, 40, 41, 42, 43, 44 },
 { 45, 46, 47, 48, 49, 50, 51, 52, 53 },
 { 54, 55, 56, 57, 58, 59, 60, 61, 62 },
 { 63, 64, 65, 66, 67, 68, 69, 70, 71 },
 { 72, 73, 74, 75, 76, 77, 78, 79, 80 }};

/* This is the list of cell coordinates specified on a column basis */

static int const col[PUZZLE_DIM][PUZZLE_DIM] = {
 {  0,  9, 18, 27, 36, 45, 54, 63, 72 },
 {  1, 10, 19, 28, 37, 46, 55, 64, 73 },
 {  2, 11, 20, 29, 38, 47, 56, 65, 74 },
 {  3, 12, 21, 30, 39, 48, 57, 66, 75 },
 {  4, 13, 22, 31, 40, 49, 58, 67, 76 },
 {  5, 14, 23, 32, 41, 50, 59, 68, 77 },
 {  6, 15, 24, 33, 42, 51, 60, 69, 78 },
 {  7, 16, 25, 34, 43, 52, 61, 70, 79 },
 {  8, 17, 26, 35, 44, 53, 62, 71, 80 }};

/* This is the list of cell coordinates specified on a 3x3 box basis */

static int const box[PUZZLE_DIM][PUZZLE_DIM] = {
 {  0,  1,  2,  9, 10, 11, 18, 19, 20 },
 {  3,  4,  5, 12, 13, 14, 21, 22, 23 },
 {  6,  7,  8, 15, 16, 17, 24, 25, 26 },
 { 27, 28, 29, 36, 37, 38, 45, 46, 47 },
 { 30, 31, 32, 39, 40, 41, 48, 49, 50 },
 { 33, 34, 35, 42, 43, 44, 51, 52, 53 },
 { 54, 55, 56, 63, 64, 65, 72, 73, 74 },
 { 57, 58, 59, 66, 67, 68, 75, 76, 77 },
 { 60, 61, 62, 69, 70, 71, 78, 79, 80 }};

typedef struct {
	int row, col, box;
} cellmap;

/* Array structure to help map cell index back to row, column, and box */
static cellmap const map[PUZZLE_CELLS] = {
   { 0, 0, 0 },
   { 0, 1, 0 },
   { 0, 2, 0 },
   { 0, 3, 1 },
   { 0, 4, 1 },
   { 0, 5, 1 },
   { 0, 6, 2 },
   { 0, 7, 2 },
   { 0, 8, 2 },
   { 1, 0, 0 },
   { 1, 1, 0 },
   { 1, 2, 0 },
   { 1, 3, 1 },
   { 1, 4, 1 },
   { 1, 5, 1 },
   { 1, 6, 2 },
   { 1, 7, 2 },
   { 1, 8, 2 },
   { 2, 0, 0 },
   { 2, 1, 0 },
   { 2, 2, 0 },
   { 2, 3, 1 },
   { 2, 4, 1 },
   { 2, 5, 1 },
   { 2, 6, 2 },
   { 2, 7, 2 },
   { 2, 8, 2 },
   { 3, 0, 3 },
   { 3, 1, 3 },
   { 3, 2, 3 },
   { 3, 3, 4 },
   { 3, 4, 4 },
   { 3, 5, 4 },
   { 3, 6, 5 },
   { 3, 7, 5 },
   { 3, 8, 5 },
   { 4, 0, 3 },
   { 4, 1, 3 },
   { 4, 2, 3 },
   { 4, 3, 4 },
   { 4, 4, 4 },
   { 4, 5, 4 },
   { 4, 6, 5 },
   { 4, 7, 5 },
   { 4, 8, 5 },
   { 5, 0, 3 },
   { 5, 1, 3 },
   { 5, 2, 3 },
   { 5, 3, 4 },
   { 5, 4, 4 },
   { 5, 5, 4 },
   { 5, 6, 5 },
   { 5, 7, 5 },
   { 5, 8, 5 },
   { 6, 0, 6 },
   { 6, 1, 6 },
   { 6, 2, 6 },
   { 6, 3, 7 },
   { 6, 4, 7 },
   { 6, 5, 7 },
   { 6, 6, 8 },
   { 6, 7, 8 },
   { 6, 8, 8 },
   { 7, 0, 6 },
   { 7, 1, 6 },
   { 7, 2, 6 },
   { 7, 3, 7 },
   { 7, 4, 7 },
   { 7, 5, 7 },
   { 7, 6, 8 },
   { 7, 7, 8 },
   { 7, 8, 8 },
   { 8, 0, 6 },
   { 8, 1, 6 },
   { 8, 2, 6 },
   { 8, 3, 7 },
   { 8, 4, 7 },
   { 8, 5, 7 },
   { 8, 6, 8 },
   { 8, 7, 8 },
   { 8, 8, 8 }
};

static const short symtab[1<<PUZZLE_DIM] = {
	'.','1','2','.','3','.','.','.','4','.','.','.','.','.','.','.','5','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'6','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'7','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'8','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'9','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.',
	'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.'};

static const int tuples1[9] = {
	0x001, 0x002, 0x004, 0x008, 0x010, 0x020, 0x040, 0x080, 0x100 };

static const int tuples2[36] = {
	0x003, 0x005, 0x006, 0x009, 0x00a, 0x00c, 0x011, 0x012, 0x014, 0x018, 0x021, 0x022, 0x024, 0x028, 0x030, 0x041, 
	0x042, 0x044, 0x048, 0x050, 0x060, 0x081, 0x082, 0x084, 0x088, 0x090, 0x0a0, 0x0c0, 0x101, 0x102, 0x104, 0x108, 
	0x110, 0x120, 0x140, 0x180 };

static const int tuples3[84] = {
	0x007, 0x00b, 0x00d, 0x00e, 0x013, 0x015, 0x016, 0x019, 0x01a, 0x01c, 0x023, 0x025, 0x026, 0x029, 0x02a, 0x02c, 
	0x031, 0x032, 0x034, 0x038, 0x043, 0x045, 0x046, 0x049, 0x04a, 0x04c, 0x051, 0x052, 0x054, 0x058, 0x061, 0x062, 
	0x064, 0x068, 0x070, 0x083, 0x085, 0x086, 0x089, 0x08a, 0x08c, 0x091, 0x092, 0x094, 0x098, 0x0a1, 0x0a2, 0x0a4, 
	0x0a8, 0x0b0, 0x0c1, 0x0c2, 0x0c4, 0x0c8, 0x0d0, 0x0e0, 0x103, 0x105, 0x106, 0x109, 0x10a, 0x10c, 0x111, 0x112, 
	0x114, 0x118, 0x121, 0x122, 0x124, 0x128, 0x130, 0x141, 0x142, 0x144, 0x148, 0x150, 0x160, 0x181, 0x182, 0x184, 
	0x188, 0x190, 0x1a0, 0x1c0 };

static const int tuples4[126] = {
	0x00f, 0x017, 0x01b, 0x01d, 0x01e, 0x027, 0x02b, 0x02d, 0x02e, 0x033, 0x035, 0x036, 0x039, 0x03a, 0x03c, 0x047, 
	0x04b, 0x04d, 0x04e, 0x053, 0x055, 0x056, 0x059, 0x05a, 0x05c, 0x063, 0x065, 0x066, 0x069, 0x06a, 0x06c, 0x071, 
	0x072, 0x074, 0x078, 0x087, 0x08b, 0x08d, 0x08e, 0x093, 0x095, 0x096, 0x099, 0x09a, 0x09c, 0x0a3, 0x0a5, 0x0a6, 
	0x0a9, 0x0aa, 0x0ac, 0x0b1, 0x0b2, 0x0b4, 0x0b8, 0x0c3, 0x0c5, 0x0c6, 0x0c9, 0x0ca, 0x0cc, 0x0d1, 0x0d2, 0x0d4, 
	0x0d8, 0x0e1, 0x0e2, 0x0e4, 0x0e8, 0x0f0, 0x107, 0x10b, 0x10d, 0x10e, 0x113, 0x115, 0x116, 0x119, 0x11a, 0x11c, 
	0x123, 0x125, 0x126, 0x129, 0x12a, 0x12c, 0x131, 0x132, 0x134, 0x138, 0x143, 0x145, 0x146, 0x149, 0x14a, 0x14c, 
	0x151, 0x152, 0x154, 0x158, 0x161, 0x162, 0x164, 0x168, 0x170, 0x183, 0x185, 0x186, 0x189, 0x18a, 0x18c, 0x191, 
	0x192, 0x194, 0x198, 0x1a1, 0x1a2, 0x1a4, 0x1a8, 0x1b0, 0x1c1, 0x1c2, 0x1c4, 0x1c8, 0x1d0, 0x1e0 };

static const int tuples5[126] = {
	0x01f, 0x02f, 0x037, 0x03b, 0x03d, 0x03e, 0x04f, 0x057, 0x05b, 0x05d, 0x05e, 0x067, 0x06b, 0x06d, 0x06e, 0x073, 
	0x075, 0x076, 0x079, 0x07a, 0x07c, 0x08f, 0x097, 0x09b, 0x09d, 0x09e, 0x0a7, 0x0ab, 0x0ad, 0x0ae, 0x0b3, 0x0b5, 
	0x0b6, 0x0b9, 0x0ba, 0x0bc, 0x0c7, 0x0cb, 0x0cd, 0x0ce, 0x0d3, 0x0d5, 0x0d6, 0x0d9, 0x0da, 0x0dc, 0x0e3, 0x0e5, 
	0x0e6, 0x0e9, 0x0ea, 0x0ec, 0x0f1, 0x0f2, 0x0f4, 0x0f8, 0x10f, 0x117, 0x11b, 0x11d, 0x11e, 0x127, 0x12b, 0x12d, 
	0x12e, 0x133, 0x135, 0x136, 0x139, 0x13a, 0x13c, 0x147, 0x14b, 0x14d, 0x14e, 0x153, 0x155, 0x156, 0x159, 0x15a, 
	0x15c, 0x163, 0x165, 0x166, 0x169, 0x16a, 0x16c, 0x171, 0x172, 0x174, 0x178, 0x187, 0x18b, 0x18d, 0x18e, 0x193, 
	0x195, 0x196, 0x199, 0x19a, 0x19c, 0x1a3, 0x1a5, 0x1a6, 0x1a9, 0x1aa, 0x1ac, 0x1b1, 0x1b2, 0x1b4, 0x1b8, 0x1c3, 
	0x1c5, 0x1c6, 0x1c9, 0x1ca, 0x1cc, 0x1d1, 0x1d2, 0x1d4, 0x1d8, 0x1e1, 0x1e2, 0x1e4, 0x1e8, 0x1f0 };

static const int tuples6[84] = {
	0x03f, 0x05f, 0x06f, 0x077, 0x07b, 0x07d, 0x07e, 0x09f, 0x0af, 0x0b7, 0x0bb, 0x0bd, 0x0be, 0x0cf, 0x0d7, 0x0db, 
	0x0dd, 0x0de, 0x0e7, 0x0eb, 0x0ed, 0x0ee, 0x0f3, 0x0f5, 0x0f6, 0x0f9, 0x0fa, 0x0fc, 0x11f, 0x12f, 0x137, 0x13b, 
	0x13d, 0x13e, 0x14f, 0x157, 0x15b, 0x15d, 0x15e, 0x167, 0x16b, 0x16d, 0x16e, 0x173, 0x175, 0x176, 0x179, 0x17a, 
	0x17c, 0x18f, 0x197, 0x19b, 0x19d, 0x19e, 0x1a7, 0x1ab, 0x1ad, 0x1ae, 0x1b3, 0x1b5, 0x1b6, 0x1b9, 0x1ba, 0x1bc, 
	0x1c7, 0x1cb, 0x1cd, 0x1ce, 0x1d3, 0x1d5, 0x1d6, 0x1d9, 0x1da, 0x1dc, 0x1e3, 0x1e5, 0x1e6, 0x1e9, 0x1ea, 0x1ec, 
	0x1f1, 0x1f2, 0x1f4, 0x1f8 };

static const int tuples7[36] = {
	0x07f, 0x0bf, 0x0df, 0x0ef, 0x0f7, 0x0fb, 0x0fd, 0x0fe, 0x13f, 0x15f, 0x16f, 0x177, 0x17b, 0x17d, 0x17e, 0x19f, 
	0x1af, 0x1b7, 0x1bb, 0x1bd, 0x1be, 0x1cf, 0x1d7, 0x1db, 0x1dd, 0x1de, 0x1e7, 0x1eb, 0x1ed, 0x1ee, 0x1f3, 0x1f5, 
	0x1f6, 0x1f9, 0x1fa, 0x1fc };

static int tuples8[9] = {
	0x0ff, 0x17f, 0x1bf, 0x1df, 0x1ef, 0x1f7, 0x1fb, 0x1fd, 0x1fe };

typedef struct tuple_list_info {
	const int *tuple_list;
        const int tuple_count;
} TUPLE_LIST_INFO;

static const TUPLE_LIST_INFO tuples_list[9] = {
	{ NULL, 0 },
	{ tuples1, 9 },
        { tuples2, 36 },
        { tuples3, 84 },
        { tuples4, 126 },
        { tuples5, 126 },
        { tuples6, 84 },
        { tuples7, 36 },
        { tuples8, 9 }
};

#define PEER_LEN ((PUZZLE_DIM - 1) * 2 + PUZZLE_DIM - (2 * PUZZLE_ORDER - 1))

/* Enumerate each cell's peers */
static const int peers[PUZZLE_CELLS][PEER_LEN] = {
 { 1, 2, 3, 4, 5, 6, 7, 8, 9, 18, 27, 36, 45, 54, 63, 72, 10, 11, 19, 20 }, 
 { 0, 2, 3, 4, 5, 6, 7, 8, 10, 19, 28, 37, 46, 55, 64, 73, 9, 11, 18, 20 }, 
 { 0, 1, 3, 4, 5, 6, 7, 8, 11, 20, 29, 38, 47, 56, 65, 74, 9, 10, 18, 19 },
 { 0, 1, 2, 4, 5, 6, 7, 8, 12, 21, 30, 39, 48, 57, 66, 75, 13, 14, 22, 23 }, 
 { 0, 1, 2, 3, 5, 6, 7, 8, 13, 22, 31, 40, 49, 58, 67, 76, 12, 14, 21, 23 }, 
 { 0, 1, 2, 3, 4, 6, 7, 8, 14, 23, 32, 41, 50, 59, 68, 77, 12, 13, 21, 22 }, 
 { 0, 1, 2, 3, 4, 5, 7, 8, 15, 24, 33, 42, 51, 60, 69, 78, 16, 17, 25, 26 }, 
 { 0, 1, 2, 3, 4, 5, 6, 8, 16, 25, 34, 43, 52, 61, 70, 79, 15, 17, 24, 26 }, 
 { 0, 1, 2, 3, 4, 5, 6, 7, 17, 26, 35, 44, 53, 62, 71, 80, 15, 16, 24, 25 }, 
 { 10, 11, 12, 13, 14, 15, 16, 17, 0, 18, 27, 36, 45, 54, 63, 72, 1, 2, 19, 20 }, 
 { 9, 11, 12, 13, 14, 15, 16, 17, 1, 19, 28, 37, 46, 55, 64, 73, 0, 2, 18, 20 },  
 { 9, 10, 12, 13, 14, 15, 16, 17, 2, 20, 29, 38, 47, 56, 65, 74, 0, 1, 18, 19 },  
 { 9, 10, 11, 13, 14, 15, 16, 17, 3, 21, 30, 39, 48, 57, 66, 75, 4, 5, 22, 23 },  
 { 9, 10, 11, 12, 14, 15, 16, 17, 4, 22, 31, 40, 49, 58, 67, 76, 3, 5, 21, 23 },  
 { 9, 10, 11, 12, 13, 15, 16, 17, 5, 23, 32, 41, 50, 59, 68, 77, 3, 4, 21, 22 },  
 { 9, 10, 11, 12, 13, 14, 16, 17, 6, 24, 33, 42, 51, 60, 69, 78, 7, 8, 25, 26 },  
 { 9, 10, 11, 12, 13, 14, 15, 17, 7, 25, 34, 43, 52, 61, 70, 79, 6, 8, 24, 26 },  
 { 9, 10, 11, 12, 13, 14, 15, 16, 8, 26, 35, 44, 53, 62, 71, 80, 6, 7, 24, 25 },  
 { 19, 20, 21, 22, 23, 24, 25, 26, 0, 9, 27, 36, 45, 54, 63, 72, 1, 2, 10, 11 },  
 { 18, 20, 21, 22, 23, 24, 25, 26, 1, 10, 28, 37, 46, 55, 64, 73, 0, 2, 9, 11 },  
 { 18, 19, 21, 22, 23, 24, 25, 26, 2, 11, 29, 38, 47, 56, 65, 74, 0, 1, 9, 10 },  
 { 18, 19, 20, 22, 23, 24, 25, 26, 3, 12, 30, 39, 48, 57, 66, 75, 4, 5, 13, 14 },  
 { 18, 19, 20, 21, 23, 24, 25, 26, 4, 13, 31, 40, 49, 58, 67, 76, 3, 5, 12, 14 },  
 { 18, 19, 20, 21, 22, 24, 25, 26, 5, 14, 32, 41, 50, 59, 68, 77, 3, 4, 12, 13 },  
 { 18, 19, 20, 21, 22, 23, 25, 26, 6, 15, 33, 42, 51, 60, 69, 78, 7, 8, 16, 17 },  
 { 18, 19, 20, 21, 22, 23, 24, 26, 7, 16, 34, 43, 52, 61, 70, 79, 6, 8, 15, 17 },  
 { 18, 19, 20, 21, 22, 23, 24, 25, 8, 17, 35, 44, 53, 62, 71, 80, 6, 7, 15, 16 },  
 { 28, 29, 30, 31, 32, 33, 34, 35, 0, 9, 18, 36, 45, 54, 63, 72, 37, 38, 46, 47 },  
 { 27, 29, 30, 31, 32, 33, 34, 35, 1, 10, 19, 37, 46, 55, 64, 73, 36, 38, 45, 47 },  
 { 27, 28, 30, 31, 32, 33, 34, 35, 2, 11, 20, 38, 47, 56, 65, 74, 36, 37, 45, 46 },  
 { 27, 28, 29, 31, 32, 33, 34, 35, 3, 12, 21, 39, 48, 57, 66, 75, 40, 41, 49, 50 },  
 { 27, 28, 29, 30, 32, 33, 34, 35, 4, 13, 22, 40, 49, 58, 67, 76, 39, 41, 48, 50 },  
 { 27, 28, 29, 30, 31, 33, 34, 35, 5, 14, 23, 41, 50, 59, 68, 77, 39, 40, 48, 49 },  
 { 27, 28, 29, 30, 31, 32, 34, 35, 6, 15, 24, 42, 51, 60, 69, 78, 43, 44, 52, 53 },  
 { 27, 28, 29, 30, 31, 32, 33, 35, 7, 16, 25, 43, 52, 61, 70, 79, 42, 44, 51, 53 },  
 { 27, 28, 29, 30, 31, 32, 33, 34, 8, 17, 26, 44, 53, 62, 71, 80, 42, 43, 51, 52 },  
 { 37, 38, 39, 40, 41, 42, 43, 44, 0, 9, 18, 27, 45, 54, 63, 72, 28, 29, 46, 47  }, 
 { 36, 38, 39, 40, 41, 42, 43, 44, 1, 10, 19, 28, 46, 55, 64, 73, 27, 29, 45, 47 },  
 { 36, 37, 39, 40, 41, 42, 43, 44, 2, 11, 20, 29, 47, 56, 65, 74, 27, 28, 45, 46 },  
 { 36, 37, 38, 40, 41, 42, 43, 44, 3, 12, 21, 30, 48, 57, 66, 75, 31, 32, 49, 50 },  
 { 36, 37, 38, 39, 41, 42, 43, 44, 4, 13, 22, 31, 49, 58, 67, 76, 30, 32, 48, 50 },  
 { 36, 37, 38, 39, 40, 42, 43, 44, 5, 14, 23, 32, 50, 59, 68, 77, 30, 31, 48, 49 },  
 { 36, 37, 38, 39, 40, 41, 43, 44, 6, 15, 24, 33, 51, 60, 69, 78, 34, 35, 52, 53 },  
 { 36, 37, 38, 39, 40, 41, 42, 44, 7, 16, 25, 34, 52, 61, 70, 79, 33, 35, 51, 53 },  
 { 36, 37, 38, 39, 40, 41, 42, 43, 8, 17, 26, 35, 53, 62, 71, 80, 33, 34, 51, 52 },  
 { 46, 47, 48, 49, 50, 51, 52, 53, 0, 9, 18, 27, 36, 54, 63, 72, 28, 29, 37, 38  }, 
 { 45, 47, 48, 49, 50, 51, 52, 53, 1, 10, 19, 28, 37, 55, 64, 73, 27, 29, 36, 38 },  
 { 45, 46, 48, 49, 50, 51, 52, 53, 2, 11, 20, 29, 38, 56, 65, 74, 27, 28, 36, 37 },  
 { 45, 46, 47, 49, 50, 51, 52, 53, 3, 12, 21, 30, 39, 57, 66, 75, 31, 32, 40, 41 },  
 { 45, 46, 47, 48, 50, 51, 52, 53, 4, 13, 22, 31, 40, 58, 67, 76, 30, 32, 39, 41 },  
 { 45, 46, 47, 48, 49, 51, 52, 53, 5, 14, 23, 32, 41, 59, 68, 77, 30, 31, 39, 40 },  
 { 45, 46, 47, 48, 49, 50, 52, 53, 6, 15, 24, 33, 42, 60, 69, 78, 34, 35, 43, 44 },  
 { 45, 46, 47, 48, 49, 50, 51, 53, 7, 16, 25, 34, 43, 61, 70, 79, 33, 35, 42, 44 },  
 { 45, 46, 47, 48, 49, 50, 51, 52, 8, 17, 26, 35, 44, 62, 71, 80, 33, 34, 42, 43 },  
 { 55, 56, 57, 58, 59, 60, 61, 62, 0, 9, 18, 27, 36, 45, 63, 72, 64, 65, 73, 74  }, 
 { 54, 56, 57, 58, 59, 60, 61, 62, 1, 10, 19, 28, 37, 46, 64, 73, 63, 65, 72, 74 },  
 { 54, 55, 57, 58, 59, 60, 61, 62, 2, 11, 20, 29, 38, 47, 65, 74, 63, 64, 72, 73 },  
 { 54, 55, 56, 58, 59, 60, 61, 62, 3, 12, 21, 30, 39, 48, 66, 75, 67, 68, 76, 77 },  
 { 54, 55, 56, 57, 59, 60, 61, 62, 4, 13, 22, 31, 40, 49, 67, 76, 66, 68, 75, 77 },  
 { 54, 55, 56, 57, 58, 60, 61, 62, 5, 14, 23, 32, 41, 50, 68, 77, 66, 67, 75, 76 },  
 { 54, 55, 56, 57, 58, 59, 61, 62, 6, 15, 24, 33, 42, 51, 69, 78, 70, 71, 79, 80 },  
 { 54, 55, 56, 57, 58, 59, 60, 62, 7, 16, 25, 34, 43, 52, 70, 79, 69, 71, 78, 80 },  
 { 54, 55, 56, 57, 58, 59, 60, 61, 8, 17, 26, 35, 44, 53, 71, 80, 69, 70, 78, 79 },  
 { 64, 65, 66, 67, 68, 69, 70, 71, 0, 9, 18, 27, 36, 45, 54, 72, 55, 56, 73, 74  }, 
 { 63, 65, 66, 67, 68, 69, 70, 71, 1, 10, 19, 28, 37, 46, 55, 73, 54, 56, 72, 74 },  
 { 63, 64, 66, 67, 68, 69, 70, 71, 2, 11, 20, 29, 38, 47, 56, 74, 54, 55, 72, 73 },  
 { 63, 64, 65, 67, 68, 69, 70, 71, 3, 12, 21, 30, 39, 48, 57, 75, 58, 59, 76, 77 },  
 { 63, 64, 65, 66, 68, 69, 70, 71, 4, 13, 22, 31, 40, 49, 58, 76, 57, 59, 75, 77 },  
 { 63, 64, 65, 66, 67, 69, 70, 71, 5, 14, 23, 32, 41, 50, 59, 77, 57, 58, 75, 76 },  
 { 63, 64, 65, 66, 67, 68, 70, 71, 6, 15, 24, 33, 42, 51, 60, 78, 61, 62, 79, 80 },  
 { 63, 64, 65, 66, 67, 68, 69, 71, 7, 16, 25, 34, 43, 52, 61, 79, 60, 62, 78, 80 },  
 { 63, 64, 65, 66, 67, 68, 69, 70, 8, 17, 26, 35, 44, 53, 62, 80, 60, 61, 78, 79 },  
 { 73, 74, 75, 76, 77, 78, 79, 80, 0, 9, 18, 27, 36, 45, 54, 63, 55, 56, 64, 65  }, 
 { 72, 74, 75, 76, 77, 78, 79, 80, 1, 10, 19, 28, 37, 46, 55, 64, 54, 56, 63, 65 },  
 { 72, 73, 75, 76, 77, 78, 79, 80, 2, 11, 20, 29, 38, 47, 56, 65, 54, 55, 63, 64 },  
 { 72, 73, 74, 76, 77, 78, 79, 80, 3, 12, 21, 30, 39, 48, 57, 66, 58, 59, 67, 68 },  
 { 72, 73, 74, 75, 77, 78, 79, 80, 4, 13, 22, 31, 40, 49, 58, 67, 57, 59, 66, 68 },  
 { 72, 73, 74, 75, 76, 78, 79, 80, 5, 14, 23, 32, 41, 50, 59, 68, 57, 58, 66, 67 },  
 { 72, 73, 74, 75, 76, 77, 79, 80, 6, 15, 24, 33, 42, 51, 60, 69, 61, 62, 70, 71 },  
 { 72, 73, 74, 75, 76, 77, 78, 80, 7, 16, 25, 34, 43, 52, 61, 70, 60, 62, 69, 71 },  
 { 72, 73, 74, 75, 76, 77, 78, 79, 8, 17, 26, 35, 44, 53, 62, 71, 60, 61, 69, 70 }};

/* Function prototype(s) */

#if defined(DEBUG)
static void mypause()
{
	char buf[8];
        printf("\tPress enter -> ");
        fgets(buf, 8, stdin);
}
#endif

/*****************************************************/
/* Return the number of '1' bits in a cell.          */
/* Rather than count bits, do a quick table lookup.  */
/* Warning: Only valid for 9 low order bits.         */
/*****************************************************/
 
static inline short bitcount(short cell)
{
        static const short bcounts[512] = {
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
        4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,5,6,6,7,6,7,7,8,6,7,7,8,7,8,8,9};

        return bcounts[cell];
}

/******************************************************************/
/* Construct a string representing the possible values a cell may */
/* contain according to current markup.                           */
/******************************************************************/
static char *clues(short cell, char *buf)
{
	int i, m, multi, mask;
        char *p;

        multi = m = bitcount(cell & 0x01ff);

        if (!multi) {
		strcpy(buf, "NULL");
                return buf;
        }

	if (multi > 1) {
        	strcpy(buf, "tuple (");
        }
        else {
        	buf[0] = '\0';
        }

        p = buf + strlen(buf);

        for (mask = i = 1; i <= PUZZLE_DIM; i++) {
        	if (mask & cell) {
                	*p++ = symtab[mask];
                        multi -= 1;
                        if (multi) { *p++ = ','; *p++ = ' '; }
                }
                mask <<= 1;
        }
        if (m > 1) *p++ = ')';
	*p = 0;
        return buf;
}

#ifdef EXPLAIN

/**************************************************/
/* Indent two spaces for each level of recursion. */
/**************************************************/
static inline void explain_indent(FILE *h)
{
	int i;

        for (i = 0; i < lvl-1; i++) fprintf(h, "  ");
}

/*************************************************************/
/* Explain removal of a candidate value from a changed cell. */
/*************************************************************/
static void explain_markup_elim(Grid *g, int chgd, int clue)
{
	int chgd_row, chgd_col, clue_row, clue_col;
        char buf[32];

        chgd_row = map[chgd].row+1;
        chgd_col = map[chgd].col+1;
        clue_row = map[clue].row+1;
        clue_col = map[clue].col+1;

        explain_indent(solnfile);
        fprintf(solnfile, "Candidate %s removed from row %d, col %d because of cell at row %d, col %d\n",
                clues(g->cell[clue], buf), chgd_row, chgd_col, clue_row, clue_col);
}

/*****************************************/
/* Dump the state of the current markup. */
/*****************************************/
static void explain_current_markup(Grid *g)
{
	if (g->exposed >= PUZZLE_CELLS) return;

        fprintf(solnfile, "\n");
        explain_indent(solnfile);
	fprintf(solnfile, "Current markup is as follows:");
        diagnostic_grid(g, solnfile);
        fprintf(solnfile, "\n");
}

/****************************************/
/* Explain the solving of a given cell. */
/****************************************/
static void explain_solve_cell(Grid *g, int chgd)
{
	int chgd_row, chgd_col;
        char buf[32];

        chgd_row = map[chgd].row+1;
        chgd_col = map[chgd].col+1;

        explain_indent(solnfile);
        fprintf(solnfile, "Cell at row %d, col %d solved with value %s\n",
                chgd_row, chgd_col, clues(g->cell[chgd], buf));
}

/******************************************************************/
/* Explain the current impasse reached during markup elimination. */
/******************************************************************/
static void explain_markup_impasse(Grid *g, int chgd, int clue)
{
	int chgd_row, chgd_col, clue_row, clue_col;

        chgd_row = map[chgd].row+1;
        chgd_col = map[chgd].col+1;
        clue_row = map[clue].row+1;
        clue_col = map[clue].col+1;

        explain_indent(solnfile);
        fprintf(solnfile, "Impasse for cell at row %d, col %d because cell at row %d, col %d removes %s\n",
                chgd_row, chgd_col, clue_row, clue_col, g->cellflags[chgd] == GIVEN ? "a given clue" : "the last candidate");
        explain_current_markup(g);
}

/****************************************/
/* Explain naked and/or hidden singles. */
/****************************************/
static void explain_singleton(Grid *g, int chgd, int mask, char *vdesc)
{
	int chgd_row, chgd_col, chgd_box;
        char buf[32];

        chgd_row = map[chgd].row+1;
        chgd_col = map[chgd].col+1;
        chgd_box = map[chgd].box+1;

        explain_indent(solnfile);
        fprintf(solnfile, "Cell of box %d at row %d, col %d will only solve for %s in this %s\n",
                chgd_box, chgd_row, chgd_col, clues(mask, buf), vdesc);
}

/*********************************/
/* Explain initial puzzle state. */
/*********************************/
static void explain_markup()
{
        fprintf(solnfile, "\n");
        explain_indent(solnfile);
	fprintf(solnfile, "Assume all cells may contain any values in the range: [1 - 9]\n");
}

/************************/
/* Explain given clues. */
/************************/
static void explain_given(int cell, char val)
{
	int cell_row, cell_col;

        cell_row = map[cell].row+1;
        cell_col = map[cell].col+1;

        explain_indent(solnfile);
        fprintf(solnfile, "Cell at row %d, col %d is given clue value %c\n", cell_row, cell_col, val);
}

/*******************************************/
/* Explain box/row/column interactions.    */
/*******************************************/
static void explain_vector_elim(char *desc, int chute, int cell, int val, int box_tuple)
{
	int cell_row, cell_col;
        char buf1[32], buf2[32];

        cell_row = map[cell].row+1;
        cell_col = map[cell].col+1;

        explain_indent(solnfile);
        fprintf(solnfile, "Candidate %d removed from cell at row %d, col %d because it aligns along %s %s in box %s\n",
                val+1, cell_row, cell_col, desc, clues(chute, buf1), clues(box_tuple, buf2));
}

/******************************************************************/
/* Explain the current impasse reached during vector elimination. */
/******************************************************************/
static void explain_vector_impasse(Grid *g, char *desc, int chute, int cell, int val, int box_tuple)
{
	int cell_row, cell_col;
        char buf1[32], buf2[32];

        cell_row = map[cell].row+1;
        cell_col = map[cell].col+1;

        explain_indent(solnfile);
        fprintf(solnfile, "Impasse at cell at row %d, col %d because candidate %d aligns along %s %s in box %s\n",
                cell_row, cell_col, val, desc, clues(chute, buf1), clues(box_tuple, buf2));
        explain_current_markup(g);
}

/*****************************************************************/
/* Explain the current impasse reached during tuple elimination. */
/*****************************************************************/
static void explain_tuple_impasse(Grid *g, char *desc, int elt, int tuple, int count, int bits)
{
	char buf[32];

        explain_indent(solnfile);
        fprintf(solnfile, "Impasse in %s %d because too many (%d) cells have %d-valued %s\n",
                desc, elt+1, count, bits, clues(tuple, buf));
        explain_current_markup(g);
}

/*********************************************************************/
/* Explain the removal of a tuple of candidate solutions from a cell */
/*********************************************************************/
static void explain_tuple_elim(char *desc, int elt, int tuple, int cell)
{
	char buf[32];

        explain_indent(solnfile);
        fprintf(solnfile, "Value of %s in %s %d removed from cell at row %d, col %d\n",
                clues(tuple, buf), desc, elt+1, map[cell].row+1, map[cell].col+1);

}

/**************************************************/
/* Indicate that a viable solution has been found */
/**************************************************/
static void explain_soln_found(Grid *g)
{
	char buf[90];

        fprintf(solnfile, "\n");
        explain_indent(solnfile);
        fprintf(solnfile, "Solution found: %s\n", format_answer(g, buf));
        print_grid(buf, solnfile);
        fprintf(solnfile, "\n");
}

/***************************/
/* Show the initial puzzle */
/***************************/
static void explain_grid(Grid *g)
{
	char buf[90];

        fprintf(solnfile, "Initial puzzle: %s\n", format_answer(g, buf));
        print_grid(buf, solnfile);
        explain_current_markup(g);
        fprintf(solnfile, "\n");
}

/*************************************************/
/* Explain attempt at a trial and error solution */
/*************************************************/
static void explain_trial(int cell, int value)
{
	char buf[32];

        explain_indent(solnfile);
        fprintf(solnfile, "Attempt trial where cell at row %d, col %d is assigned value %s\n",
                map[cell].row+1, map[cell].col+1, clues(value, buf));
}

/**********************************************/
/* Explain back out of current trial solution */
/**********************************************/
static void explain_backtrack()
{
	if (lvl <= 1) return;

        explain_indent(solnfile);
        fprintf(solnfile, "Backtracking\n\n");
}

#define EXPLAIN_MARKUP                                 if (explain) explain_markup()
#define EXPLAIN_CURRENT_MARKUP(g)                      if (explain) explain_current_markup((g))
#define EXPLAIN_GIVEN(cell, val)	               if (explain) explain_given((cell), (val))
#define EXPLAIN_MARKUP_ELIM(g, chgd, clue)             if (explain) explain_markup_elim((g), (chgd), (clue))
#define EXPLAIN_MARKUP_SOLVE(g, cell)                  if (explain) explain_solve_cell((g), (cell)) 
#define EXPLAIN_MARKUP_IMPASSE(g, chgd, clue)          if (explain) explain_markup_impasse((g), (chgd), (clue))
#define EXPLAIN_SINGLETON(g, chgd, mask, vdesc)        if (explain) explain_singleton((g), (chgd), (mask), (vdesc))
#define EXPLAIN_VECTOR_ELIM(desc, i, cell, v, r)       if (explain) explain_vector_elim((desc), (i), (cell), (v), (r))
#define EXPLAIN_VECTOR_IMPASSE(g, desc, i, cell, v, r) if (explain) explain_vector_impasse((g), (desc), (i), (cell), (v), (r))
#define EXPLAIN_VECTOR_SOLVE(g, cell)                  if (explain) explain_solve_cell((g), (cell)) 
#define EXPLAIN_TUPLE_IMPASSE(g, desc, j, c, count, i) if (explain) explain_tuple_impasse((g), (desc), (j), (c), (count), (i))
#define EXPLAIN_TUPLE_ELIM(desc, j, c, cell)           if (explain) explain_tuple_elim((desc), (j), (c), (cell))
#define EXPLAIN_TUPLE_SOLVE(g, cell)                   if (explain) explain_solve_cell((g), (cell)) 
#define EXPLAIN_SOLN_FOUND(g)			       if (explain) explain_soln_found((g));
#define EXPLAIN_GRID(g)			               if (explain) explain_grid((g));
#define EXPLAIN_TRIAL(cell, val)		       if (explain) explain_trial((cell), (val));
#define EXPLAIN_BACKTRACK                              if (explain) explain_backtrack();
#define EXPLAIN_INDENT(h)			       if (explain) explain_indent((h))

#else

#define EXPLAIN_MARKUP
#define EXPLAIN_CURRENT_MARKUP(g)
#define EXPLAIN_GIVEN(cell, val)
#define EXPLAIN_MARKUP_ELIM(g, chgd, clue)
#define EXPLAIN_MARKUP_SOLVE(g, cell) 
#define EXPLAIN_MARKUP_IMPASSE(g, chgd, clue)
#define EXPLAIN_SINGLETON(g, chgd, mask, vdesc);
#define EXPLAIN_VECTOR_ELIM(desc, i, cell, v, r)
#define EXPLAIN_VECTOR_IMPASSE(g, desc, i, cell, v, r)
#define EXPLAIN_VECTOR_SOLVE(g, cell)
#define EXPLAIN_TUPLE_IMPASSE(g, desc, j, c, count, i)
#define EXPLAIN_TUPLE_ELIM(desc, j, c, cell)
#define EXPLAIN_TUPLE_SOLVE(g, cell)
#define EXPLAIN_SOLN_FOUND(g)
#define EXPLAIN_GRID(g)
#define EXPLAIN_TRIAL(cell, val)
#define EXPLAIN_BACKTRACK
#define EXPLAIN_INDENT(h)

#endif


/*****************************************************/
/* Initialize a grid to an empty state.              */
/* At the start, all cells can have any value        */
/* so set all 9 lower order bits in each cell.       */
/* In effect, the 9x9 grid now has markup that       */
/* specifies that each cell can assume any value     */
/* of 1 through 9.                                   */
/*****************************************************/

static void init_grid(Grid *g)
{
	int i;

        for (i = 0; i < PUZZLE_CELLS; i++) {
		g->cell[i] = 0x01ff;
                g->cellflags[i] = UNSOLVED;
        }
        g->exposed = 0;
        g->givens = 0;
        g->inc = 0;
        g->maxlvl = 1;
        g->score = 0;
        g->solncount = 0;
        g->reward = 1;
        g->next = NULL;
        g->tail = 0;
        EXPLAIN_MARKUP;
}

/*****************************************************/
/* Convert a puzzle from the input format,           */
/* i.e. a string of 81 non-blank characters          */
/* with ASCII digits '1' thru '9' specified          */
/* for the givens, and non-numeric characters        */
/* for the remaining cells. The string, read         */
/* left-to-right fills the 9x9 Sudoku grid           */
/* in left-to-right, top-to-bottom order.            */
/*****************************************************/

static int cvt_to_grid(Grid *g, const char *game)
{
	int i;

        init_grid(g);

        for (i = 0; i < PUZZLE_CELLS && game[i]; i++) {
        	if (is_given(game[i])) {
			/* warning -- ASCII charset assumed */
                	g->cell[i] = 1 << (game[i] - '1');
                        g->cellflags[i] = GIVEN;
                        g->givens += 1;
                        g->solved[g->exposed++] = i;
                        EXPLAIN_GIVEN(i, game[i]);
                }
        }

        return i;
}

/**********************************************************************/
/* Print the partially solved puzzle, 'g', and all associated markup  */
/* in 9x9 fashion to the file, 'h'. Note, markup is not printed if    */
/* the puzzle is already solved. No value is returned.                */
/**********************************************************************/

void diagnostic_grid(const Grid *g, FILE *h)
{
	int i, j, flag;
        short c;
        char line1[40], line2[40], line3[40], cbuf1[5], cbuf2[5], cbuf3[5], outbuf[PUZZLE_CELLS+1];

	/* Sanity check */
	for (flag = 1, i = 0; i < PUZZLE_CELLS; i++) {
        	if (bitcount(g->cell[i]) != 1) {
	                flag = 0;
                        break;
                }
        }

        /* Don't need to print grid with diagnostic markup? */
        if (flag) {
                format_answer(g, outbuf);
        	print_grid(outbuf, h);
                fflush(h);
                return;
        }

        strcpy(cbuf1, "   |");
	strcpy(cbuf2, cbuf1);
	strcpy(cbuf3, cbuf1);
	fprintf(h, "\n");

        for (i = 0; i < PUZZLE_DIM; i++) {

        	*line1 = *line2 = *line3 = 0;

        	for (j = 0; j < PUZZLE_DIM; j++) {

                	c = g->cell[row[i][j]];

                        if (bitcount(c) == 1) {
			        strcpy(cbuf1, "   |");
				strcpy(cbuf2, cbuf1);
				strcpy(cbuf3, cbuf1);
                                cbuf2[1] = symtab[c];  
                        }
                        else {
                        	if (c & 1) cbuf1[0] = '*'; else cbuf1[0] = '.';
                                if (c & 2) cbuf1[1] = '*'; else cbuf1[1] = '.';
                                if (c & 4) cbuf1[2] = '*'; else cbuf1[2] = '.';
                        	if (c & 8) cbuf2[0] = '*'; else cbuf2[0] = '.';
                                if (c & 16) cbuf2[1] = '*'; else cbuf2[1] = '.';
                                if (c & 32) cbuf2[2] = '*'; else cbuf2[2] = '.';
                        	if (c & 64) cbuf3[0] = '*'; else cbuf3[0] = '.';
                                if (c & 128) cbuf3[1] = '*'; else cbuf3[1] = '.';
                                if (c & 256) cbuf3[2] = '*'; else cbuf3[2] = '.';
                        }

	                strcat(line1, cbuf1);
			strcat(line2, cbuf2);
			strcat(line3, cbuf3);
                }

		EXPLAIN_INDENT(h);
                fprintf(h, "+---+---+---+---+---+---+---+---+---+\n");
		EXPLAIN_INDENT(h);
                fprintf(h, "|%s\n", line1);
		EXPLAIN_INDENT(h);
		fprintf(h, "|%s\n", line2);
		EXPLAIN_INDENT(h);
		fprintf(h, "|%s\n", line3);
        }
	EXPLAIN_INDENT(h);
        fprintf(h, "+---+---+---+---+---+---+---+---+---+\n"); fflush(h);
}

/***********************************************************************/
/* Validate that a sudoku grid contains a valid solution. Return 1 if  */
/* true, 0 if false. If the verbose argument is non-zero, then print   */
/* reasons for invalidating the solution to "rejects" file.            */
/***********************************************************************/

static int validate(const Grid *g, int verbose)
{
	int i, j, bc, boxmask, rowmask, colmask, flag = 1;
        char buf[32];

	/* Sanity check */
	for (i = 0; i < PUZZLE_CELLS; i++) {
        	if ((bc = bitcount(g->cell[i])) != 1) {
                	if (verbose) {
                                fprintf(rejects, "Cell %d at row %d, col %d %s.\n",
                                        1+i, 1+map[i].row, 1+map[i].col, (bc ? "has no unique solution" :"is at an impasse"));
				fflush(rejects);
	                	flag = 0;
                        } else return 0;
                }
        }

        /* Check rows */
        for (i = 0; i < PUZZLE_DIM; i++) {
        	for (rowmask = j = 0; j < PUZZLE_DIM; j++) {
                        if (bitcount(g->cell[row[i][j]]) == 1) rowmask |= g->cell[row[i][j]];
                }
                if (rowmask != 0x01ff) {
                	if (verbose) {
				fprintf(rejects, "Row %d is not solved for %s.\n", 1+i, clues(~rowmask, buf)); fflush(rejects);
	                	flag = 0;
                        } else return 0;
                }
        }

        /* Check columns */
        for (i = 0; i < PUZZLE_DIM; i++) {
        	for (colmask = j = 0; j < PUZZLE_DIM; j++) {
                        if (bitcount(g->cell[col[i][j]]) == 1) colmask |= g->cell[col[i][j]];
                }
                if (colmask != 0x01ff) {
                	if (verbose) {
				fprintf(rejects, "Column %d is not solved for %s.\n", 1+i, clues(~colmask, buf)); fflush(rejects);
	                	flag = 0;
                        } else return 0;
                }
        }

        /* Check 3x3 boxes */
        for (i = 0; i < PUZZLE_DIM; i++) {
        	for (boxmask = j = 0; j < PUZZLE_DIM; j++) {
                        if (bitcount(g->cell[box[i][j]]) == 1) boxmask |= g->cell[box[i][j]];
                }
                if (boxmask != 0x01ff) {
                	if (verbose) {
				fprintf(rejects, "Box %d is not solved for %s.\n", 1+i, clues(~boxmask, buf)); fflush(rejects);
	                	flag = 0;
                        } else return 0;
                }
        }

        return flag;
}

/********************************************************************************/
/* This function uses the cells with unique values, i.e. the given              */
/* or subsequently discovered solution values, to eliminate said values         */
/* as candidates in other as yet unsolved cells in the associated               */
/* rows, columns, and 3x3 boxes.                                                */
/*                                                                              */
/* The function has three possible return values:                               */
/*   NOCHANGE - Markup did not change during the last pass,                     */
/*   CHANGE   - Markup was modified, and                                        */
/*   IMPASSE  - Markup results are invalid, i.e. a cell has no candidate values */
/********************************************************************************/

static int mark_cells(Grid *g)
{
        int i, chgflag, bc, ndx;
        short elt, mask, before, cell;

       	chgflag = NOCHANGE;

	while (g->tail < g->exposed) {

		elt = g->solved[g->tail++];

                mask = ~g->cell[elt];

                for (i = 0; i < PEER_LEN; i++) {

                	/* Get the cell value to change */
                        ndx = peers[elt][i];
                        before = cell = g->cell[ndx];

                        /* Eliminate this candidate value whilst preserving other candidate values */
                        cell &= mask;
			g->cell[ndx] = cell;

                        /* Did the cell change value? */
                	if (before != cell) {

				chgflag |= CHANGE;	/* Flag that puzzle markup was changed */
                                g->score += g->inc;	/* More work means higher scoring      */

                                if (!(bc = bitcount(cell))) {
                                	EXPLAIN_MARKUP_IMPASSE(g, ndx, elt);
					return IMPASSE;	/* Crap out if no candidates remain */
                                }

                                EXPLAIN_MARKUP_ELIM(g, ndx, elt);

                                /* Check if we solved for this cell, i.e. bit count indicates a unique value */
                                if (bc == 1) {
					g->cellflags[ndx] = SOLVED;	/* Mark cell as found  */
                                        g->score += g->reward;		/* Add to puzzle score */
					g->pass_mods += 1;
                                        g->solved[g->exposed++] = ndx;
                                	EXPLAIN_MARKUP_SOLVE(g, ndx);
                                }
			}
                }
        }

        return chgflag;
}


/*******************************************************************/
/* Identify and "solve" all cells that, by reason of their markup, */
/* can only assume one specific value, i.e. the cell is the only   */
/* one in a row/column/box (specified by vector) that is           */
/* able to assume a particular value.                              */
/*                                                                 */
/* The function has two possible return values:                    */
/*   NOCHANGE - Markup did not change during the last pass,        */
/*   CHANGE   - Markup was modified.                               */
/*******************************************************************/

static int find_singletons(Grid *g, int const *vector, char *vdesc)
{
	int i, j, mask, hist[PUZZLE_DIM], value[PUZZLE_DIM], found = NOCHANGE;

	/* We are going to create a histogram of cell candidate values */
        /* for the specified cell vector (row/column/box).             */
        /* First set all buckets to zero.                              */
        memset(hist, 0, sizeof(hist[0])*PUZZLE_DIM);

	/* For each possible candidate value... */
	for (mask = 1, i = 0; i < PUZZLE_DIM; i++) {

	        /* For each cell in the vector... */
        	for (j = 0; j < PUZZLE_DIM; j++) {

                	/* If the cell may possibly assume this value... */
        		if (g->cell[vector[j]] & mask) {

                        	if (++hist[i] > 1) break;		/* Bump bucket in histogram */
                		value[i] = vector[j];			/* Save the cell coordinate */
                	}
                }

                mask <<= 1;
        }

        /* Examine each bucket in the histogram... */
        for (mask = 1, i = 0; i < PUZZLE_DIM; i++) {

        	/* If the bucket == 1 and the cell is not already solved,  */
		/* then the cell has a unique solution specified by "mask" */
        	if (hist[i] == 1 && g->cellflags[value[i]] == UNSOLVED) {

                	found = CHANGE;			 /* Indicate that markup has been changed */
                        g->cell[value[i]] = mask;	 /* Assign solution value to cell         */
                        g->cellflags[value[i]] = SOLVED; /* Mark cell as solved                   */
                        g->score += g->reward;           /* Bump puzzle score                     */
                        g->pass_mods += 1;
                        g->solved[g->exposed++] = value[i];
                        EXPLAIN_SINGLETON(g, value[i], mask, vdesc);
                }

                mask <<= 1;		/* Get next candidate value */
        }

	return found;
}


/*******************************************************************/
/* Find all cells with unique solutions (according to markup)      */
/* and mark them as found. Do this for each row, column, and       */
/* box.                                                            */
/*                                                                 */
/* The function has two possible return values:                    */
/*   NOCHANGE - Markup did not change during the last pass,        */
/*   CHANGE   - Markup was modified.                               */
/*******************************************************************/

static int eliminate_singles(Grid *g)
{
	int i, found = NOCHANGE;

        /* Do rows (horizontal chutes) */
        for (i = 0; i < PUZZLE_DIM; i++) {
        	found |= find_singletons(g, row[i], "row");
        }

        /* Do columns (vertical chutes) */
        for (i = 0; i < PUZZLE_DIM; i++) {
        	found |= find_singletons(g, col[i], "column");
        }

        /* Do boxes */
        for (i = 0; i < PUZZLE_DIM; i++) {
        	found |= find_singletons(g, box[i], "box");
        }

        return found;
}

/********************************************************************************/
/* Solves simple puzzles, i.e. single elimination                               */
/*                                                                              */
/* The function has three possible return values:                               */
/*   NOCHANGE - Markup did not change during the last pass,                     */
/*   CHANGE   - Markup was modified, and                                        */
/*   IMPASSE  - Markup results are invalid, i.e. a cell has no candidate values */
/********************************************************************************/
static int simple_solver(Grid *g)
{
	int i, b, flag, rc = NOCHANGE;

        /* Mark the unsolved cells with candidate solutions based upon the current set of "givens" and solved cells */
        for (i = 0;; i++) {

        	if (abort_mission) return IMPASSE;

	        g->pass_mods = 0;	/* Count number of solved cells per iteration */

        	if ((flag = mark_cells(g)) == IMPASSE) return flag;

                rc |= flag;

        	g->inc = 1;	     /* After initial markup, we start scoring for additional markup work */

        	if (flag == CHANGE) EXPLAIN_CURRENT_MARKUP(g);

		/* Continue to eliminate cells with unique candidate solutions from the game until */
        	/* elimination and repeated markup efforts produce no changes in the remaining     */
	        /* candidate solutions.                                                            */
                if (eliminate_singles(g) == NOCHANGE)
			break;

                /* score penalty for puzzle bottlenecks */
		if (g->pass_mods < 4) {
                        b = 1 + (20 * (PUZZLE_CELLS - g->exposed - g->pass_mods)) / (g->exposed * (g->pass_mods + 1));
                        g->score += b;
        	}

                EXPLAIN_CURRENT_MARKUP(g);
        }

        /* score penalty for puzzle bottlenecks */
	if (i == 0 && g->pass_mods < 4 && g->exposed < PUZZLE_CELLS) {
                b = 1 + (20 * (PUZZLE_CELLS - g->exposed - g->pass_mods)) / (g->exposed * (g->pass_mods + 1));
                g->score += b;
        }

        return rc;
}


/************************************************************************************/
/* Test rows and columns of box arrays to see if the candidates for a particular    */
/* number are confined to the same N rows or columns for the same set of N boxes,   */
/* and if so, eliminate their occurences in the remainder of the rows and/or        */
/* columns of the boxes not members of the aforementioned set of boxes.             */
/* (Hint: Think pointing pairs, etc.)                                               */
/*                                                                                  */
/* The function has three possible return values:                                   */
/*   NOCHANGE - Markup did not change during the last pass,                         */
/*   CHANGE   - Markup was modified, and                                            */
/*   IMPASSE  - Markup results are invalid, i.e. a cell has no candidate values     */
/************************************************************************************/

static int box_row_chute_elim(Grid *g, int num)
{
        int i, j, k, b, c, mask, box_tuple, box_row_mask, t, rc;
        short cell, boxmask[PUZZLE_DIM];

        /* Init */
        rc = NOCHANGE;

        mask = 1 << num;

	/* Compute the mask value for the box that has a 1 bit in       */
        /* positions corresponding to the row containing the candidate. */
        for (i = 0; i < PUZZLE_DIM; i++) {    /* for each box, do... */
        	boxmask[i] = 0;
        	for (j = 0; j < PUZZLE_DIM; j++) {	/* for each cell in the box do... */
        		c = box[i][j];
        		if ((g->cellflags[c] == UNSOLVED) && (g->cell[c] & mask)) {
                        	boxmask[i] |= 1 << map[c].row;
                	}
        	}
        }

	mask = ~mask;

	/* Figure out, for each row of boxes, if the candidate number lies in N rows for a subset of N boxes */
        /* where N is in the range [1..PUZZLE_ORDER-1].  If so, eliminate the candidate number from boxes    */
        /* in that row of boxes which are not contained within the subset.                                   */

        for (i = 1; i < PUZZLE_ORDER; i++) {					/* for each subset... */

        	for (b = 0; b < PUZZLE_DIM; b += PUZZLE_ORDER) {		/* for each of the rows of boxes... */

                	box_tuple = box_row_mask = 0;

                	for (k = 0; k < PUZZLE_ORDER; k++) {			/* for each box within the row... */

				if (bitcount(boxmask[b+k]) == i) {		/* does the box belong to the subset? */

                                	box_tuple = 1 << (b+k);			/* include box in subset */
                                        box_row_mask = boxmask[b+k];

                                	if (i - 1) for (t = 0; t < PUZZLE_ORDER; t++) {	/* find other boxes in the subset, if any */

                                        	if (t != k && box_row_mask == boxmask[b+t]) {	/* did we find one? */
                                                	box_tuple |= 1 << (b+t);
                                                }
                                        }
                        	}
                        }

                        /* Did we meet N row and N box constraint for this row of boxes? */
                        if (bitcount(box_tuple) == i) for (k = b; k < b+PUZZLE_ORDER; k++) {

                        	if ((box_tuple & (1 << k)) == 0) {	/* If box k is not in subset... */

                                	for (t = 0; t < PUZZLE_DIM; t++) {

                                        	c = box[k][t];
                                                cell = g->cell[c];

                                                /* Is box cell in the desired row? */
                                                /* And is it unsolved?             */
                                                if (((1 << map[c].row) & box_row_mask) &&       
                                                   g->cellflags[c] == UNSOLVED) {
                                                	if ((g->cell[c] &= mask) == 0) {
								EXPLAIN_VECTOR_IMPASSE(g, "row", box_row_mask, c, num, box_tuple);
								g->score += 10;
								return IMPASSE;
                                                        }
                                                        if (g->cell[c] ^ cell) {
                                                                boxmask[k] &= ~(1 << map[c].row);
                                                        	rc = CHANGE;
                                                                g->pass_mods += 1;
                                                                g->score += bitcount(g->cell[c] ^ cell);
			                                        EXPLAIN_VECTOR_ELIM("row", box_row_mask, c, num, box_tuple);
                                                                if (bitcount(g->cell[c]) == 1) {
                                                                	g->cellflags[c] = SOLVED;
                                                                        g->score += g->reward + 5;
                                                                        g->solved[g->exposed++] = c;
                                                                        EXPLAIN_VECTOR_SOLVE(g, c);
                                                                        return CHANGE;
                                                                }
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }

        if (rc == CHANGE) {
		g->score += 5;	/* Bump score for sucessfully invoking this rule */
        }

        return rc;
}

/*************/
/* As above. */
/*************/

static int box_col_chute_elim(Grid *g, int num)
{
        int i, j, k, b, c, mask, box_tuple, box_col_mask, t, rc;
        short cell, boxmask[PUZZLE_DIM];

        /* Init */
        rc = NOCHANGE;

        mask = 1 << num;

	/* Compute the mask value for the box that has a 1 bit in          */
        /* positions corresponding to the column containing the candidate. */
        for (i = 0; i < PUZZLE_DIM; i++) {    /* for each box, do... */
        	boxmask[i] = 0;
        	for (j = 0; j < PUZZLE_DIM; j++) {	/* for each cell in the box do... */
        		c = box[i][j];
        		if ((g->cellflags[c] == UNSOLVED) && (g->cell[c] & mask)) {
                        	boxmask[i] |= 1 << map[c].col;
                	}
        	}
        }

	mask = ~mask;

	/* Figure out, for each column of boxes, if the candidate number lies in N columns for a subset of N boxes */
        /* where N is in the range [1..PUZZLE_ORDER-1].  If so, eliminate the candidate number from boxes          */
        /* in those column of boxes which are not contained within the subset.                                     */

        for (i = 1; i < PUZZLE_ORDER; i++) {					/* for each subset... */

        	for (b = 0; b < PUZZLE_ORDER; b++) {				/* for each of the columns of boxes... */

                	box_tuple = box_col_mask = 0;

                	for (k = 0; k < PUZZLE_DIM; k += PUZZLE_ORDER) {	/* for each box within the column... */

				if (bitcount(boxmask[b+k]) == i) {		/* does the box belong to the subset? */

                                	box_tuple = 1 << (b+k);			/* include box in subset */
                                        box_col_mask = boxmask[b+k];

                                	if (i - 1) for (t = 0; t < PUZZLE_DIM; t += PUZZLE_ORDER) { /* find other boxes in the subset, if any */

                                        	if (t != k && box_col_mask == boxmask[b+t]) {	/* did we find one? */
                                                	box_tuple |= 1 << (b+t);
                                                }
                                        }
                        	}
                        }

                        /* Did we meet N column and N box constraint for this column of boxes? */
                        if (bitcount(box_tuple) == i) for (k = b; k < PUZZLE_DIM; k += PUZZLE_ORDER) {

                        	if ((box_tuple & (1 << k)) == 0) {	/* If box k is not in subset... */

                                	for (t = 0; t < PUZZLE_DIM; t++) {

                                        	c = box[k][t];
                                                cell = g->cell[c];

                                                /* Is box cell in the desired column? */
                                                /* And is it unsolved?                */
                                                if (((1 << map[c].col) & box_col_mask) &&       
                                                   g->cellflags[c] == UNSOLVED) {
                                                	if ((g->cell[c] &= mask) == 0) {
								EXPLAIN_VECTOR_IMPASSE(g, "column", box_col_mask, c, num, box_tuple);
								g->score += 10;
								return IMPASSE;
                                                        }
                                                        if (g->cell[c] ^ cell) {
                                                                boxmask[k] &= ~(1 << map[c].col);
                                                        	rc = CHANGE;
                                                                g->pass_mods += 1;
                                                                g->score += bitcount(g->cell[c] ^ cell);
			                                        EXPLAIN_VECTOR_ELIM("column", box_col_mask, c, num, box_tuple);
                                                                if (bitcount(g->cell[c]) == 1) {
                                                                	g->cellflags[c] = SOLVED;
                                                                        g->score += g->reward + 5;
                                                                        g->solved[g->exposed++] = c;
                                                                        EXPLAIN_VECTOR_SOLVE(g, c);
                                                                        return CHANGE;
                                                                }
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }

        if (rc == CHANGE) {
		g->score += 5;	/* Bump score for sucessfully invoking this rule */
        }

        return rc;
}

/**********************************************************************************/
/* Test all boxes to see if the possibilities for a number                        */
/* are confined to specific row or column chutes, and if so, eliminate            */
/* the occurence of candidate solutions from the remainder of the                 */
/* specified row or column.                                                       */
/*                                                                                */
/* The function has three possible return values:                                 */
/*   NOCHANGE - Markup did not change during the last pass,                       */
/*   CHANGE   - Markup was modified, and                                          */
/*   IMPASSE  - Markup results are invalid, i.e. a cell has no candidate values   */
/**********************************************************************************/

static int chute_elimination(Grid *g)
{
	int i, rc;

        rc = NOCHANGE;
        g->pass_mods = 0;

	/* For each digit... */
	for (i = 0; i < PUZZLE_DIM && rc == NOCHANGE; i++) {
		rc |= box_row_chute_elim(g, i);
        }        

	if (rc == NOCHANGE) for (i = 0; i < PUZZLE_DIM && rc == NOCHANGE; i++) {
		rc |= box_col_chute_elim(g, i);
        }

        /* score penalty for puzzle bottlenecks */
	if (g->pass_mods && g->pass_mods < 3) {
                g->score += 15 - (5 * g->pass_mods);;
        }

        return rc;
}


/**********************************************************************************/
/* This function implements the rule that when a subset of cells                  */
/* in a row/column/box contain matching tuples of candidate                       */
/* solutions, i.e. 2 matching candidates for 2 cells, 3 matching                  */
/* candidate possibilities for 3 cells, etc., then those                          */
/* candidate tuples may be eliminated from the other cells in the                 */
/* row, column, or box.                                                           */
/*                                                                                */
/* The function has three possible return values:                                 */
/*   NOCHANGE - Markup did not change during the last pass,                       */
/*   CHANGE   - Markup was modified, and                                          */
/*   IMPASSE  - Markup results are invalid, i.e. a cell has no candidate values   */
/**********************************************************************************/

static int elim_naked_tuples(Grid *g, int const *cell_list, char *desc, int ndx)
{
	int i, j, k, c, rc, flag, tuple_count, cellset, iter;
	const int *tuple_list;
        short m, mask, totalmask, tmp;

        rc = NOCHANGE;

	/* Compute bitmap of all possible candidates for this list of unsolved cells. */
        for (j = totalmask = 0; j < PUZZLE_DIM; j++) {
        	if (g->cellflags[cell_list[j]] == UNSOLVED)
                	totalmask |= g->cell[cell_list[j]];
        }

        iter = bitcount(totalmask);

        /* Check for two thru N valued naked tuples */
        for (i = 2; i < iter; i++) {

        	flag = NOCHANGE;
                tuple_list = tuples_list[i].tuple_list;
                tuple_count = tuples_list[i].tuple_count;

                for (j = 0; j < tuple_count && i < iter; j++) {

                	mask = tuple_list[j];

                	/* prune tuple search space */
                	if ((mask & totalmask) != mask) continue;

			/* Look for all unsolved cells containing this tuple */
                        for (m = 1, cellset = k = 0; k < PUZZLE_DIM; m <<= 1, k++) {
	                	c = cell_list[k];
        	                if (g->cellflags[c] == UNSOLVED && (g->cell[c] & mask) == g->cell[c])
                                	cellset |= m;
                        }

			/* Did we find a naked tuple? */
                        if ((m = bitcount(cellset)) == i) {

                                mask = ~mask;
                                totalmask &= mask;
                                iter = bitcount(totalmask);

                                for (m = 1, k = 0; k < PUZZLE_DIM; k++, m <<= 1) {

        		                if (m & cellset) continue;	/* skip cells within tuple set */

		                	c = cell_list[k];
					if (g->cellflags[c] != UNSOLVED) continue;	/* only consider unsolved cells */

                                        /* Get cell candidates */
                                        tmp = g->cell[c];

                                        /* Eliminate tuple values from cell candidates */
                                        g->cell[c] &= mask;

                                        /* Did the elimination change the candidates? */
                                        if (tmp ^ g->cell[c]) {

                                                /* Note the change and bump the score */
						flag = CHANGE;
                                                g->pass_mods += 1;
		                                g->score += bitcount(tmp ^ g->cell[c]);

                                                EXPLAIN_TUPLE_ELIM(desc, ndx, ~mask, c);

                                                /* Did we solve the cell under consideration? */
                        	        	if (bitcount(g->cell[c]) == 1) {

                                                	/* Mark cell as found and bump the score */
                                        		g->cellflags[c] = SOLVED;
                		                        g->score += g->reward;
                                                        g->solved[g->exposed++] = c;
                                                        EXPLAIN_TUPLE_SOLVE(g, c);
	        	                        }
        	                        }
                                }
                        }
			else if (m > i) {
                		EXPLAIN_TUPLE_IMPASSE(g, desc, ndx, cellset, m, i);
                        	g->score += 10;
				return IMPASSE;
                	}
                }
                
                if (flag == CHANGE)
			g->score += 10 + 2 *(5 - abs(5 - i));

                rc |= flag;
        }

	return rc;
}


/**********************************************************************************/
/* Eliminate subsets from rows, columns, and boxes.                               */
/*                                                                                */
/* The function has three possible return values:                                 */
/*   NOCHANGE - Markup did not change during the last pass,                       */
/*   CHANGE   - Markup was modified, and                                          */
/*   IMPASSE  - Markup results are invalid, i.e. a cell has no candidate values   */
/**********************************************************************************/
 
static int naked_tuple_elimination(Grid *g)
{
	int i, rc = NOCHANGE;

        g->pass_mods = 0;

        /* Eliminate subsets from rows */
        for (i = 0; i < PUZZLE_DIM; i++) {
        	rc |= elim_naked_tuples(g, row[i], "row", i);
        }

        /* Eliminate subsets from columns */
        for (i = 0; i < PUZZLE_DIM; i++) {
        	rc |= elim_naked_tuples(g, col[i], "column", i);
        }

        /* Eliminate subsets from boxes */
        for (i = 0; i < PUZZLE_DIM; i++) {
        	rc |= elim_naked_tuples(g, box[i], "box", i);
        }

        /* score penalty for puzzle bottlenecks */
	if (g->pass_mods && g->pass_mods < 4) {
                g->score += 20 - (5 * g->pass_mods);;
        }

        return rc;
}

/**************************************************/
/* Entry point to the recursive solver algorithm. */
/**************************************************/
static int rsolve(Grid *g)
{
	int i, j, min, c, mask, flag = NOCHANGE;
        Grid mygrid;

        /* Keep track of recursive depth */
        lvl += 1;
        if (lvl > g->maxlvl) g->maxlvl = lvl;

        /* Attempt a simple solution */
        while (simple_solver(g) != IMPASSE && g->exposed < PUZZLE_CELLS) {

                /* Eliminate clues aligned along chutes within boxes from */
		/* cells exterior to the box that are in those chutes     */
                if ((flag = chute_elimination(g)) == CHANGE) {
			EXPLAIN_CURRENT_MARKUP(g);
			continue;
		}

                /* Check if impasse or solution */
                if (flag == IMPASSE || g->exposed >= PUZZLE_CELLS) break;

		/* Eliminate tuples */
                if ((flag = naked_tuple_elimination(g)) == CHANGE) {
			EXPLAIN_CURRENT_MARKUP(g);
			continue;
		}

                /* Check if impasse or solution */
                if (flag == IMPASSE || g->exposed >= PUZZLE_CELLS) break;

                g->reward = lvl * 10;		/* Bump reward as we are about to start trial-and-error soutions */

                /* Attempt a trial solution */
        	memcpy(&mygrid, g, sizeof(Grid));	/* Make working copy of puzzle */

		/* Find the first cell with the smallest number of alternatives */
        	for (c = -1, j = 0, min = PUZZLE_DIM, i = 0; i < PUZZLE_CELLS; i++) {
        		if (mygrid.cellflags[i] == UNSOLVED) {
				j = bitcount(mygrid.cell[i]);
        	                if (j < min) {
					min = j;
                        	        c = i;
                                        if (j == 2) break;	/* bifurcate now */
	                        }
        	        }
	        }

                if (j) mygrid.score += (PUZZLE_CELLS - mygrid.exposed) * 5 * j * (1+lvl) * (1+lvl);	/* Add penalty to score */

                /* Cell at index 'c' will be our starting point */
        	if (c >= 0) for (mask = 1, i = 0; i < PUZZLE_DIM; i++) {

                	/* Is this a candidate? */
        		if (mask & g->cell[c]) {

                        	EXPLAIN_TRIAL(c, mask);

                                /* Try one of the possible candidates for this cell */
        	        	mygrid.cell[c] = mask;
                	        mygrid.cellflags[c] = SOLVED;
                                mygrid.solved[mygrid.exposed++] = c;

				EXPLAIN_CURRENT_MARKUP(&mygrid);
	                        flag = rsolve(&mygrid);		/* Recurse with working copy of puzzle */

                                /* Preserve score, solution count and recursive depth as we back out of recursion */
                                g->score = mygrid.score;
                                g->solncount = mygrid.solncount;
                                g->maxlvl = mygrid.maxlvl;
                	        memcpy(&mygrid, g, sizeof(Grid));

                                /* Did we find a solution? */
                        	if (flag == SOLVED) {
					if (!enumerate_all) {
                                		EXPLAIN_BACKTRACK;
                                        	lvl -= 1;
                                        	return SOLVED;
                                        }
        	                }
                                else if (abort_mission) {
                                       	lvl -= 1;
                                       	return IMPASSE;
                                }

	                }
        		mask <<= 1;	/* Get next possible candidate */
	        }

                break;
        }

        if (abort_mission) {
        	lvl -= 1;
		return IMPASSE;
        }

        if (g->exposed == PUZZLE_CELLS && validate(g, 0)) {
                add_soln(g);
	        EXPLAIN_SOLN_FOUND(g);
                EXPLAIN_BACKTRACK;
                lvl -= 1;
		flag = SOLVED;
        } else {
	        EXPLAIN_BACKTRACK;
		lvl -= 1;
		flag = IMPASSE;



                if (!lvl && !g->solncount) validate(g, 1);		/* Print verbose diagnostic for insoluble puzzle */
        }

	return flag;
}

/*****************************************************************/
/* This function adds a puzzle solution to a singly linked list  */
/* of solutions. It dies if no memory is available.              */
/*****************************************************************/
static inline void add_grid(const Grid *g)
{
	Grid *tmp;

	if ((tmp = malloc(sizeof(Grid))) == NULL) {
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	}
	memcpy(tmp, g, sizeof(Grid));
	tmp->next = soln_list;
	soln_list = tmp;
}

static int add_soln(Grid *g)
{
        g->solncount += 1;
	add_grid(g);
        abort_mission = soln_callback(g);
        return 0;
}


/****************************************/
/* Entry point to the solver algorithm. */
/****************************************/

static inline int solve_grid(Grid *g)
{
	int flag = NOCHANGE;

        if (simple_solver(g) != IMPASSE && g->exposed < PUZZLE_CELLS) {

	        flag = naked_tuple_elimination(g);		/* It is beneficial to eliminate subsets once before recursion, */
                						/* but this is *expensive*, so we keep it pushed to the back    */
                                                                /* of the rule set in rsolve()                                  */

        	if (flag != IMPASSE && g->exposed < PUZZLE_CELLS) {

			/* Non-trivial puzzle, call recursive solver */
                        return rsolve(g);
                }
        }

        if (g->exposed == PUZZLE_CELLS && validate(g, 0)) {
                add_soln(g);
	        EXPLAIN_SOLN_FOUND(g);
		flag = SOLVED;
        } else {
		flag = IMPASSE;
                validate(g, 1);		/* Print verbose diagnostic for insoluble puzzle */
        }

	return flag;
}

/*************************************************/
/* DTOR for list returned from the solver engine */
/*************************************************/

void free_soln_list(Grid *soln_list)
{
	Grid *s;

	/* Clean out old solutions, if any */
	for (s = soln_list; s;) {
		s = soln_list->next;
                free(soln_list);
                soln_list = s;
        }
}

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


static Grid *_solve_sudoku(const char *puzzle)
{
        Grid g;

        abort_mission = 0;

	if (cvt_to_grid(&g, puzzle) != PUZZLE_CELLS) {	/* bogus puzzle */
		return NULL;
        }

        if (g.givens < 17) {
	        return NULL;            /* Bogus puzzle */
	}

        EXPLAIN_GRID(&g);

	soln_list = NULL;

        /* Solve the puzzle, if possible */
        solve_grid(&g);

        if (g.solncount == 0) {
		add_grid(&g);	/* add unsolved grid - solncount == 0 indicates puzzle is unsolvable */
        }

        return soln_list;
}

/*******************************************/
/* Entry point if not properly initialized */
/*******************************************/

static Grid *_not_initialized(const char *puzzle)
{
	fprintf(stderr, "solve engine not properly initialized\n");
        exit(1);
	return NULL;
}

typedef Grid *(*SOLVE_ENGINE)(const char *puzzle);

static SOLVE_ENGINE solver_engine = _not_initialized;

/********************************************/
/* API entry point to the solver algorithm. */
/********************************************/

Grid *solve_sudoku(const char *puzzle)
{
	return solver_engine(puzzle);
}

static int default_callback(const Grid *g)
{
	return 0;
}

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
/* enumeration.                                                          */
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

const char *init_solve_engine(RETURN_SOLN solution_callback, FILE *solns, FILE *reject, int first_soln_only, int explanation)
{
	static char version[32];

        /* construct version string */
        sprintf(version, "Sudoku Engine version %s\n", VERSION);

#ifdef EXPLAIN
	explain = explanation;
        solnfile = solns ? solns : stdout;
#endif

	enumerate_all = first_soln_only == 0;

        rejects = reject ?  reject : stderr;

	soln_callback = solution_callback ? solution_callback : default_callback;

        solver_engine = _solve_sudoku;

        return version;
}


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

char *format_answer(const Grid *g, char *outbuf)
{
	int i;

	for (i = 0; i < PUZZLE_CELLS; i++)
		outbuf[i] = symtab[g->cell[i]];
	outbuf[i] = 0;

        return outbuf;
}

/*******************************************************************************************/
/* Print the (presumably solved) 81 character puzzle string, 'sud', as a standard 9x9 grid */
/* to the given file. No value is returned. Results are undefined if sud is not an 81      */
/* character string                                                                        */
/*******************************************************************************************/

void print_grid(const char *sud, FILE *h)
{

        fprintf(h, "\n");
        EXPLAIN_INDENT(h);
	fprintf(h, "+---+---+---+\n");

        EXPLAIN_INDENT(h);
        fprintf(h, "|%*.*s|%*.*s|%*.*s|\n", PUZZLE_ORDER, PUZZLE_ORDER, sud, PUZZLE_ORDER, PUZZLE_ORDER, sud+3, PUZZLE_ORDER, PUZZLE_ORDER, sud+6);
        EXPLAIN_INDENT(h);
        fprintf(h, "|%*.*s|%*.*s|%*.*s|\n", PUZZLE_ORDER, PUZZLE_ORDER, sud+9, PUZZLE_ORDER, PUZZLE_ORDER, sud+12, PUZZLE_ORDER, PUZZLE_ORDER, sud+15);
        EXPLAIN_INDENT(h);
        fprintf(h, "|%*.*s|%*.*s|%*.*s|\n", PUZZLE_ORDER, PUZZLE_ORDER, sud+18, PUZZLE_ORDER, PUZZLE_ORDER, sud+21, PUZZLE_ORDER, PUZZLE_ORDER, sud+24);

        EXPLAIN_INDENT(h);
        fprintf(h, "+---+---+---+\n");

        EXPLAIN_INDENT(h);
        fprintf(h, "|%*.*s|%*.*s|%*.*s|\n", PUZZLE_ORDER, PUZZLE_ORDER, sud+27, PUZZLE_ORDER, PUZZLE_ORDER, sud+30, PUZZLE_ORDER, PUZZLE_ORDER, sud+33);
        EXPLAIN_INDENT(h);
        fprintf(h, "|%*.*s|%*.*s|%*.*s|\n", PUZZLE_ORDER, PUZZLE_ORDER, sud+36, PUZZLE_ORDER, PUZZLE_ORDER, sud+39, PUZZLE_ORDER, PUZZLE_ORDER, sud+42);
        EXPLAIN_INDENT(h);
        fprintf(h, "|%*.*s|%*.*s|%*.*s|\n", PUZZLE_ORDER, PUZZLE_ORDER, sud+45, PUZZLE_ORDER, PUZZLE_ORDER, sud+48, PUZZLE_ORDER, PUZZLE_ORDER, sud+51);

        EXPLAIN_INDENT(h);
        fprintf(h, "+---+---+---+\n");

        EXPLAIN_INDENT(h);
        fprintf(h, "|%*.*s|%*.*s|%*.*s|\n", PUZZLE_ORDER, PUZZLE_ORDER, sud+54, PUZZLE_ORDER, PUZZLE_ORDER, sud+57, PUZZLE_ORDER, PUZZLE_ORDER, sud+60);
        EXPLAIN_INDENT(h);
        fprintf(h, "|%*.*s|%*.*s|%*.*s|\n", PUZZLE_ORDER, PUZZLE_ORDER, sud+63, PUZZLE_ORDER, PUZZLE_ORDER, sud+66, PUZZLE_ORDER, PUZZLE_ORDER, sud+69);
        EXPLAIN_INDENT(h);
        fprintf(h, "|%*.*s|%*.*s|%*.*s|\n", PUZZLE_ORDER, PUZZLE_ORDER, sud+72, PUZZLE_ORDER, PUZZLE_ORDER, sud+75, PUZZLE_ORDER, PUZZLE_ORDER, sud+78);

        EXPLAIN_INDENT(h);
        fprintf(h, "+---+---+---+\n");
}

/**************************************************************/
/* Based upon the unsolved Left-to-Right-Top-to-Bottom puzzle */
/* presented in "sbuf", create a 27 octal digit mask of the   */
/* givens in the 28 character buffer pointed to by "mbuf."    */
/* Return a pointer to mbuf after conversion or NULL if sbuf  */
/* contains less than 81 characters. Results are undefined if */
/* mbuf is less than 28 characters in length.                 */
/**************************************************************/

char *cvt_to_mask(char *mbuf, const char *sbuf)
{
        char *mask_buf = mbuf;
        static const char *maskchar = "01234567";
        int i, m;

	if (strlen(sbuf) < PUZZLE_CELLS) return NULL;
        mask_buf[PUZZLE_DIM*3] = 0;
        for (i = 0; i < PUZZLE_CELLS; i += 3) {
	   m = 0;
           if (is_given(sbuf[i])) {
		m |= 4;
           }
           if (is_given(sbuf[i+1])) {
		m |= 2;
           }
           if (is_given(sbuf[i+2])) {
		m |= 1;
           }
           *mask_buf++ = maskchar[m];
        }
	return mbuf;
}

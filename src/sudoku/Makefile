#
#       Copyright (C) 2006 Bill DuPree, All rights reserved
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#
#
# Select your compiler for your favorite platform below

# MGSim
CC		= slc -b mipsel_s
WARNINGS	= -Wall
COMPILE		= -DEXPLAIN
DEBUG		= 
PROC_OPT        =
LD_OPT		= 

# Linux gcc 2.5.8 (or better) compiler
#CC		= gcc
#WARNINGS	= -Wall
#COMPILE		= -pipe -O2 -DEXPLAIN
#COMPILE	= -pipe -O2
#DEBUG		= -g
#PROC_OPT        = -march=i686
#LD_OPT		= -s


# These are the supported compile time options for AIX

#CC		= c89
#WARNINGS	=
#COMPILE	= -O2
#DEBUG		= -g
#LD_OPT		= -s

# These are the supported compile time options for Solaris

#CC		= ucbcc
#WARNINGS	=
#COMPILE	= -Xa -O2
#DEBUG		= -g -xs
#LD_OPT		= -s

# These are the supported compile time options for HPUX

#CC		= c89
#WARNINGS       =
#COMPILE        = -O
#DEBUG          = 
#LD_OPT         = -s

#
#  Don't change anything below this point!
#

CFLAGS = $(DEBUG) $(WARNINGS) $(COMPILE) $(PROC_OPT)
SRCS    = sudoku_solver.c sudoku_engine.c getopt.c

OBJS  = $(SRCS:.c=.o)

sudoku_solver: $(SRCS) $(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) $(LD_OPT) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) sudoku_solver core *~

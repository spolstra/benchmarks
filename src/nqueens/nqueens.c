# include <stdio.h>
# include <stdlib.h>
# include <time.h>

void printArray(int n, int a[]);        /* Just to Print the Final Solution */
void getPositions(int n, int a[],int n1,int n2);  /* The Recursive Function */

int main(int argc, char **argv)
{
    int N, i;
    int *a;

   if (argc != 2) {
      fprintf(stderr, "Usage: %s <N>\n", argv[0]);
      exit(0);
   }

   N = atoi(argv[1]);

   a=(int *)(malloc(sizeof(int)*N));

   for(i=0;i<N;i++)
        getPositions(N, a, 0, i);

   return 0;
}

void printArray(int N, int a[])
{
    int i, j;
    for(i = 0; i < N; ++i)
    {
        for (j = 0; j < N; ++j)
        {
            if (j == a[i])
                putchar('Q');
            else
                putchar('.');
            putchar(' ');
        }
        putchar('\n');
    }
    for (i = 0; i < 2*N; ++i)
        putchar('-');
    putchar('\n');
}

void getPositions(int N, int a1[], int colno, int val)
{
    int ctr1,ctr2;

    a1[colno] = val;

    if(colno == N-1)
    {
        printArray(N, a1);
        return;
    };

    for(ctr1 = 0; ctr1 < N; ctr1++)
    {
        int keepgoing = 0;
        for(ctr2 = 0; ctr2 <= colno; ctr2++)
            if(a1[ctr2]==ctr1 || (colno+1-ctr2)*(colno+1-ctr2)==(ctr1-a1[ctr2])*(ctr1-a1[ctr2]))
            {
                keepgoing = 1;
                break;
            }
        if (!keepgoing)
            getPositions(N, a1, colno+1, ctr1);
    }
}

#include <stdio.h>
#include <stdlib.h>

#ifdef CUSTOM_RANDOM
#include "random.c"
#endif

void print_array(int a[], int size) {
    int i;
    for (i=0; i < size; i++)
        printf("%d ", a[i]);
    printf("\n");
}

void myqsort (int a[], int left, int right) {
    int pivot, store, temp, i;
    if (left >= right)
        return; // lists smaller than 2 don't need sorting.
    pivot = (right + left) / 2;

    // move pivot to end (swap func?)
    temp = a[right]; a[right] = a[pivot]; a[pivot] = temp;

    for (i = store = left; i < right; i++)
        if (a[i] < a[right]) {
            temp = a[i]; a[i] = a[store]; a[store] = temp;
            store++;
        }
    // move pivot back
    temp = a[store]; a[store] = a[right]; a[right] = temp;
    pivot = store;

    // pivot now in correct position..
    // recursively sort arrays to it's left..
    myqsort(a, left, pivot-1);
    // and right..
    myqsort(a, pivot+1, right);

    return;
}

int main(int argc, char* argv[]) {
    int size, i;
    int *a;

    if (argc != 2) {
        printf("Usage %s <array size>\n", argv[0]);
        exit(2);
    }

    size = atoi(argv[1]);
    a = malloc (sizeof (int) * size);

    srandom(1);
    for (i=0; i < size; i++)
        a[i] = random();

    //print_array(a, size);
    myqsort(a, 0, size - 1);
    print_array(a, size);

    return 0;
}

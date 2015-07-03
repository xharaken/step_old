#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

double get_time()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec * 1e-6;
}

double random_01()
{
  return (double)rand() / RAND_MAX;
}

int random_int(int from, int to)
{
  return from + (int64_t)(random_01() * (to - from));
}

int main(int argc, char** argv)
{
  if (argc != 2) {
    printf("usage: %s N\n", argv[0]);
    return -1;
  }

  int n = atoi(argv[1]);
  int* a = (int*)malloc(n * sizeof(int));
  int* b = (int*)malloc(n * sizeof(int));

  int i;
  for (i = 0; i < n; i++) {
    a[i] = i;
  }

  for (i = 0; i < n; i++) {
    //b[i] = 0;
    //b[i] = i;
    b[i] = random_int(0, n);
  }

  double begin = get_time();

  int sum = 0;
  for (i = 0; i < n; i++) {
    sum += a[b[i]];
  }

  double end = get_time();
  printf("time: %.6lf sec\n", end - begin);

  printf("sum: %d\n", sum);

  free(a);
  free(b);
  return 0;
}

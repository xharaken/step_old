#define _GNU_SOURCE
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define FAST 1

#define min(x, y) (x) <= (y) ? (x) : (y)

double get_time()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec * 1e-6;
}

void bind_to_cpu(int cpu)
{
  cpu_set_t cpu_mask;
  
  CPU_ZERO(&cpu_mask);
  CPU_SET(cpu, &cpu_mask);
  int ret = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);
  if (ret < 0)
    fprintf(stderr, "failed at sched_setaffinity\n");
}

double* allocate_matrix(int n)
{
#if FAST
  const int align = 4096;
  const int align_mask = align - 1;
  intptr_t ptr = (intptr_t)malloc(n * n * sizeof(double) + align);
  return (double*)((ptr + align_mask) & (~align_mask));
#else
  return (double*)malloc(n * n * sizeof(double));
#endif
}

void multiply_normal(double* a, double* b, double* c, const int n)
{
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      for (int k = 0; k < n; k++) {
        c[i * n + j] += a[i * n + k] * b[k * n + j];
      }
    }
  }
}

void multiply_fast(double* restrict a, double* restrict b, double* restrict c, const int n)
{
  // Maunally optimized.
  const int prefetch = 64;
  const int block = 32;

  int i, j, k;
  // Cache blocking for i and k.
  for (int ii = 0; ii < n; ii += block) {
    for (int kk = 0; kk < n; kk += block) {
      double* ap = &a[ii * n + kk];
      double* bp_head = &b[kk * n];
      double* cp_head = &c[ii * n];
      int imax = min(ii + block, n);
      for (i = ii; i < imax; i++) {
        double* bp = bp_head;
        int kmax = min(kk + block, n);
        for (k = kk; k < kmax; k++) {
          double* cp = cp_head;
          __asm__("movsd (%0),%%xmm9"::"r"(ap));
          __asm__("movddup %xmm9,%xmm0");
          // Loop unrolling.
          for (j = 0; j + 8 <= n; j += 8) {
            __asm__("prefetcht0 (%0)"::"r"(bp + prefetch));
            __asm__("prefetcht0 (%0)"::"r"(cp + prefetch));
            __asm__("movapd (%0),%%xmm1"::"r"(bp));
            __asm__("movapd (%0),%%xmm2"::"r"(bp + 2));
            __asm__("movapd (%0),%%xmm3"::"r"(bp + 4));
            __asm__("movapd (%0),%%xmm4"::"r"(bp + 6));
            __asm__("mulpd %xmm0,%xmm1");
            __asm__("mulpd %xmm0,%xmm2");
            __asm__("mulpd %xmm0,%xmm3");
            __asm__("mulpd %xmm0,%xmm4");
            __asm__("movapd (%0),%%xmm5"::"r"(cp));
            __asm__("movapd (%0),%%xmm6"::"r"(cp + 2));
            __asm__("movapd (%0),%%xmm7"::"r"(cp + 4));
            __asm__("movapd (%0),%%xmm8"::"r"(cp + 6));
            __asm__("addpd %xmm1,%xmm5");
            __asm__("addpd %xmm2,%xmm6");
            __asm__("addpd %xmm3,%xmm7");
            __asm__("addpd %xmm4,%xmm8");
            __asm__("movapd %%xmm5,(%0)"::"r"(cp));
            __asm__("movapd %%xmm6,(%0)"::"r"(cp + 2));
            __asm__("movapd %%xmm7,(%0)"::"r"(cp + 4));
            __asm__("movapd %%xmm8,(%0)"::"r"(cp + 6));
            bp += 8;
            cp += 8;
          }
          for (; j < n; j++) {
            (*cp) += (*ap) * (*bp);
            bp++;
            cp++;
          }
          ap++;
        }
        ap += n - kmax + kk;
        cp_head += n;
      }
    }
  }
}

int main(int argc, char** argv)
{
  if (argc != 2) {
    printf("usage: %s N\n", argv[0]);
    return -1;
  }

  int n = atoi(argv[1]);

#if FAST
  bind_to_cpu(4);
#endif

  double* a = allocate_matrix(n); // Matrix A
  double* b = allocate_matrix(n); // Matrix B
  double* c = allocate_matrix(n); // Matrix C

  // Initialize the matrices to some values.
  int i, j;
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      a[i * n + j] = i * n + j; // A[i][j]
      b[i * n + j] = j * n + i; // B[i][j]
      c[i * n + j] = 0; // C[i][j]
    }
  }

  double begin = get_time();

  // Write code to calculate C = A * B.
#if FAST
  multiply_fast(a, b, c, n);
#else
  multiply_normal(a, b, c, n);
#endif

  double end = get_time();

  printf("time: %.6lf sec\n", end - begin);

  // Print C for debugging. Comment out the print before measuring the execution time.
  double sum = 0;
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      sum += c[i * n + j];
      // printf("c[%d][%d]=%lf\n", i, j, c[i * n + j]);
    }
  }
  // Print out the sum of all values in C.
  // This should be 450 for N=3, 3680 for N=4, and 18250 for N=5.
  printf("sum: %.6lf\n", sum);
  return 0;
}

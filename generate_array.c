#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MRAND_A 645
#define MRAND_B 1234567
#define MRAND_X0 137
#define MRAND_E32 (1.0 / (1 << 16) / (1 << 16))
#define MRAND_E64 (1.0 / (1 << 16) / (1 << 16) / (1 << 16) / (1 << 16))

uint64_t mrand_x[521];
int8_t mrand_init_flag = 0;
uint64_t mrand_cur;
uint64_t mrand_cur2;

void mrand_init(int64_t x0)
{
  int i;
  mrand_x[0] = MRAND_A * x0;
  for (i = 1; i < 521; i++)
    mrand_x[i] = MRAND_A * mrand_x[i - 1] + MRAND_B;
  mrand_init_flag = 1;
  mrand_cur = 0;
  mrand_cur2 = 521;
}

double mrand_01(void)
{
  int i;
  if (!mrand_init_flag) {
    mrand_x[0] = MRAND_A * MRAND_X0;
    for (i = 1; i < 521; i++)
      mrand_x[i] = MRAND_A * mrand_x[i - 1];
    mrand_init_flag = 1;
    mrand_cur = 0;
    mrand_cur2 = 521;
  }
  mrand_x[mrand_cur] = mrand_x[mrand_cur] ^ mrand_x[mrand_cur2 - 32];
  double d = mrand_x[mrand_cur] * MRAND_E64;
  mrand_cur++;
  mrand_cur2++;
  if (mrand_cur == 521)
    mrand_cur = 0;
  if (mrand_cur2 == 553)
    mrand_cur2 = 32;
  return d;
}

int mrand_int(int64_t inf, int64_t sup)
{
  return inf + (int)(mrand_01() * (sup - inf + 1));
}

int main(int argc, char** argv)
{
  if (argc != 3) {
    printf("usage: %s <filename> <size (MB)>\n", argv[0]);
    return -1;
  }

  char* filename = argv[1];
  int size = atoi(argv[2]);
  FILE* fp = fopen(filename, "wb");
  if (fp == NULL) {
    printf("error: Cannot open file %s\n", filename);
    return -1;
  }

  int i, j;
  int* buffer = (int*)malloc(1000 * 1000);
  for (i = 0; i < size; i++) {
    int num = 1000 * 1000 / sizeof(int);
    for (j = 0; j < num; j++) {
      int r = mrand_int(0, (1ULL << 31) - 1);
      assert(r >= 0);
      buffer[j] = r;
    }
    size_t ret = fwrite(buffer, sizeof(int), num, fp);
    if (ret != num) {
      printf("error: Failed in writing bytes to the file %s\n", filename);
      return -1;
    }
  }
  free(buffer);
  fclose(fp);
  return 0;
}

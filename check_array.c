#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
  if (argc != 2) {
    printf("usage: %s <filename>\n", argv[0]);
    exit(1);
  }

  char* filename = argv[1];
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    printf("error: Cannot open file %s\n", filename);
    exit(1);
  }

  int current = 0;
  int num = 1000 * 1000 / sizeof(int);
  int* buffer = (int*)malloc(num * sizeof(int));
  while (1) {
    size_t ret = fread(buffer, sizeof(int), num, fp);
    if (ret == 0)
      break;
    int i;
    for (i = 0; i < num; i++) {
      if (buffer[i] < current) {
        printf("Error: The array is not sorted. %d => %d is found.\n", current, buffer[i]);
        exit(1);
      }
      current = buffer[i];
    }
  }
  free(buffer);
  fclose(fp);
  if (current == 0) {
    printf("Error: The file exists, but the array is broken. The array contains only 0.\n");
    exit(1);
  }

  printf("OK: The array is correctly sorted!\n");
  return 0;
}

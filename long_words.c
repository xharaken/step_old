#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_MAX 1483277 // # of lines of pages.txt

char** read_pagefile(char *filename) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    printf("Couldn't find file: %s\n", filename);
    exit(1);
  }

  char** words = (char**)malloc(PAGE_MAX * sizeof(char*));
  int index = 0;
  while (1) {
    char buffer[4096];
    int id;
    int ret = fscanf(file, "%d\t%4095s\n", &id, buffer);
    if (ret == EOF) {
      fclose(file);
      break;
    } else {
      int length = strlen(buffer);
      if (length == 4095) {
        printf("Word is too long\n");
        exit(1);
      }
      char* word = (char*)malloc(length + 1);
      strcpy(word, buffer);
      assert(index < PAGE_MAX);
      words[index++] = word;
    }
  }
  return words;
}

int qsort_function(const void* data1, const void* data2) {
  char* word1 = *(char**)data1;
  char* word2 = *(char**)data2;
  return (int)(strlen(word2) - strlen(word1));
}

int main(void) {
  char** words = read_pagefile("pages.txt");
  qsort(words, PAGE_MAX, sizeof(char*), qsort_function);

  int i;
  for (i = 0; i < 30; i++) {
    printf("%s\n", words[i]);
  }
  return 0;
}

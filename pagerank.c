// Don't forget to add the '-lm' option to link math.h.
// $ gcc pagerank.c -lm
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINK_MAX 52973671 // links.txtの総行数
#define PAGE_MAX 1483277  // pages.txtの総行数

typedef struct link_t {
  int src_id; // リンク元のページのid
  int dst_id; // リンク先のページのid
} link_t;

typedef struct page_t {
  int id;          // ページのid
  char* word;      // ページの見出し語
  int linking_num; // このページから出ているリンク数
  int linked_num;  // このページへ入ってくるリンク数
  double pagerank; // ページランク
} page_t;

link_t links[LINK_MAX]; // 大きさLINK_MAXの配列を宣言
page_t pages[PAGE_MAX]; // 大きさPAGE_MAXの配列を宣言

int link_num = 0; // リンク情報はlinks[link_num]まで
int page_num = 0; // ページ情報はpages[page_num]まで


// Calculate the pagerank.
// The calculated result is written to pages[i].pagerank for each i.
// For the pagerank algorithm, see this slide:
// https://docs.google.com/presentation/d/1HbKpCDB0-msWlzjctjpHFi1nRNS6bDrPNAYDXRVUB0s/edit#slide=id.g29606b155_1223
void calculate_pagerank() {
  double damping_factor = 0.85;

  int id, link;
  for (id = 0; id < page_num; id++) {
    pages[id].linking_num = 0;
    pages[id].linked_num = 0;
    pages[id].pagerank = 1.0; // Intialize the pagerank to 1.0.
  }
  for (link = 0; link < link_num; link++) { // Count up the number of ongoing and incoming links for all nodes.
    pages[links[link].src_id].linking_num++;
    pages[links[link].dst_id].linked_num++;
  }
  
  double* new_pageranks = (double*)malloc(sizeof(double) * page_num);
  int iteration;
  for (iteration = 0; iteration < 100000; iteration++) {
    // This is the core part of the pagerank algorithm.
    // We update the pageranks with the following formula:
    //
    //   new_pageranks(i) = (1 - damping_factor) + dampling_factor * \sum (pagerank(j) / outdegree(j)).
    //
    // The summation is taken for all j's that have links to the page i.
    // outdegree(j) indicates the number of outgoing links from the page j.
    for (id = 0; id < page_num; id++) {
      new_pageranks[id] = 1 - damping_factor;
    }
    for (link = 0; link < link_num; link++) {
      int src_id = links[link].src_id;
      int dst_id = links[link].dst_id;
      new_pageranks[dst_id] += damping_factor * pages[src_id].pagerank / pages[src_id].linking_num;
    }

    // This is a subtle part to fix up the pageranks calculated in the above.
    // The problem is that there are pages that don't have any outgoing links (let's call
    // these pages "orphaned pages"). Since the above loop only distributes pageranks
    // of pages that have outgoing links, the pageranks of the orphaned pages are lost.
    // This is problematic because it breaks the invariant that the summation
    // of the pageranks for all pages must be kept equal to page_num. To fix the problem,
    // we distribute the pageranks of the orphaned pages evenly to all pages.
    double orphaned_pageranks = 0.0;
    for (id = 0; id < page_num; id++) {
      if (pages[id].linking_num == 0) { // Count up the pageranks of the orphaned pages.
        orphaned_pageranks += pages[id].pagerank;
      }
    }
    for (id = 0; id < page_num; id++) { // Distribute the pageranks evenly to all pages.
      new_pageranks[id] += damping_factor * orphaned_pageranks / page_num;
    }

    double sum = 0; // sum calculates \sum pagerank(i).
    double norm = 0; // norm calculates \sum (new_pagerank(i) - pagerank(i))^2.
    for (id = 0; id < page_num; id++) {
      double delta = new_pageranks[id] - pages[id].pagerank;
      norm += delta * delta;
      sum += new_pageranks[id];
    }
    printf("# [iteration = %d]\n", iteration);
    // The invariant of the pagerank algorithm is that the summation of the pageranks
    // for all pages must be kept equal to page_num. In other words, sum must be equal
    // to page_num. Otherwise, the above code has a bug.
    printf("# sum = %.2lf, norm = %.5lf\n", sum, norm); 

    if (norm < 0.01) { // Finish the iteration when the pageranks converge.
      break;
    }

    for (id = 0; id < page_num; id++) { // Update the pageranks before going to the next iteration.
      pages[id].pagerank = new_pageranks[id];
    }
  }

  free(new_pageranks);
}

int qsort_function(const void* data1, const void* data2) { // Sorting function for qsort().
  page_t* page1 = (page_t*)data1;
  page_t* page2 = (page_t*)data2;
  return (int)(page2->pagerank - page1->pagerank); // Sort pages by the descending order of the pageranks.
}

// Print the top 10 pages that have the highest pageranks.
void print_highest_pageranks()
{
  // Sort pages by the descending order of the pageranks.
  page_t* sorted_pages = (page_t*)malloc(sizeof(page_t) * page_num);
  int id;
  for (id = 0; id < page_num; id++) {
    sorted_pages[id] = pages[id];
  }
  qsort(sorted_pages, page_num, sizeof(page_t), qsort_function);

  // Print the top 10 pages.
  int i;
  for (i = 0; i < page_num && i < 10; i++) {
    printf("# [%d] %s (rank=%.2lf)\n", i + 1, sorted_pages[i].word, sorted_pages[i].pagerank);
  }
  free(sorted_pages);
}

// Print a pair of (linked_num, pagerank) for all pages.
// Also prints the correlation coefficient between the linked_num and the pagerank.
void print_pageranks_and_linked_nums()
{
  int id;
  for (id = 0; id < page_num; id++) {
    printf("%d %.2lf\n", pages[id].linked_num, pages[id].pagerank);
  }

  // Coefficient(x, y) = \sum (x - avg(x))(y - avg(y)) / sqrt(\sum (x - avg(x))^2) / sqrt(\sum (y - avg(y))^2).
  // See https://en.wikipedia.org/wiki/Correlation_coefficient.
  double avg_pagerank = 0.0;
  double avg_linked_num = 0.0;
  for (id = 0; id < page_num; id++) {
    avg_pagerank += pages[id].pagerank;
    avg_linked_num += pages[id].linked_num;
  }
  avg_pagerank /= page_num;
  avg_linked_num /= page_num;

  double v1 = 0.0;
  double v2 = 0.0;
  double v3 = 0.0;
  for (id = 0; id < page_num; id++) {
    double pagerank_diff = pages[id].pagerank - avg_pagerank;
    double linked_num_diff = pages[id].linked_num - avg_linked_num;
    v1 += pagerank_diff * linked_num_diff;
    v2 += pagerank_diff * pagerank_diff;
    v3 += linked_num_diff * linked_num_diff;
  }
  double coefficent = v1 / sqrt(v2) / sqrt(v3);
  printf("# coefficient between pagerank and linked_num = %.4lf\n", coefficent);
}

void insert_link(int src_id, int dst_id) { // linksの最後にデータを追加する
  if (link_num < LINK_MAX) { // linksに空きがあるなら
    links[link_num].src_id = src_id;
    links[link_num].dst_id = dst_id;
    link_num++;
  } else {
    printf("テーブルがいっぱいです。ごめんなさい。\n");
    exit(1);
  }
}

void insert_page(int id, char* word) { // pagesの最後にデータを追加する
  if (page_num < PAGE_MAX) { // pagesに空きがあるなら
    pages[page_num].id = id;
    pages[page_num].word = word;
    pages[page_num].linking_num = 0;
    pages[page_num].linked_num = 0;
    pages[page_num].pagerank = 0;
    page_num++;
  } else {
    printf("テーブルがいっぱいです。ごめんなさい。\n");
    exit(1);
  }
}

void read_linkfile(char *filename) { // ファイルからリンク情報を読み込む
  FILE* file = fopen(filename, "r");  // ファイルを読み込み専用で開く
  if (file == NULL) { // ファイルが見つからなかったら
    printf("がんばってはみましたが、ファイル%sは見つかりませんでした\n", filename);
    exit(1); // エラーを出して終了
  }

  while (1) {
    int src_id, dst_id;
    int ret = fscanf(file, "%d\t%d\n", &src_id, &dst_id); // 1行リンク情報を読み込む
    if (ret == EOF) { // ファイルの終わりなら
      fclose(file); // ファイルを閉じて
      return; // 終了
    } else { // データがあったなら
      insert_link(src_id, dst_id); // それを追加して読み込みを続ける
    }
  }
}

void read_pagefile(char *filename) { // ファイルからページ情報を読み込む
  FILE* file = fopen(filename, "r"); // ファイルを読み込み専用で開く
  if (file == NULL) { // ファイルが見つからなかったら
    printf("がんばってはみましたが、ファイル%sは見つかりませんでした\n", filename);
    exit(1); // エラーを出して終了
  }

  while (1) {
    int id;
    char buffer[1024];
    int ret = fscanf(file, "%d\t%1023s\n", &id, buffer); // 1行、ページ情報を読み込む
    if (ret == EOF) { // ファイルの終わりなら
      fclose(file); // ファイルを閉じて
      return; // 終了
    } else { // データがあったなら
      int length = strlen(buffer);
      if (length == 1023) {
        printf("なんか見出し語が長すぎ\n");
        exit(1);
      }
      char* word = (char*)malloc(length + 1); // 見出し語を格納するメモリを確保
      strcpy(word, buffer);
      insert_page(id, word); // それを追加して読み込みを続ける
    }
  }
}

int main(void) {
  read_linkfile("links.txt"); // リンク情報を読み込む
  read_pagefile("pages.txt"); // ページ情報を読み込む
  
  calculate_pagerank();
  print_highest_pageranks();
  print_pageranks_and_linked_nums();
  return 0;
}

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define CITY_MAX 2048

double cityX[CITY_MAX];
double cityY[CITY_MAX];
double distance[CITY_MAX][CITY_MAX];
int city_num;

double random_01()
{
  return (double)rand() / RAND_MAX;
}

int random(int from, int to)
{
  return from + (int64_t)(random_01() * (to - from));
}

void copy_state(int* state, int* new_state)
{
  for (int i = 0; i < city_num; i++)
    new_state[i] = state[i];
}

double get_score(int* state)
{
  double score = 0;
  for (int i = 0; i < city_num; i++) {
    int next = (i + 1) % city_num;
    score += distance[state[i]][state[next]];
  }
  return score;
}

void print_score(int* state)
{
  /*
  for (int i = 0; i < city_num; i++) {
    printf("%d ", state[i]);
  }
  printf("\n");
  */
  printf("score: %.2lf\n", get_score(state));
}

void shuffle_random(int* state)
{
  int temp_state[CITY_MAX];
  copy_state(state, temp_state);

  int i = random(0, city_num);
  int delta = 1 + random(0, city_num - 1);
  int j = (i + delta) % city_num;
  int inext = (i + 1) % city_num;
  for (int k = 0; k < delta; k++) {
    state[(inext + k) % city_num] = temp_state[(j - k + city_num) % city_num];
  }
}

bool shuffle_2opt(int* state)
{
  int temp_state[CITY_MAX];
  copy_state(state, temp_state);

  for (int i = 0; i < city_num; i++) {
    for (int j = 0; j < city_num; j++) {
      if (i == j)
        continue;
      int inext = (i + 1) % city_num;
      int jnext = (j + 1) % city_num;
      int delta = j - i;
      double old_value = distance[state[i]][state[inext]] + distance[state[j]][state[jnext]];
      double new_value = distance[state[i]][state[j]] + distance[state[inext]][state[jnext]];
      if (new_value < old_value) {
        for (int k = 0; k < delta; k++) {
          state[(inext + k) % city_num] = temp_state[(j - k + city_num) % city_num];
        }
        return true;
      }
    }
  }
  return false;
}

void solve_random(int* state)
{
  for (int i = 0; i < city_num; i++)
    state[i] = i;

  for (int repeat = 0; repeat < city_num; repeat++) {
    int i = random(0, city_num);
    int j = random(0, city_num);
    int temp = state[i];
    state[i] = state[j];
    state[j] = temp;
  }
}

void solve_greedy(int* state, int current)
{
  bool visited[CITY_MAX];
  for (int i = 0; i < city_num; i++)
    visited[i] = false;

  visited[current] = true;
  state[0] = current;
  for (int i = 1; i < city_num; i++) {
    double min_dist = 1e23;
    int min_city = -1;
    for (int j = 0; j < city_num; j++) {
      if (!visited[j] && min_dist > distance[current][j]) {
        min_dist = distance[current][j];
        min_city = j;
      }
    }
    current = min_city;
    visited[current] = true;
    state[i] = current;
  }
}

void solve_nearest(int* state, int current)
{
  bool visited[CITY_MAX];
  for (int i = 0; i < city_num; i++)
    visited[i] = false;

  visited[current] = true;
  state[0] = current;
  for (int i = 1; i < city_num; i++) {
    double min_dist = 1e23;
    int min_unvisited_city = -1;
    int min_visited_city = -1;
    for (int visited_city = 0; visited_city < city_num; visited_city++) {
      if (!visited[visited_city])
        continue;
      for (int unvisited_city = 0; unvisited_city < city_num; unvisited_city++) {
        if (visited[unvisited_city])
          continue;
        double dist = distance[visited_city][unvisited_city];
        if (dist < min_dist) {
          min_dist = dist;
          min_visited_city = visited_city;
          min_unvisited_city = unvisited_city;
        }
      }
    }
    int j = i - 1;
    for (; j >= 0; j--) {
      if (state[j] == min_visited_city)
        break;
      state[j + 1] = state[j];
    }
    visited[min_unvisited_city] = true;
    state[j + 1] = min_unvisited_city;
  }
}

void solve_hc(int* state, int current)
{
  solve_greedy(state, current);
  while (shuffle_2opt(state)) { }
}

bool judge(double old_score, double new_score, double temperature)
{
  double delta = new_score - old_score;
  if (delta <= 0)
    return true;
  double sigmoid = exp(-delta / (temperature * 3));
  // printf("delta = %.2f, temperature = %.2f, sigmoid = %.2f\n", delta, temperature, sigmoid);
  if (random_01() < sigmoid) {
    // printf("hit!\n");
    return true;
  }
  return false;
}

void solve_sa(int* state, int current)
{
  solve_greedy(state, 0);
  double temperature = 100;
  while (true) {
    int new_state[CITY_MAX];
    copy_state(state, new_state);
    shuffle_random(new_state);
    double old_score = get_score(state);
    double new_score = get_score(new_state);
    // printf("old_score = %.2f, new_score = %.2f\n", old_score, new_score);
    if (judge(old_score, new_score, temperature)) {
      // printf("updated!\n");
      copy_state(new_state, state);
    }
    temperature *= 0.999;
    if (temperature < 1.0)
      break;
  }
}

void solve_hybrid(int* state, int current)
{
  solve_greedy(state, 0);
  double temperature = 100;
  double best_score = 1e23;
  while (true) {
    while (shuffle_2opt(state)) { }

    int new_state[CITY_MAX];
    copy_state(state, new_state);
    shuffle_random(new_state);
    double old_score = get_score(state);
    double new_score = get_score(new_state);
    // printf("old_score = %.2f, new_score = %.2f\n", old_score, new_score);
    if (best_score > old_score) {
      best_score = old_score;
      printf("best_score = %.2f\n", best_score);
    }
    if (judge(old_score, new_score, temperature)) {
      // printf("updated!\n");
      copy_state(new_state, state);
    }
    temperature *= 0.999;
    if (temperature < 1.0)
      break;
  }
}

void solve_test(int* state, int current)
{
  double best_score = 1e23;
  for (int i = 0; i < CITY_MAX; i++) {
    if (i >= city_num) {
      solve_random(state);
    } else {
      solve_greedy(state, i);
    }
    double score1 = get_score(state);
    while (shuffle_2opt(state)) { }
    double score2 = get_score(state);
    if (best_score > score2) {
      best_score = score2;
    }
    print_score(state);
    printf("i = %d: score1 = %.2f, score2 = %.2f, best = %.2f\n", i, score1, score2, best_score);
  }
}

void read_city(char* filename)
{
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    printf("Couldn't open file: %s\n", filename);
    exit(1);
  }
  char buffer[1024];
  char* ret = fgets(buffer, sizeof(buffer), fp);
  assert(ret);
  city_num = 0;
  while (fgets(buffer, sizeof(buffer), fp)) {
    sscanf(buffer, "%lf,%lf", &cityX[city_num], &cityY[city_num]);
    city_num++;
  }
  assert(city_num <= CITY_MAX);
  fclose(fp);

  for (int i = 0; i < city_num; i++) {
    for (int j = 0; j < city_num; j++) {
      distance[i][j] = sqrt((cityX[i] - cityX[j]) * (cityX[i] - cityX[j]) + (cityY[i] - cityY[j]) * (cityY[i] - cityY[j]));
    }
  }
}

int main(int argc, char** argv)
{
  if (argc != 2) {
    printf("usage: %s filename\n", argv[0]);
    exit(1);
  }

  read_city(argv[1]);

  int state[CITY_MAX];
  solve_test(state, 0);
  print_score(state);
  return 0;
}

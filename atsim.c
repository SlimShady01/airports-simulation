/*
 *File: atsim.c
 *Author: Zhenbang Wang
 *Date: July 6. 2019
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <pthread.h>
#include "atcprint.h"
#include "airport.h"

#define ONE_DAY (24 * 60)

static flight_t **completed;
static int comp_n;
static pthread_mutex_t comp_n_mutex;
static int waiting;
static int num_threads;
static pthread_cond_t cond;
static pthread_mutex_t cond_mutex;

static void error(char *err) {
  fprintf(stderr, "%s : %s\n", err, strerror(errno));
  abort();
}

static void barrier() {
  pthread_mutex_lock(&cond_mutex);
  waiting++;
  if (waiting == num_threads) {
    waiting = 0;
    pthread_cond_broadcast(&cond);
  } else {
    pthread_cond_wait(&cond, &cond_mutex);
  }
  pthread_mutex_unlock(&cond_mutex);
}

/* Thread function */
void *thread_func(airport_t apt) {
  flight_t *flight;

  /* Simulate every minutes  */
  for (int i = 0; i < ONE_DAY; i++) {
    flight = airport_step(apt, i);
    if (flight) {
      pthread_mutex_lock(&comp_n_mutex);
      completed[comp_n] = flight;
      comp_n++;
      pthread_mutex_unlock(&comp_n_mutex);
    }
    barrier();  // synchronize with all airports
  }
  return NULL;
}

int main(int argc, char **argv) {
  int i;
  airport_t apt;
  int hour = 0;
  int minute = 0;
  int num = 0;
  char airline[20];
  char origin[20];
  char dest[20];
  flight_t *flight;
  pthread_t *pid;

  while (1) {
    if (scanf("%s", airline) != 1) {
      error("Unexpected scanf failure");
    } else if (!strcmp(airline, "end")) {
      break;
    }

    flight = malloc(sizeof(flight_t));
    assert(flight);
    memset(flight, 0, sizeof(flight_t));

    if ((scanf("%d", &flight->f_no) != 1) || (scanf("%d", &flight->pid) != 1) ||
        (scanf("%s", origin) != 1) || (scanf("%d:%d", &hour, &minute) != 2) ||
        (scanf("%d", &flight->length) != 1) || (scanf("%s", dest) != 1)) {
      error("Unexpected scanf failure");
    }

    strcpy(flight->airline, airline);
    flight->origin = airport_get(origin);
    flight->departure = 60 * hour + minute;
    flight->destination = airport_get(dest);
    airport_schedule(flight);
    num++;
  }

  /* Init vars */
  comp_n = 0;
  completed = malloc(sizeof(flight_t *) * num);
  assert(completed);
  pthread_mutex_init(&comp_n_mutex, NULL);
  waiting = 0;
  num_threads = airport_num();
  pthread_mutex_init(&cond_mutex, NULL);
  pthread_cond_init(&cond, NULL);

  // for (int i = 0; (n < num) && (i < ONE_DAY); i++) {
  //   for (apt = airport_next(NULL); apt != NULL; apt = airport_next(apt)) {
  //     flight = airport_step(apt, i);
  //     if (flight) {
  //       completed[n] = flight;
  //       n++;
  //     }
  //   }
  // }

  /* Create 'airport_num()' threads */
  pid = malloc(sizeof(pthread_t) * airport_num());
  assert(pid);
  for (i = 0, apt = airport_next(NULL); apt != NULL;
       apt = airport_next(apt), ++i) {
    pthread_create(&pid[i], NULL, thread_func, apt);
  }

  /* Wait all threads exit */
  for (i = 0; i < airport_num(); ++i) {
    pthread_join(pid[i], NULL);
  }

  atcprint(completed, comp_n);

  free(pid);
  free(completed);

  return 0;
}

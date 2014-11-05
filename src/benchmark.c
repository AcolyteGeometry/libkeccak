/**
 * libkeccak – Keccak-family hashing library
 * 
 * Copyright © 2014  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <libkeccak.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>


#define MESSAGE_LEN       34520  /* The size of the file LICENSE. */
#ifndef BITRATE
# define BITRATE           1024
#endif
#ifndef CAPACITY
# define CAPACITY           576
#endif
#ifndef OUTPUT
# define OUTPUT             512
#endif

#ifndef UPDATE_RUNS
# define UPDATE_RUNS        100
#endif
#ifndef FAST_SQUEEZE_RUNS
# define FAST_SQUEEZE_RUNS  100
#endif
#ifndef SLOW_SQUEEZE_RUNS
# define SLOW_SQUEEZE_RUNS  100
#endif
#ifndef RERUNS
# define RERUNS              50
#endif



/**
 * Benchmark, will print the number of nanoseconds
 * spent with hashing algorithms and representation
 * convertion from binary to hexadecimal. The latter
 * can be compiled out by compiling with -DIGNORE_BEHEXING.
 * 
 * @return  Zero on success, 1 on error
 */
int main(void)
{
  char message[MESSAGE_LEN];
  libkeccak_spec_t spec;
  libkeccak_state_t state;
  char hashsum[OUTPUT / 8];
  char hexsum[OUTPUT / 8 * 2 + 1];
  struct timespec start, end;
  long i, r;
  
  /* Fill message with connent from the file LICENSE. */
  {
    int fd;
    ssize_t got;
    size_t ptr;
    if (fd = open("LICENSE", O_RDONLY), fd < 0)
      return perror("open"), 1;
    for (ptr = 0; ptr < MESSAGE_LEN; ptr += (size_t)got)
      if (got = read(fd, message, MESSAGE_LEN - ptr), got <= 0)
	return perror("read"), close(fd), 1;
    close(fd);
  }
  
  /* Initialise state. */
  spec.bitrate = BITRATE;
  spec.capacity = CAPACITY;
  spec.output = OUTPUT;
  if (libkeccak_state_initialise(&state, &spec))
    return perror("libkeccak_state_initialise"), 1;
  
  /* Get start-time. */
  if (clock_gettime(CLOCK_MONOTONIC_RAW, &start) < 0)
    return perror("clock_gettime"), 1;
  
  /* Run benchmarking loop. */
  for (r = 0; r < RERUNS; r++)
    {
      /* Updates. */
#if UPDATE_RUNS > 0
      for (i = 0; i < UPDATE_RUNS; i++)
	if (libkeccak_update(&state, message, MESSAGE_LEN) < 0)
	  return perror("libkeccak_update"), 1;
#endif
      
      /* Digest. */
      if (libkeccak_digest(&state, NULL, 0, 0, NULL, hashsum) < 0)
	return perror("libkeccak_digest"), 1;
#ifndef IGNORE_BEHEXING
      libkeccak_behex_lower(hexsum, hashsum, OUTPUT / 8);
#endif
      
      /* Fast squeezes. */
#if FAST_SQUEEZE_RUNS > 0
      libkeccak_fast_squeeze(&state, FAST_SQUEEZE_RUNS);
#endif
      
      /* Slow squeezes. */
#if SLOW_SQUEEZE_RUNS > 0
      for (i = 0; i < SLOW_SQUEEZE_RUNS; i++)
	{
	  libkeccak_squeeze(&state, hashsum);
# ifndef IGNORE_BEHEXING
	  libkeccak_behex_lower(hexsum, hashsum, OUTPUT / 8);
# endif
	}
#endif
    }
  
  /* Get end-time. */
  if (clock_gettime(CLOCK_MONOTONIC_RAW, &end) < 0)
    return perror("clock_gettime"), -1;
  
  /* Print execution-time. */
  end.tv_sec -= start.tv_sec;
  end.tv_nsec -= start.tv_nsec;
  if (end.tv_nsec < 0)
    {
      end.tv_sec--;
      end.tv_nsec += 1000000000L;
    }
  printf("%03li%09li\n", (long)(end.tv_sec), end.tv_nsec);
  
  /* Release resources and exit. */
  libkeccak_state_fast_destroy(&state);
  return 0;
  
#if (FAST_SQUEEZE_RUNS == 0) && (SLOW_SQUEEZE_RUNS == 0)
  (void) i;
#endif
}

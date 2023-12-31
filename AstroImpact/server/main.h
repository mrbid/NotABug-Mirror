#pragma once

#include <signal.h>
#include <stdint.h>
#include <errno.h>

#include "inc/real_syscall.h"

extern int fd;
extern uint32_t main_lock;
extern struct state_holder *states;
extern uint64_t num_states;
extern struct client_holder *clients;
extern uint64_t num_clients;

extern int main_loop_world(void *tmp);
extern int main_loop_send_asteroids(void *tmp);
extern int main_loop_send_players(void *tmp);
extern int main_loop_pod(void *tmp);
void skip_sleep_handler(int signal, siginfo_t *info, void *tmp);

#define SENDTO(fd, buf, len, flags, addr, addrlen) while (real_syscall((uint64_t)fd, (uint64_t)buf, (uint64_t)len, (uint64_t)flags, (uint64_t)addr, (uint64_t)addrlen, (uint64_t)SYS_sendto) == -EINTR);

// bleh on the ; but that'll have far worse results if it's missing, since the compiler won't complain

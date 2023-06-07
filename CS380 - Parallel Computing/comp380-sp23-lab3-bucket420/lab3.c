#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <shmem.h>

#include "wctimer.h"

//#define DEBUG
#ifdef DEBUG
#define dbg_printf(...) dbg_printf_real(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif
#define dprintf(...) dbg_printf_real(__VA_ARGS__)

#define N 10000
#define G 6.67e-11
#define TIMESTEP 0.25
#define NSTEPS 10

struct global_s {
    int rank;                   // index of the calling process
    int nproc;                  // # of parallel processes
    int n;                      // N = number of bodies in simulation
    int nsteps;                 // # of timesteps to run simulation
    int block_size;             // size of the body array on one process
    double *bodies;             // body data in current state
    double *next;               // body data in next state
    double *temp;               // temporary array to store data from another process
    int debug;                  // debug flag
};
typedef struct global_s global_t;


/*
 *  global data structure
 */
global_t g;

/*
 *  prototypes
 */
int    dbg_printf_real(const char *format, ...);
int    eprintf(const char *format, ...);
void   print_body(double *bodies, int i);

/**
 *  dbg_printf - debug printing wrapper, only enabled if DEBUG defined
 *    @param format printf-like format string
 *    @param ... arguments to printf format string
 *    @return number of bytes written to stderr
 */
int dbg_printf_real(const char *format, ...) {
    va_list ap;
    int ret, len;
    char buf[1024], obuf[1024];

    va_start(ap, format);
    ret = vsprintf(buf, format, ap);
    va_end(ap);
    len = sprintf(obuf, "%4d: %s", g.rank, buf);
    write(STDOUT_FILENO, obuf, len);
    return ret;
}


/**
 *  eprintf - error printing wrapper
 *    @param format printf-like format string
 *    @param ... arguments to printf format string
 *    @return number of bytes written to stderr
 */
int eprintf(const char *format, ...) {
    va_list ap;
    int ret;
    char buf[1024];

    if (g.rank == 0) {
        va_start(ap, format);
        ret = vsprintf(buf, format, ap);
        va_end(ap);
        write(STDOUT_FILENO, buf, ret);
        return ret;
    }
    else
        return 0;
}

/**
 * print_body - prints the contents of a body
 *  @param bodies - array storing body data
 *  @param i - index of body to print
 */
void print_body(double *bodies, int i) {
    dprintf("x: %7.3f y: %7.3f z: %7.3f dx: %7.3f dy: %7.3f dz: %7.3f mass: %7.3f\n",
            g.bodies[i], g.bodies[g.block_size + i], g.bodies[2 * g.block_size + i], g.bodies[4 * g.block_size + i], g.bodies[5 * g.block_size + i], g.bodies[6 * g.block_size + i], g.bodies[3 * g.block_size + i]);
}

/**
 * init - initialize global variables
 */
void init() {
    // set number of processes and rank
    g.nproc = shmem_n_pes();
    g.rank  = shmem_my_pe();

    // calculate size of local body array (rounded up)
    g.block_size = ceil((double) g.n / g.nproc);

    // allocate memory on the symmetric heap for g.bodies
    g.bodies = shmem_calloc(7 * g.block_size, sizeof(double));

    // actual index of a body 
    int real_index;

    // initialize every element in g.bodies
    for (int i = 0; i < g.block_size; i++) {
        // convert local index to actual index
        real_index = g.rank * g.block_size + i;

        // ignore extraneous elements
        if (real_index == g.n) 
            break; 

        // assign initial values to x, y, z, dx, dy, dz, mass
        g.bodies[                   i] = 100.0 * (real_index + 0.1);        // x
        g.bodies[    g.block_size + i] = 200.0 * (real_index + 0.1);        // y
        g.bodies[2 * g.block_size + i] = 300.0 * (real_index + 0.1);        // z
        g.bodies[3 * g.block_size + i] = 10e6 * (real_index + 100.2);       // mass
        g.bodies[4 * g.block_size + i] = real_index + 400.0;                // dx
        g.bodies[5 * g.block_size + i] = real_index + 500.0;                // dy
        g.bodies[6 * g.block_size + i] = real_index + 600.0;                // dz
    }

    // allocate memory on the private heap for g.next
    g.next = malloc(7 * g.block_size * sizeof(double));

    // copy everything from g.bodies to g.next
    memcpy(g.next, g.bodies, 7 * g.block_size * sizeof(double));

    // allocate memory on the private heap for g.temp
    // we only need 4 rows (x, y, z, mass) for particles exerting forces
    g.temp = malloc(4 * g.block_size * sizeof(double));

}

/**
 * next_state - compute the next state of all bodies
 * all bodies in the next state are stored in g.next
 */
void next_state() {
    double d_reciprocal;    // the inverse of distance (1 / d)
    double a_over_d;        // acceleration divided by distance (a / d)
    double dx, dy, dz;      // position deltas
    double ax, ay, az;      // acceleration components

    // iterate through all processes to get their data
    for (int p = 0; p < g.nproc; p++) {
        // the bodies that exert forces are stored in g.temp
        if (p == g.rank) {
            memcpy(g.temp, g.bodies, 4 * g.block_size * sizeof(double));
        } else {
            shmem_getmem(g.temp, g.bodies, 4 * g.block_size * sizeof(double), p);
        }
        
        // iterate through all bodies receiving forces (on the calling process only)
        for (int i = 0; i < g.block_size; i++) {
            // the last body has been reached, stop the loop
            if (g.bodies[3 * g.block_size + i] == 0)
                break;

            // set initial acceleration to zero
            ax = ay = az = 0.0;

            // iterate through all bodies exerting forces (can be on either the calling process or a remote process)
            for (int j = 0; j < g.block_size; j++) {
                // skip the same body
                if (p == g.rank && i == j) 
                    continue;

                // compute the distances in each dimension
                dx = g.bodies[i] - g.temp[j];
                dy = g.bodies[g.block_size + i] - g.temp[g.block_size + j];
                dz = g.bodies[2 * g.block_size + i] - g.temp[2 * g.block_size + j];

                // compute the inverse of the distance magnitude
                d_reciprocal = 1.0 / sqrt((dx*dx) + (dy*dy) + (dz*dz));

                // we only care about acceleration so computing force would be redundant
                // a / d = G m_other / d^3
                a_over_d = G  * g.temp[3 * g.block_size + j] * d_reciprocal * d_reciprocal * d_reciprocal; 

                // compute acceleration components in each dimension
                ax += a_over_d * dx;  
                ay += a_over_d * dy;
                az += a_over_d * dz;
            }
            // partially update the body velocity at time t+1
            g.next[4 * g.block_size + i] += TIMESTEP * ax;      // dx
            g.next[5 * g.block_size + i] += TIMESTEP * ay;      // dy
            g.next[6 * g.block_size + i] += TIMESTEP * az;      // dz

            // update the body position at t+1 (only do once)
            if (p == g.rank) {
                g.next[i]                    += TIMESTEP * g.bodies[4 * g.block_size + i];      // x
                g.next[g.block_size + i]     += TIMESTEP * g.bodies[5 * g.block_size + i];      // y
                g.next[2 * g.block_size + i] += TIMESTEP * g.bodies[6 * g.block_size + i];      // z
            }
        }
    }
}

/**
 * main
 */
int main(int argc, char **argv) {
    char c;
    wc_timer_t ttimer; // total time
    wc_timer_t itimer; // per-iteration timer

    memset(&g, 0, sizeof(g));    // zero out global data structure
    shmem_init();                // initialize OpenSHMEM

    // calibrate timer
    wc_tsc_calibrate();

    // set default values
    g.n      = N;
    g.nsteps = NSTEPS;
    g.debug = 0;

    // get arguments
    while ((c = getopt(argc, argv, "hdn:t:")) != -1) {
        switch (c) {
            case 'h':
                eprintf("usage: lab3 [-n #bodies] [-t #timesteps]\n");
                shmem_finalize();
                exit(0);
                break;
            case 'd':
                g.debug = 1;
                break;
            case 'n':
                g.n = atoi(optarg);
                break;
            case 't':
                g.nsteps = atoi(optarg);
                break;
        }
    }

    // initialize global variables
    shmem_barrier_all();
    init();
    shmem_barrier_all();

    // print once
    if (g.rank == 0) {
        eprintf("beginning N-body simulation of %d bodies with %d processes over %d timesteps\n", g.n, g.nproc, g.nsteps);
    }

    // debug print
    if (g.debug && g.rank == 0 && g.block_size >= 10) {
        for (int i = 0; i < 10; i++) {
            print_body(g.bodies, i);
        }
    }

    // start total timer 
    shmem_barrier_all();
    WC_INIT_TIMER(ttimer);
    WC_START_TIMER(ttimer);
    shmem_barrier_all();

    // repeat g.nsteps times
    for (int i = 0; i < g.nsteps; i++) {
        // start iteration timer
        shmem_barrier_all();
        WC_INIT_TIMER(itimer);
        WC_START_TIMER(itimer);
        shmem_barrier_all();

        // calculate next state, store all new values in g.next 
        next_state();
        shmem_barrier_all();

        // next state becomes current state before next iteration
        memcpy(g.bodies, g.next, 7 * g.block_size * sizeof(double));
        shmem_barrier_all();

        // stop iteration timer and print out the time
        WC_STOP_TIMER(itimer);
        if (g.rank == 0) {
            eprintf("timestep %d: %7.4f ms\n", i, WC_READ_TIMER_MSEC(itimer));    
            // debug print
            if (g.debug && g.block_size >= 10) {
                for (int i = 0; i < 10; i++) {
                    print_body(g.bodies, i);
                }
            }
        }

    }
    shmem_barrier_all();

    // stop total timer and print out the time
    WC_STOP_TIMER(ttimer);
    if (g.rank == 0) {
        eprintf("total time: %7.4f ms\n", WC_READ_TIMER_MSEC(ttimer));
    }

    // free allocated memory
    shmem_free(g.bodies);
    free(g.next);
    free(g.temp);

    // finalize OpenSHMEM
    shmem_finalize();
}

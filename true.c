#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <getopt.h>

#define VERSION_STRING "0.0.1"
#define EXP_DESC "false sharing"

#define DEFAULT_THREADS 2
#define DEFAULT_REFS    100

#define LS_PATH "/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size"

#define ERROR(fmt, args...) fprintf(stderr, "EXP-ERR: " fmt, ##args)


size_t
get_l1_line_sz (void)
{
    FILE * p = NULL;
    unsigned sz = 0;

    p = fopen(LS_PATH, "r");
    if (!p) {
        perror("Couldn't open linux cache descriptor file\n");
        return 0;
    }

    fscanf(p, "%d", &sz);
    fclose(p);
    return sz;
}


typedef struct {
    unsigned idx;
    pthread_t thr;
    unsigned long refs;
    unsigned cache_line_sz;
    volatile unsigned char * buf; // this will be a cache line
} parm_t;


static void*
worker (void * in)
{
    int i;
    parm_t * p = (parm_t*)in;
    unsigned int nskip_idx = p->cache_line_sz / sizeof(unsigned char);
    // printf("hello from thread %d (%ld accesses) (%d nskip) \n",
    //        p->idx, p->refs, p->cache_line_sz);

    for (i = 0; i < p->refs; i++) {
        p->buf[p->idx * nskip_idx] = 1;
    }

    return NULL;
}

static void *
alloc_shared_buf (size_t n, unsigned threads)
{
    void * buf = calloc(1, n * threads);

    if (!buf) {
        return NULL;
    }

    // if ((unsigned long)buf & (n-1)) {
    //     printf("test 1\n");
    //     void * adj = (void*)((unsigned long)(buf + n) & ~(n-1));
    //     return adj;
    // }

    return buf;
}


static void
driver (unsigned threads, size_t line_sz, unsigned long refs)
{
    int i;
    void * buf = NULL;

    parm_t * parm_arr = NULL;

    parm_arr = calloc(threads, sizeof(parm_t));
    if (!parm_arr) {
        ERROR("Couldn't allocate parm array\n");
        exit(EXIT_FAILURE);
    }


    buf = alloc_shared_buf(line_sz, threads);

    if (!buf) {
        ERROR("Couldn't allocate shared buf\n");
        exit(EXIT_FAILURE);
    }

    //printf("Allocated aligned shared buf (%p)\n", buf);

    for (i = 0; i < threads; i++) {
        parm_arr[i].idx  = i;
        parm_arr[i].refs = refs;
        parm_arr[i].cache_line_sz = line_sz;
        parm_arr[i].buf  = buf;
        if (pthread_create(&parm_arr[i].thr, NULL, worker, &parm_arr[i]) != 0) {
            ERROR("Couldn't create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < threads; i++) {
        pthread_join(parm_arr[i].thr, NULL);
    }
}



static void
usage (char * prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("\nOptions:\n");

    printf("  -n, --threads <thread count> : number of threads to launch (default=%d, max = l1 line size)\n", DEFAULT_THREADS);
    printf("  -a, --refs <access count> : number of false sharing accesses (default=%d)\n", DEFAULT_REFS);
    printf("  -l, --line_size <line size in bytes> : line size of the cache in bytes (default=%d)\n", DEFAULT_REFS);
    printf("  -h, ---help : display this message\n");
    printf("  -v, --version : display the version number and exit\n");

    printf("\n");
}


static void
version ()
{
    printf("%s measurement code (HExSA Lab 2019)\n", EXP_DESC);
    printf("version %s\n\n", VERSION_STRING);
}


int
main (int argc, char ** argv)
{
    unsigned threads   = DEFAULT_THREADS;
    unsigned long refs = DEFAULT_REFS;
    size_t line_sz     = 0;

    int c;

    while (1) {

        int optidx = 0;

        static struct option lopts[] = {
            {"threads", required_argument, 0, 'n'},
            {"refs", required_argument, 0, 'a'},
            {"line_size", required_argument, 0, 'l'},
            {"help", no_argument, 0, 'h'},
            {"version", no_argument, 0, 'v'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "a:n:l:hv", lopts, &optidx);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 'n':
                threads = atoi(optarg);
                break;
            // line size of the cache in bytes
            case 'l':
                line_sz = atoi(optarg);
                break;
            case 'a':
                refs = strtoul(optarg, NULL, 0);
                break;
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'v':
                version();
                exit(EXIT_SUCCESS);
            case '?':
                break;
            default:
                printf("?? getopt returned character code 0%o ??\n", c);
        }

    }


    if (!line_sz) {
        line_sz = get_l1_line_sz();
    }

    threads = (threads > line_sz) ? line_sz : threads;

    printf("# %s experiment config:\n", EXP_DESC);
    printf("#   %20s : %d\n", "threads", threads);
    printf("#   %20s : %luB\n", "L1 line size", line_sz);
    printf("#   %20s : %lu\n", "memory refs", refs);

    driver(threads, line_sz, refs);

    return 0;
}



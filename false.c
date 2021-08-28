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
#include <stdint.h>
#include <assert.h>

#define VERSION_STRING "0.0.1"
#define EXP_DESC "false sharing"

#define DEFAULT_THREADS 2
#define DEFAULT_REFS    100
#define DEFAULT_LINE_SIZE_BYTES 64
#define DEFAULT_SKIP_WRITING    1

#define LS_PATH "/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size"

#define ERROR(fmt, args...) fprintf(stderr, "EXP-ERR: " fmt, ##args)

typedef struct {
    uint64_t idx;
    pthread_t thr;
    uint64_t refs;
    uint64_t cache_line_sz;
    uint64_t skip_writing;
    volatile uint64_t * buf; // this will be a cache line
} parm_t;

pthread_barrier_t   barrier;

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

    int32_t ret = fscanf(p, "%d", &sz);
    fclose(p);
    return ret == 1 ? sz : 0;
}


static void*
worker (void * in)
{
    uint64_t i;
    parm_t * p = (parm_t*)in;
    // printf("# hi thread %ld %ld accesses %ld index\n",
    //        p->idx, p->refs, p->idx);

    for (i = 0; i < p->refs; i++) {
        if(i % p->skip_writing == 0) {
            // printf("# thread %d %d/%ld accesses %ld skip_writing\n",
            //        p->idx, i, p->refs, p->skip_writing);
            __sync_add_and_fetch(&(p->buf[p->idx]), 1);
            // skip_writing = 0;
        }
        // printf("thread %d is waitting in barrier\n", p->idx);
        pthread_barrier_wait(&barrier);
        // printf("thread %d is out of barrier\n", p->idx);
    }

    printf("# thread %lu value %lu\n", p->idx, p->buf[p->idx]);
    return NULL;
}


static void *
alloc_shared_buf (size_t n)
{
    void * buf = calloc(1, n*2);

    if (!buf) {
        return NULL;
    }

    // make sure that this function returns the aligned shared buf
    if ((unsigned long)buf & (n-1)) {
        void * adj = (void*)((unsigned long)(buf + n) & ~(n-1));
        return adj;
    }

    return buf;
}


static void
driver (
    unsigned threads,
    size_t line_sz,
    unsigned long refs,
    unsigned long skip_writing)
{
    int i;
    void * buf = NULL;

    parm_t * parm_arr = NULL;

    parm_arr = calloc(threads, sizeof(parm_t));
    if (!parm_arr) {
        ERROR("Couldn't allocate parm array\n");
        exit(EXIT_FAILURE);
    }


    buf = alloc_shared_buf(line_sz);
    if (!buf) {
        ERROR("Couldn't allocate shared buf\n");
        exit(EXIT_FAILURE);
    }

    printf("Allocated aligned shared buf (%p)\n", buf);

    for (i = 0; i < threads; i++) {
        parm_arr[i].idx  = i;
        parm_arr[i].refs = refs;
        parm_arr[i].skip_writing = skip_writing;
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

    printf("  -n, --threads <thread count> : number of threads to launch (default=%d, max = l1 line size)\n",
           DEFAULT_THREADS);
    printf("  -a, --refs <access count> : number of false sharing accesses (default=%d)\n",
           DEFAULT_REFS);
    printf("  -l, --line_size <line size in bytes> : line size of the cache in bytes (default=%d)\n",
           DEFAULT_LINE_SIZE_BYTES);
    // skip_writing = 0 means that it keeps writing to the share object/memory
    // skip_writing = 10 means that it skips writing to the share object/memory
    // for 10 times. If a = 100 and s = 10, it only writes to the share
    // object/memory for 10 times.
    printf("  -s, --skip_writing <times> : line size of the cache in bytes (default=%d)\n",
           DEFAULT_SKIP_WRITING);
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
    uint64_t threads   = DEFAULT_THREADS;
    uint64_t refs = DEFAULT_REFS;
    uint64_t skip_writing = DEFAULT_SKIP_WRITING;
    size_t line_sz     = 0;

    int c;

    while (1) {

        int optidx = 0;

        static struct option lopts[] = {
            {"threads", required_argument, 0, 'n'},
            {"refs", required_argument, 0, 'a'},
            {"line_size", required_argument, 0, 'l'},
            {"skip_writing", optional_argument, 0, 's'},
            {"help", no_argument, 0, 'h'},
            {"version", no_argument, 0, 'v'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "a:n:l:s:hv", lopts, &optidx);

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
            case 's':
                skip_writing = atoi(optarg);
                // assert(skip_writing != 0 && "skip_writing should not be 0\n");
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

    threads = (threads > line_sz / sizeof(uint64_t)) ? line_sz : threads;

    if(pthread_barrier_init(&barrier, NULL, threads)) {
        ERROR("Couldn't init barrier\n");
        exit(EXIT_FAILURE);
    }

    printf("# %s experiment config:\n", EXP_DESC);
    printf("#   %20s : %lu\n", "threads", threads);
    printf("#   %20s : %luB\n", "L1 line size", line_sz);
    printf("#   %20s : %lu\n", "skip_writing", skip_writing);
    printf("#   %20s : %lu\n", "memory refs", refs);

    driver(threads, line_sz, refs, skip_writing);

    if(pthread_barrier_destroy(&barrier)) {
        ERROR("Couldn't destroy barrier\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

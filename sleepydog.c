#ifdef OSAIX
#include <sys/processor.h>
#include <sys/thread.h>
#else
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h> 

unsigned long desired_sleep_duration_ms = 25;
unsigned long sleep_duration_threshold_ms = 50;
struct timespec interval;
FILE *logfile = NULL;

void usage() {
    printf("sleepydog will create and bound one thread for every logical cpu in the system.\n");
    printf("Every thread will sleep for desired_sleep_duration_ms and check actual sleep time using CLOCK_REALTIME.\n");
    printf("Program will make log record if actual sleep time exeeds sleep_duration_threshold_ms.\n");
    printf("\n");
    printf("Syntax: sleepydog [-s desired_sleep_duration_ms] [-t sleep_duration_threshold_ms] [-r run_time_s] [-l log_file]\n");
    printf("        -s desired_sleep_duration_ms: sleep for desired_sleep_duration_ms milliseconds [default: 25]\n");
    printf("        -t sleep_duration_threshold_ms: make log record if sleep takes more than sleep_duration_threshold_ms milliseconds [default: 50]\n");
    printf("        -r run_s: exit after run_s seconds (0 - unlimited run time) [default: 0]\n");
    printf("        -l log_file: write log to log_file instead of stdout (file will be appended or created if not exist)\n");
}

struct timespec ms_to_timespec(unsigned long msecs)
{
    struct timespec ts;
    ts.tv_sec = msecs / 1000;
    ts.tv_nsec = msecs % 1000 * 1000000;
    return ts;
}

unsigned long timespec_to_ms(struct timespec *ts)
{
    long msecs = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
    return msecs;
}

void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result)
{
    if (stop->tv_nsec < start->tv_nsec) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
    return;
}

void get_timestamp(time_t *ti_t, char dt_str[])
{
    struct tm temp_tm;
    time_t temp_time_t;
    if (ti_t == NULL) {
        time(&temp_time_t);
    } else {
        temp_time_t = *ti_t; 
    }
    localtime_r(&temp_time_t, &temp_tm);
    strftime(dt_str, 20, "%Y-%m-%d %H:%M:%S", &temp_tm);
}

void *sleepyhead(void *cpu_number)
{
    int cpu = (int)(uintptr_t)cpu_number;
    char datetime_str[20];
 
#ifdef OSAIX
    tid_t self_tid = thread_self();
    int rc = bindprocessor(BINDTHREAD, self_tid, cpu);
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_t thread = pthread_self();
    int rc = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#endif

    get_timestamp(NULL, datetime_str);
    if (rc){
        fprintf(logfile, "%19s could not set affinity for cpu %d because of error %d. Exiting thread.\n", datetime_str, cpu, rc);
        pthread_exit(NULL);
    }
    fprintf(logfile, "%19s thread affinity for cpu %d is set.\n", datetime_str, cpu);
 
    struct timespec start_ts, end_ts, diff_ts;
 
    while (1){
        clock_gettime(CLOCK_REALTIME, &start_ts); 
        nanosleep(&interval, NULL);
        clock_gettime(CLOCK_REALTIME, &end_ts); 
        timespec_diff(&start_ts, &end_ts, &diff_ts);
        unsigned long diff_ms = timespec_to_ms(&diff_ts);
        if (diff_ms > sleep_duration_threshold_ms){
            get_timestamp(&end_ts.tv_sec, datetime_str);
            fprintf(logfile, "%19s cpu: %-3d desired_sleep_ms: %-10ld actual_sleep_ms: %-10ld\n", datetime_str, cpu, desired_sleep_duration_ms, diff_ms); 
        }
    }
}

int main (int argc, char *argv[])
{

    unsigned int run_time_s = 0;
    
    char dt[20];
    get_timestamp(NULL, dt);

    int opt;
    while ((opt = getopt(argc, argv, "s:t:r:l:h")) != -1) {
        switch (opt) {
            case 'h': 
                usage();
                exit(0);
                break;
            case 's': 
                desired_sleep_duration_ms = strtol(optarg, NULL, 10);
                break;
            case 't': 
                sleep_duration_threshold_ms = strtol(optarg, NULL, 10);
                break;
            case 'r': 
                run_time_s = strtol(optarg, NULL, 10);
                break;
            case 'l': 
                logfile = fopen(optarg, "a");
                if (logfile == NULL) {
                    printf("%19s error fopening %s for writing: %d\n", dt, optarg, errno);
                    exit(-1);
                }
                printf ("%19s logfile: %s\n", dt, optarg);
                break;
            case '?': 
                usage();
                exit(-1);
                break;
        }
    }
   
    interval = ms_to_timespec(desired_sleep_duration_ms);

    if(logfile == NULL){
        logfile = stdout;
    }

    fprintf(logfile, "%19s starting with desired_sleep_duration_ms %ld and sleep_duration_threshold_ms %ld\n", dt, desired_sleep_duration_ms, sleep_duration_threshold_ms); 

    int cpu_count = (int) sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t threads[cpu_count];
    for(unsigned int cpu=0; cpu<cpu_count; cpu++){
        int rc = pthread_create(&threads[cpu], NULL, sleepyhead, (void *)(uintptr_t)cpu);
        if (rc){
           get_timestamp(NULL, dt);
           fprintf(logfile, "%19s could not create thread because of error %d. Exiting.\n", dt, rc);
           exit(-1);
        }
    }

    unsigned int curcycle = 0; 
    while (run_time_s == 0 || run_time_s > curcycle){
        fflush(logfile);
        sleep(1);
        curcycle++;
    }
    exit(0);
}

## Sleepydog - lightweight execution delays detector

### Usage

~~~
$ ./sleepydog -h
sleepydog will create and bound one thread for every logical cpu in the system.
Every thread will sleep for desired_sleep_duration_ms and check actual sleep time using CLOCK_REALTIME.
Program will make log record if actual sleep time exeeds sleep_duration_threshold_ms.

Syntax: sleepydog [-s desired_sleep_duration_ms] [-t sleep_duration_threshold_ms] [-r run_time_s] [-l log_file]
        -s desired_sleep_duration_ms: sleep for desired_sleep_duration_ms milliseconds [default: 25]
        -t sleep_duration_threshold_ms: make log record if sleep takes more than sleep_duration_threshold_ms milliseconds [default: 50]
        -r run_s: exit after run_s seconds (0 - unlimited run time) [default: 0]
        -l log_file: write log to log_file instead of stdout (file will be appended or created if not exist)
~~~

### Compile

* Linux
  ~~~
  cc -std=c11 -pthread -o sleepydog sleepydog.c
  ~~~

* AIX

  IBM xlc compiler needed to build binary.
  ~~~
  /usr/vac/bin/xlc_r -DOSAIX -o sleepydog sleepydog.c
  ~~~

### Log analysis example

~~~
# egrep '2021-12-28 23:[23]' /var/log/sleepydog.log | awk '/actual_sleep_ms:/{TSTMP=$1" "substr($2,1,7); if(TS && TS!=TSTMP){print TS" "asort(CPU_DELAYS)" "MAXDELAY; delete CPU_DELAYS; MAXDELAY=0};TS=TSTMP;CPU_DELAYS[$4]++; MAXDELAY=($NF>MAXDELAY)?$NF:MAXDELAY}END{print TS" "asort(CPU_DELAYS)" "MAXDELAY;}'
2021-12-28 23:28:0 3 57
2021-12-28 23:28:1 1 54
2021-12-28 23:28:2 4 58
2021-12-28 23:28:3 1 55
2021-12-28 23:29:0 1 54
2021-12-28 23:29:2 1 51
2021-12-28 23:29:4 1 51
2021-12-28 23:29:5 2 54
2021-12-28 23:30:0 1 51
2021-12-28 23:34:1 2 113
#
~~~

#### Columns

    1. Date
    2. Time with 10 seconds precision
    3. Count of CPUs with delays in the correspondind time span
    4. Duration of the longest execution delay
    
  > Note: SMI always cause simultaneous delay on all CPUs

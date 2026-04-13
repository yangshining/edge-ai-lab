#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <time.h>
#include <signal.h>

#include <syslog.h>

#define RLOG_SYSLOG_DAEMON_PID "/var/run/syslogd.pid"

static inline void write_rlog(char *rs_reason, char *cause, char *prog, pid_t pid, char *desc)
{
    char *info = NULL;
    size_t length = 0;  
    FILE *file;
    pid_t rsyslog;
    char mydate[20];

    if (!rs_reason) 
        rs_reason = "-";
    if (!cause) 
        cause = "-";
    if (!prog) 
        prog = "-";
    if (!desc) 
        desc = "-";

    time_t cu = time(NULL);
    struct tm *t = localtime(&cu);
    sprintf(mydate, "%.04d-%.02d-%.02d %.02d:%.02d:%.02d",
            t->tm_year + 1900, 
            t->tm_mon + 1, 
            t->tm_mday,
            t->tm_hour, 
            t->tm_min, 
            t->tm_sec);

    openlog("rlog", LOG_NDELAY, LOG_LOCAL6);
    if (!pid) {
        syslog(LOG_ERR, "@ %s @ %s @ %s @ %s @ %s @ %s", rs_reason, cause, prog, "-", desc, mydate);
    } else {
        syslog(LOG_ERR, "@ %s @ %s @ %s @ %d @ %s @ %s", rs_reason, cause, prog, pid, desc, mydate);
    }
    closelog();

    //we don't use legacy syslogd from busybox impl, so check rsyslogd status
    file = fopen(RLOG_SYSLOG_DAEMON_PID, "r");
    if (file != NULL) {
        if (getline(&info, &length, file) != -1) {
            rsyslog = (pid_t) strtol(info, NULL, 10);
            kill(rsyslog, SIGHUP);
            usleep(4000);
        }
        fclose(file);
    }
}


/****************************************************************/
/*                                                              *
 *        LICENSE for nsn aka Network Status Notifier           *
 *                                                              *
 *  It is allowed to use this program for free, as long as:     *
 *                                                              *
 *      -You use this as permitted by local laws.               *
 *      -You do not use it for malicious purposes like          *
 *      harming networks by messing up arp tables etc.          *
 *      -You understand and accept that any harm caused by      *
 *      using this program is not program author's fault.       *
 *      -You let me know if you liked this, my mail             *
 *      Mazziesaccount@gmail.com is open for comments.          *
 *                                                              *
 *  It is also allowed to redistribute this program as long     *
 *  as you maintain this license and information about          *
 *  original author - Matti Vaittinen                           *
 *  (Mazziesaccount@gmail.com)                                  *
 *                                                              *
 *  Modifying this program is allowed as long as:               *
 *                                                              *
 *      -You maintain information about original author/SW      *
 *      BUT also add information that the SW you provide        *
 *      has been modified. (I cannot provide support for        *
 *      modified SW.)                                           *
 *      -If you correct bugs from this software, you should     *
 *      send corrections to me also (Mazziesaccount@gmail.com   *
 *      so I can include fixes to official version. If I stop   *
 *      developing this software then this requirement is no    *
 *      longer valid.                                           *
 *                                                              *
 *                                                              *
 *                                                              *
 *                                                              *
 ****************************************************************/




#include "nsn_printfuncs.h"
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

static printdev *G_printdevices[MAX_SUPPORTED_PRINT_DEVICES]={NULL,NULL};
static Everbosity G_verbositylimit=NORMAL_VERBOSITY;

static printdev *init_stdout();
static printdev *init_fileout(FILE *fptr,int *fd);

static struct timespec G_starttime={
    .tv_sec=0,
    .tv_nsec=0
};

int set_relative_logtimes()
{
    if(clock_gettime(CLOCK_REALTIME,&G_starttime))
    {
        NSNpanicPrint("Failed to get time - times disabled");
        return -1;
    }
    return 0;
}

/* Not thread safe, but I'm not going to use threads =) */
void stdout_prfunc(printdev*_this,const char *str)
{
    printf("%s\n",str);
}

int setverbosity(Everbosity verb)
{
   if((unsigned)verb>(unsigned)MAX_VERBOSITY) 
       return -1;
   G_verbositylimit=verb;
   return 0;
}

void stdout_errorprfunc(printdev*_this,const char *str)
{
    fprintf(stderr,"%s\n",str);
    fflush(stdout);
    fflush(stderr);
}

void fileout_prfunc(printdev* _this_,const char *str)
{
    fileprintdev *_this=(fileprintdev *)_this_;

    if(_this->isopen)
    {
        fprintf(_this->fptr,"%s\n",str);
        fflush(_this->fptr);
    }
}
void fileout_errprfunc(printdev* _this_,const char *str)
{
    fileprintdev *_this=(fileprintdev *)_this_;
    if(_this->isopen)
    {
        fprintf(_this->fptr,"%s (%d:%s)\n",str,errno,strerror(errno));
    }
    fflush(stderr);
}

int start_fileout(char *filename,int *fd)
{
    FILE *logfile;
    int rval=0;
    if(filename)
    {
        logfile=fopen(filename,"w");
        if(!logfile)
        {
            NSNpanicPrint("Failed to open logfile '%s'\n",filename);
            rval=-1;
        }
        else
        {
            /* Enable file printing */

            G_printdevices[1]=init_fileout(logfile,fd);
            if(!G_printdevices[1])
                rval=-1;
        }
    }
    return rval;
}

static printdev *init_fileout(FILE *fptr,int *fd)
{
    fileprintdev *_this;
    _this=malloc(sizeof(fileprintdev));
    if(!_this)
    {
        printf("malloc FAILED!\n");
        fclose(fptr);
        return NULL;
    }
    if(-1==(*fd=fileno(fptr)))
    {
        free(_this);
        printf("Bad output file stream!\n");
        return NULL;
    }
#if 0
    if(0>(flag=fcntl(fd, F_GETFD,0)))
    {
        NSNpanicPrint("FAILED to get log file's file flags!\n");
    }
    else
    {
        flag|=FD_CLOEXEC;

        if(fcntl(fd, F_SETFD, flag))
        {
            printf("Failed to set FD_CLOEXEC to log file!\n");
        }
    }
#endif
    _this->devtype=1;
    _this->prfunc=fileout_prfunc;
    _this->errprfunc=fileout_errprfunc;
    _this->fptr=fptr;
    _this->isopen=1;
    return (printdev *) _this;
}   

int start_stdout()
{
    if((G_printdevices[0]=init_stdout()))
        return 0;
    return -1;
}


static printdev *init_stdout()
{
    printdev *_this;
    _this=malloc(sizeof(printdev));
    if(!_this)
    {
        printf("malloc FAILED!\n");
        return NULL;
    }
    _this->devtype=0;
    _this->prfunc=stdout_prfunc;
    _this->errprfunc=stdout_errorprfunc;
    return _this;
}

void NSNprint(int verbosity,const char *fmt,...)
{
    int i;
    struct timespec times;
    char foo[1050];
    va_list args;
    size_t bar;
    unsigned secs;
    int nanosecs;
       
    if(clock_gettime(CLOCK_REALTIME,&times))
    {
        memset(&times,0,sizeof(times));
    }
    if(times.tv_nsec-G_starttime.tv_nsec<0)
    {
        times.tv_sec--;
        nanosecs=1000000000+times.tv_nsec-G_starttime.tv_nsec;
    }
    secs=times.tv_sec-G_starttime.tv_sec;

    snprintf(foo,100,"[%u:%d]\t",secs,nanosecs);
    bar=strlen(foo);

    if(verbosity>=G_verbositylimit)
    {
        va_start(args,fmt);
        vsnprintf(foo+bar,1050-bar,fmt,args);
        va_end(args);
        foo[1049]='\0';

        for(i=0;G_printdevices[i]&&i<MAX_SUPPORTED_PRINT_DEVICES;i++)
        {
            G_printdevices[i]->prfunc(G_printdevices[i],foo);
        } 
    }
}




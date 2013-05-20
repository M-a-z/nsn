
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



#ifndef NSN_PRINTFUNCS_H
#define NSN_PRINTFUNCS_H

#include <stdio.h>

#define MAX_SUPPORTED_PRINT_DEVICES 2


typedef struct printdev
{
    int devtype;
    void (*prfunc)(struct printdev*,const char *);
    void (*errprfunc)(struct printdev*,const char *);
}printdev;

typedef enum Everbosity
{
    BABBLER_VERBOSITY=0,
    NORMAL_VERBOSITY=5,
    QUIET_VERBOSITY=10,
    MAX_VERBOSITY=100
}Everbosity;

#define NSNdetailPrint(fmt,args...) NSNprint(0,fmt, ## args)
#define NSNnormalPrint(fmt,args...) NSNprint(5,fmt, ## args)
#define NSNpanicPrint(fmt,args...) NSNprint(11,fmt, ## args)


typedef struct fileprintdev
{
    int devtype;
    void (*prfunc)(printdev *,const char *);
    void (*errprfunc)(printdev *,const char *);
    FILE *fptr;
    int isopen;
}fileprintdev;

int start_stdout();
int start_fileout(char *filename,int *fds);
void NSNprint(int verbosity,const char *fmt,...);
int setverbosity(Everbosity verb);
int set_relative_logtimes();


#endif

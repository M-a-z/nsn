
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




#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>


//#include <net/if.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include <bits/sockaddr.h>
#include <asm/types.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdarg.h>
#include <getopt.h>
#include <linux/if.h>
#include "nsn_printfuncs.h"
#include "nsn_commandlauncher.h"

extern char *if_indextoname(unsigned ifindex, char *ifname);

static SNSNcommandlist G_ladd_cmds;
static SNSNcommandlist G_ldel_cmds;
static SNSNcommandlist G_aadd_cmds;
static SNSNcommandlist G_adel_cmds;
static SNSNcommandlist G_radd_cmds;
static SNSNcommandlist G_rdel_cmds;

static void run_command(char *interpreter, char *interpretparam,char *cmd)
{
    int forkval;
    //size_t arglen;
    char * argv[4]={NULL};
    //char *argstart;
    int i;
    if(!cmd)
        return;

    /* trim spaces and tabs from front of cmd */
    while(cmd && (*cmd==' ' || *cmd=='\t') && *cmd)
        cmd++;
    if(!cmd)
        return;

    argv[0]=interpreter;
    argv[1]=interpretparam;
    argv[2]=cmd;
    argv[3]=(char *)NULL;

    forkval=fork();
    if(!forkval)
    {
        /* child */
        printf("\n");
        printf("Child pid %d calling exec\n",getpid());
        printf("%s\n",interpreter);
        for(i=0;argv[i];i++)
            printf("param %d: '%s'\n",i,argv[i]);
        printf("\n");

        if(execvp(interpreter,argv))
        {
            NSNpanicPrint("EXEC FAILED!\n");
            exit(-1);
        }
    }
    else if(forkval<0)
    {
        NSNpanicPrint("Fork FAILED!");
    }
}
/*
Interface index for interface to
              which address was bound, address family like AF_INET or AF_INET6 and prefix lenght of address are given  as  parameters  to
              command.  Also the address is given as last parameter (if available) If address is not available, zero is passed instead of
              address.
*/
static void checkrun_addradd(struct nlmsghdr *netlinkreq)
{
    int len;
    struct rtattr *at;
    char runcmd[1000];
    char addr[100]={'0','\0'};
    SNSNcommandlist *cmdlist;

    if(RTM_NEWADDR==netlinkreq->nlmsg_type)
        cmdlist=&G_aadd_cmds;
    else if(RTM_DELADDR==netlinkreq->nlmsg_type)
        cmdlist=&G_adel_cmds;
    else
        return;

    for(at=RTM_RTA(( NLMSG_DATA(netlinkreq) )); RTA_OK(at,len);at=RTA_NEXT(at,len))
    {
        if
        ( 
            (
                IFA_ADDRESS== at->rta_type || IFA_LOCAL == at->rta_type
            ) 
            && 
            (
                (int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_family  == AF_INET ||
                (int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_family  == AF_INET6
            )
        )
        {
            if
            (
                !inet_ntop
                ( 
                    (int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_family, 
                    RTA_DATA(at),
                    addr, 
                    100
                )
            )
            {
                NSNpanicPrint("Invalid address!\n");
                addr[0]='0';
                addr[1]='\0';
            }
        }
    }
    for(cmdlist=cmdlist->next;cmdlist;cmdlist=cmdlist->next)
    {
        unsigned int len;
        if
        (
            1000 <=
            (
                len=snprintf
                (
                    runcmd,
                    1000,
                    /* family, prefixlen,address */
                    "%s %d %d %s",
                    cmdlist->command,
                    (int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_family,
                    (int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_prefixlen,
                    addr
                )
            )
        )
        {
            NSNpanicPrint("Failed to execute addr add/del command - cmd too long");
        }
        else
        {
            NSNdetailPrint("Executing addr add/del command '%s'",runcmd);
            run_command("/bin/sh","-c",runcmd);
        }
    }


}


static void checkrun_addrdel(struct nlmsghdr *netlinkreq)
{
//    if(RTM_DELADDR!=netlinkreq->nlmsg_type)
        return;

}

static void checkrun_routeadd(struct nlmsghdr *netlinkreq)
{
    int len;
    struct rtattr *at;
    char runcmd[1000];
    SNSNcommandlist *cmdlist;
    len=RTM_PAYLOAD(netlinkreq);
    char dsta[100];
    char srca[100];
    char gwa[100];
    char *src=NULL;
    char *dst=NULL;
    char *gw=NULL;

    if(RTM_NEWROUTE==netlinkreq->nlmsg_type)
        cmdlist=&G_radd_cmds;
    else if(RTM_DELROUTE==netlinkreq->nlmsg_type)
        cmdlist=&G_rdel_cmds;
    else
        return;


    /*
     * Address family, destination (if
     *               available), destination lenght, routing table, route type, gate‐
     *                             way  address  and source address are given as parameters to com‐
     *                                           mand being executed. If destination is not given  (like  may  be
     *                                                         for default gateway), zero is given as destination. Similarly if
     *                                                                       gateway or source addresses are not given,  zero  is  passed  as
     *                                                                                     missing  address  to  program being called.
     */
    /*
            if(netlinkreq->nlmsg_len>=sizeof(struct nlmsghdr)+sizeof(struct rtmsg))
                NSNdetailPrint
                (
                    
                    "family %u, dstlen %u, srclen %u, tos %u, table %u, proto %u, scope %u, type %u, flags %u",
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_family,
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_dst_len,
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_src_len,
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_tos,
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_table,
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_protocol,
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_scope,
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_type,
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_flags
                );

     */

    for(at=RTM_RTA(( NLMSG_DATA(netlinkreq) )); RTA_OK(at,len);at=RTA_NEXT(at,len))
    {
        char **dataptr=NULL;
        char **datastore;
        switch(at->rta_type)
        {
            case RTA_DST:
                 dataptr=&dst;
                 datastore=(char **)&dsta;
                 break;
            case RTA_PREFSRC:
                 dataptr=&src;
                 datastore=(char **)&srca;
                 break;
            case RTA_GATEWAY:
                 dataptr=&gw;
                 datastore=(char **)&gwa;
                 break;
        }
        if(dataptr && *dataptr)
        {
            if
            (
                inet_ntop
                (
                    (int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_family, 
                    RTA_DATA(at),
                    *datastore, 
                    100
                )
            )
                *dataptr=*datastore;
            dataptr=NULL;
        }
    }

    for(cmdlist=cmdlist->next;cmdlist;cmdlist=cmdlist->next)
    {
        unsigned int len;
        if
        (
            1000 <=
            (
                len=snprintf
                (
                    runcmd,
                    1000,
                    /* cmd,family, dest/preflen,rt_table,rt_type,gw,src */
                    "%s %d %s/%d %d %d %s %s",
                    cmdlist->command,
                    (int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_family,
                    dst?dst:"0",
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_dst_len,
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_table,
                    (unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_type,
                    gw?gw:"0",
                    src?src:"0"
                )
            )
        )
        {
            NSNpanicPrint("Failed to execute rt add/del command - cmd too long");
        }
        else
        {
            NSNdetailPrint("Executing rt add/del command '%s'",runcmd);
            run_command("/bin/sh","-c",runcmd);
        }
    }
}

static void checkrun_routedel(struct nlmsghdr *netlinkreq)
{
    //if(RTM_DELROUTE!=netlinkreq->nlmsg_type)
        return;

}

static void checkrun_linkadd(struct nlmsghdr *netlinkreq)
{
    int len;
    struct rtattr *at;
    char runcmd[1000];
    char ifname[IFNAMSIZ]={' ','\0'};
    /* Assume link add */
    SNSNcommandlist *cmdlist=&G_ladd_cmds;
    len=RTM_PAYLOAD(netlinkreq);

    /* 
     * It seems that ifi_change flag is implemented for RTM_NEWINK to indicate state changes!!!!!
     * So we can simply check if ifi_change says that RUNNING state was changed for the link =)
     */

    if
    (
        RTM_NEWLINK!=netlinkreq->nlmsg_type || 
        IFF_UP!=(IFF_UP&(unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_change) || 
        0xFFFFFFFF == (unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_change 
    ) 
        return;
    if
    ( 
        IFF_UP!=(IFF_UP&(unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_flags)
    )
        cmdlist=&G_ldel_cmds;
    /* Let's loop command and fork processes */
    for(at=RTM_RTA(( NLMSG_DATA(netlinkreq) )); RTA_OK(at,len);at=RTA_NEXT(at,len))
    {
        if( IFLA_IFNAME== at->rta_type)
        {
            strncpy(ifname,(char *)RTA_DATA(at),IFNAMSIZ);
            ifname[IFNAMSIZ-1]='\0';
        }
    }
    for(cmdlist=cmdlist->next;cmdlist;cmdlist=cmdlist->next)
    {
        unsigned int len;
        if
        (
            1000 <=
            (
                len=snprintf
                (
                    runcmd,
                    1000,
                    "%s %d %d %s",
                    cmdlist->command,
                    (int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_index,
                    (int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_family,
                    ifname
                )
            )
        )
        {
            NSNpanicPrint("Failed to execute link add command - cmd too long");
        }
        else
        {
            NSNdetailPrint("Executing link add command '%s'",runcmd);
            run_command("/bin/sh","-c",runcmd);
        }
    }

}

void commandprocesses(int commandsinstalled,struct nlmsghdr *nlmhdr)
{
    void (*checkerrunners[NSN_cmd_order_last])(struct nlmsghdr *);

    int shifter;

    checkerrunners[NSN_cmd_order_adda]=&checkrun_addradd;
    checkerrunners[NSN_cmd_order_addd]=&checkrun_addrdel;
    checkerrunners[NSN_cmd_order_rta]=&checkrun_routeadd;
    checkerrunners[NSN_cmd_order_rtd]=&checkrun_routedel;
    checkerrunners[NSN_cmd_order_lina]=&checkrun_linkadd;
    /* actually, we probably only need one checkrun for links... Looks like also if down is notified via NEWLINK msg! */
    //checkerrunners[NSN_cmd_order_lind]=&checkrun_linkdel;

    if(!commandsinstalled)
        return;
    for(shifter=0;NSN_cmd_order_last>shifter;shifter++)
    {
        if(commandsinstalled&(1<<shifter))
        {
            checkerrunners[shifter](nlmhdr);
        }
    }
}
int init_childhandling(int fdamnt,int *fds)
{
    struct sigaction sa;
    int flag;
    int i;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler=NULL;
    sa.sa_flags=SA_NOCLDWAIT;
    sigaction(SIGCHLD,&sa,NULL);

    for(i=0;i<fdamnt;i++)
    {
        if(0>(flag=fcntl(fds[i], F_GETFD,0)))
        {           
            NSNpanicPrint("FAILED to get file (%d) flags!",fds[i]);
            return -1;
        }           
        flag|=FD_CLOEXEC;
        if(fcntl(fds[i], F_SETFD, flag))
        {       
            NSNpanicPrint("Failed to set FD_CLOEXEC on file %d",fds[i]);
            return -1;
        }
    }
    return 0;
}

void init_cmdlists()
{
    memset(&G_ladd_cmds,0,sizeof(SNSNcommandlist));
    G_ladd_cmds.head=&G_ladd_cmds;
    memset(&G_ldel_cmds,0,sizeof(SNSNcommandlist));
    G_ldel_cmds.head=&G_ldel_cmds;

    memset(&G_radd_cmds,0,sizeof(SNSNcommandlist));
    G_radd_cmds.head=&G_radd_cmds;
    memset(&G_rdel_cmds,0,sizeof(SNSNcommandlist));
    G_rdel_cmds.head=&G_rdel_cmds;

    memset(&G_aadd_cmds,0,sizeof(SNSNcommandlist));
    G_aadd_cmds.head=&G_aadd_cmds;
    memset(&G_adel_cmds,0,sizeof(SNSNcommandlist));
    G_adel_cmds.head=&G_adel_cmds;
}

static int add_cmd(SNSNcommandlist *head,char *cmd)
{
    SNSNcommandlist *newcmd;
    if(!cmd || !head)
    {
        NSNpanicPrint("NULL routeadd_command");
        return -1;
    }
    newcmd=calloc(1,sizeof(SNSNcommandlist));
    if(!newcmd)
    {
        NSNpanicPrint("calloc FAILED!");
        return -1;
    }
    if((newcmd->next=head->next))
    {
        newcmd->next->prev=newcmd;
    }
    head->next=newcmd;
    newcmd->prev=head;
    newcmd->command=cmd;
    return 0;
}

int install_routeadd_command(char *cmd)
{
    return add_cmd(&G_radd_cmds,cmd);
}
int install_routedel_command(char *cmd)
{
    return add_cmd(&G_rdel_cmds,cmd);
}
int install_linkadd_command(char *cmd)
{
    return add_cmd(&G_ladd_cmds,cmd);
}
int install_linkdel_command(char *cmd)
{
    return add_cmd(&G_ldel_cmds,cmd);
}
int install_addradd_command(char *cmd)
{
    return add_cmd(&G_aadd_cmds,cmd);
}
int install_addrdel_command(char *cmd)
{
    return add_cmd(&G_adel_cmds,cmd);
}



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
#include "nsn_printfuncs.h"
#include "nsn_commandlauncher.h"

#define VERSION "0.3 - Rc"
#define MAINTAINERMAIL "Mazziesaccount@gmail.com"

extern char *if_indextoname(unsigned ifindex, char *ifname);

#define DEFAULT_MULTICAST_GRPS ( RTMGRP_LINK | RTMGRP_IPV4_IFADDR )
/*| RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE | RTMGRP_IPV6_IFINFO | RTNLGRP_IPV6_RULE | RTMGRP_NEIGH */

#define OPTSTRING "tbh?vVf:s:r:R:l:L:a:A:"
#define HELP_PRINT \
"Supported args:\n"\
"    -f --file       <filename> log in file\n" \
"    -t --relative   relative times in log\n" \
"    -b --background backgound\n" \
"    -v --version    print version\n" \
"    -V --verbose    Verbose - print details of netlink msgs\n" \
"    -q --quiet      Quiet - don't say a word\n" \
"    -s --events     <monitored events>\n\n" \
"       supported events are:\n" \
"       route4 - IPv4 routes\n" \
"       route6 - IPv6 routes\n" \
"       addr6  - IPv6 addres\n" \
"       link6  - IPv6 links\n" \
"       rule6  - IPv6 rules\n" \
"       neigh  - neighbour cache\n" \
"       default is link up/down and IPv4 address\n\n" \
"    -l --link-down-command     <command>\n" \
"    -L --link-up-command       <command>\n" \
"    -a --addr-removed-command  <command>\n" \
"    -A --addr-added-command    <command>\n" \
"    -r --route-removed-command <command>\n" \
"    -R --route-added-command   <command>\n" \
"These specify commands to run when link/addr/route state changes." \
"This allows build scripts to act when state of certain link/route/address changes\n" \
"nsn adds parameters specifying link/addr/route at the end of the command\n" \
"see man pages for details of params.\n\n" \
"    -h --help       to get this help\n" \
"    -? --help       to get this help\n\n" 

static struct option long_options[] =
{
    {"file" , required_argument, 0, 'f'},
    {"timing relative", no_argument, 0, 't'},
    {"backgound",  no_argument, 0, 'b'},
    {"version",  no_argument, 0, 'v'},
    {"verbose",  no_argument, 0, 'V'},
    {"quiet",  no_argument, 0, 'q'},
    {"events", required_argument, 0, 's'},
    {"help",  no_argument, 0, 'h'},
    {"link-down-command", required_argument, 0, 'l'},
    {"link-up-command", required_argument, 0, 'L'},
    {"addr-removed-command", required_argument, 0, 'a'},
    {"addr-added-command", required_argument, 0, 'A'},
    {"route-removed-command", required_argument, 0, 'a'},
    {"route-added-command", required_argument, 0, 'A'},

    {0,0,0,0}
};


/* verbosity>=G_verbositylimit */



const char *G_exename="netPoller";

const char *UNKNOWN="unknown";

static int rtmtxtInited=0;
const char *rtmtxt[RTM_MAX];

typedef struct SNetworkPollerParams
{
    int multicastgrps;
    int commandsinstalled;
    int fdamnt;
    int fds[10];
}SNetworkPollerParams;

void fill_rtmtxt()
{
    int i;
    for(i=0;i<RTM_MAX;i++)
        rtmtxt[i]=UNKNOWN;
    rtmtxt[NLMSG_ERROR] = "NLMSG_ERROR";
    rtmtxt[NLMSG_DONE] = "NLMSG_DONE";
#ifdef RTM_NEWROUTE
    rtmtxt[RTM_NEWROUTE]= "RTM_NEWROUTE";
#endif
#ifdef RTM_DELROUTE
    rtmtxt[RTM_DELROUTE] = "RTM_DELROUTE";
#endif
#ifdef RTM_GETROUTE
    rtmtxt[RTM_GETROUTE] = "RTM_GETROUTE";
#endif
#ifdef RTM_NEWADDR
    rtmtxt[RTM_NEWADDR] = "RTM_NEWADDR";
#endif
#ifdef RTM_DELADDR
    rtmtxt[RTM_DELADDR] = "RTM_DELADDR";
#endif
#ifdef RTM_GETADDR
    rtmtxt[RTM_GETADDR] = "RTM_GETADDR";
#endif
#ifdef RTM_NEWLINK
    rtmtxt[RTM_NEWLINK] = "RTM_NEWLINK";
#endif
#ifdef RTM_GETLINK
    rtmtxt[RTM_GETLINK] = "RTM_GETLINK";
#endif
#ifdef RTM_DELLINK
    rtmtxt[RTM_DELLINK] = "RTM_DELLINK";
#endif
#ifdef RTM_SETLINK
    rtmtxt[RTM_SETLINK] = "RTM_SETLINK";
#endif
#ifdef RTM_NEWNEIGH
    rtmtxt[RTM_NEWNEIGH] = "RTM_NEWNEIGH";
#endif
#ifdef RTM_DELNEIGH
    rtmtxt[RTM_DELNEIGH] = "RTM_DELNEIGH";
#endif
#ifdef RTM_GETNEIGH
    rtmtxt[RTM_GETNEIGH] = "RTM_GETNEIGH";
#endif
#ifdef RTM_NEWRULE
    rtmtxt[RTM_NEWRULE] = "RTM_NEWRULE";
#endif
#ifdef RTM_DELRULE
    rtmtxt[RTM_DELRULE] = "RTM_DELRULE";
#endif
#ifdef RTM_GETRULE
    rtmtxt[RTM_GETRULE] = "RTM_GETRULE";
#endif
#ifdef RTM_NEWQDISC
    rtmtxt[RTM_NEWQDISC] = "RTM_NEWQDISC";
#endif
#ifdef RTM_DELQDISC
    rtmtxt[RTM_DELQDISC] = "RTM_DELQDISC";
#endif
#ifdef RTM_GETQDISC
    rtmtxt[RTM_GETQDISC] = "RTM_GETQDISC";
#endif
#ifdef RTM_NEWTCLASS
    rtmtxt[RTM_NEWTCLASS] = "RTM_NEWTCLASS";
#endif
#ifdef RTM_GETTCLASS
    rtmtxt[RTM_GETTCLASS] = "RTM_GETTCLASS";
#endif
#ifdef RTM_DELTCLASS
    rtmtxt[RTM_DELTCLASS] = "RTM_DELTCLASS";
#endif
#ifdef RTM_NEWTFILTER
    rtmtxt[RTM_NEWTFILTER] = "RTM_NEWTFILTER";
#endif
#ifdef RTM_GETTFILTER
    rtmtxt[RTM_GETTFILTER] = "RTM_GETTFILTER";
#endif
#ifdef RTM_DELTFILTER
    rtmtxt[RTM_DELTFILTER] = "RTM_DELTFILTER";
#endif
#ifdef RTM_NEWACTION
    rtmtxt[RTM_NEWACTION] = "RTM_NEWACTION";
#endif
#ifdef RTM_GETACTION
    rtmtxt[RTM_GETACTION] = "RTM_GETACTION";
#endif
#ifdef RTM_DELACTION
    rtmtxt[RTM_DELACTION] = "RTM_DELACTION";
#endif
#ifdef RTM_NEWADDRLABEL
    rtmtxt[RTM_NEWADDRLABEL] = "RTM_NEWADDRLABEL";
#endif
#ifdef RTM_GETADDRLABEL
    rtmtxt[RTM_GETADDRLABEL] = "RTM_GETADDRLABEL";
#endif
#ifdef RTM_DELADDRLABEL
    rtmtxt[RTM_DELADDRLABEL] = "RTM_DELADDRLABEL";
#endif
#ifdef RTM_SETDCB
    rtmtxt[RTM_SETDCB] = "RTM_SETDCB";
#endif
#ifdef RTM_GETDCB
    rtmtxt[RTM_GETDCB] = "RTM_GETDCB";
#endif
#ifdef RTM_NEWNDUSEROPT
    rtmtxt[RTM_NEWNDUSEROPT] = "RTM_NEWNDUSEROPT";
#endif
#ifdef RTM_SETNEIGHTBL
    rtmtxt[RTM_SETNEIGHTBL] = "RTM_SETNEIGHTBL";
#endif
#ifdef RTM_GETNEIGHTBL
    rtmtxt[RTM_GETNEIGHTBL] = "RTM_GETNEIGHTBL";
#endif
#ifdef RTM_GETANYCAST
    rtmtxt[RTM_GETANYCAST] = "RTM_GETANYCAST";
#endif
#ifdef RTM_GETMULTICAST
    rtmtxt[RTM_GETMULTICAST] = "RTM_GETMULTICAST";
#endif
#ifdef RTM_NEWPREFIX
    rtmtxt[RTM_NEWPREFIX] = "RTM_NEWPREFIX";
#endif
    rtmtxtInited=1;
}

static void print_usage()
{
    printf("\nUsage:\n\n\n");
    printf("%s [options]\n\n",G_exename);
    printf(HELP_PRINT);
    printf("%s is a network monitoring tool. It runs at background and monitors network state.\n",G_exename);
}

#define NLM_TYPESTRING(nlmtype) \
    (\
        ( \
            (!rtmtxtInited) ? "---" : \
            ( \
                ((unsigned)(nlmtype) < RTM_MAX) ? rtmtxt[(unsigned)(nlmtype)] : "UNKNOWN" \
            ) \
        )  \
    )


static void print_if_del_attrs(struct nlmsghdr *netlinkreq)
{
    struct rtattr *at;
    int len=IFLA_PAYLOAD(netlinkreq);

    for(at=RTM_RTA(( NLMSG_DATA(netlinkreq) )); RTA_OK(at,len);at=RTA_NEXT(at,len))
    {
        if( IFLA_IFNAME==at->rta_type)
            NSNnormalPrint("name %s",(char *)RTA_DATA(at));
    }

}
static void  print_if_attrs(struct nlmsghdr *netlinkreq)
{
    struct rtattr *at;
//    int len2;
    int len=IFLA_PAYLOAD(netlinkreq);
    char ifname[IFNAMSIZ]={'\0'};
    int mtu=-1;
    int operstat=-1;
    char linkprint[1000];
    
    for(at=RTM_RTA(( NLMSG_DATA(netlinkreq) )); RTA_OK(at,len);at=RTA_NEXT(at,len))
    {
        char tmp[100];
        switch(at->rta_type)
        {
            case IFLA_IFNAME:
                strncpy(ifname,(char *) RTA_DATA(at),IFNAMSIZ);
                ifname[IFNAMSIZ-1]='\0';
                NSNdetailPrint("ifname set to %s",ifname);
                break;
            case IFLA_MTU:
                NSNdetailPrint
                (
                    "MTU set to %u",
                    (mtu=*(unsigned *) RTA_DATA(at))
                );
                break;
            case IFLA_ADDRESS:
                NSNdetailPrint
                (
                    "IFLA_ADDRESS is set to %s",
                    inet_ntop
                    ( 
                        (at->rta_len>8)?AF_INET6:AF_INET,
                        RTA_DATA(at),
                        tmp,
                        100
                    )
                );
                break;
            case IFLA_OPERSTATE:
                NSNdetailPrint
                    (
                     "IFLA_OPERSTATE is set to %d",(operstat=*(unsigned *) RTA_DATA(at))
                    );
                /*
            case IFA_LOCAL:
                NSNdetailPrint
                (
                    "IFA_LOCAL is set to %s",
                    inet_ntop
                    (
                        (at->rta_len>8)?AF_INET6:AF_INET,
                        RTA_DATA(at),
                        tmp,
                        100
                    )
                );
                break;
                */
            case IFLA_BROADCAST:
                NSNdetailPrint
                (   
                    "IFLA_BROADCAST is set to %s",
                    inet_ntop
                    (
                        (at->rta_len>8)?AF_INET6:AF_INET,
                        RTA_DATA(at),
                        tmp,
                        100
                    )
                );
                break;
                /*
            case IFA_LABEL:
                NSNdetailPrint
                (
                    "IFA_LABEL is set to '%s'",
                    (char *)RTA_DATA(at)
                );
                break;
            case IFA_ANYCAST:
                NSNdetailPrint
                (   
                    "IFA_ANYCAST is set to %s",
                    inet_ntop
                    (
                        (at->rta_len>8)?AF_INET6:AF_INET,
                        RTA_DATA(at),
                        tmp,
                        100
                    )
                );
                break;
                */
            default:
                NSNdetailPrint
                (
                    "attr %hu, len %hu, (unsigned int)0x%x",
                    at->rta_type,
                    at->rta_len,
                    (len-(int)sizeof(struct rtattr)>=(int)sizeof(unsigned int))?*(unsigned int *)RTA_DATA(at):
                        (len-(int)sizeof(struct rtattr)>=(int)sizeof(short))?(unsigned int)*(unsigned short *)RTA_DATA(at):
                            ((int)sizeof(struct rtattr)<len)?(int)*(unsigned char *)RTA_DATA(at):0
                );
            break;
        }//switch ends
    }//for each attr
    if(operstat!=-1)
    {
        snprintf(linkprint,1000,"operating state=%d",operstat);
        linkprint[999]='\0';
    }
    if(-1!=mtu)
    {
        len=strlen(linkprint);
        snprintf(&(linkprint[len]),1000-len," mtu=%u",mtu);
        linkprint[999]='\0';
    }
}

static void  print_addr_attrs(struct nlmsghdr *netlinkreq)
{
    struct rtattr *at;
    int len=IFA_PAYLOAD(netlinkreq);
    char ifaceaddr[100]={'\0'};
    char bcastaddr[100]={'\0'};
    char addrprint[1000]={'\0'};
    

    
    for(at=RTM_RTA(( NLMSG_DATA(netlinkreq) )); RTA_OK(at,len);at=RTA_NEXT(at,len))
    {
         char tmp[100];
         char *label[__IFA_MAX+1]={[IFA_ADDRESS]="IFA_ADDRESS",[IFA_LOCAL]="IFA_LOCAL",[IFA_BROADCAST]="IFA_BROADCAST",[IFA_ANYCAST]="IFA_ANYCAST",[IFA_MULTICAST]="IFA_MULTICAST"};
        switch(at->rta_type)
        {
         /*
          *	IFA_UNSPEC,
	IFA_ADDRESS,
	IFA_LOCAL,
	IFA_LABEL,
	IFA_BROADCAST,
	IFA_ANYCAST,
	IFA_CACHEINFO,
	IFA_MULTICAST,
	__IFA_MAX,

          * */
//	IFA_CACHEINFO,

	        case IFA_LABEL:
                NSNdetailPrint
                (
                    "IFA_LABEL: \"%s\"",
                    (char *)RTA_DATA(at)
                );
                break;
	        case IFA_BROADCAST:
	        case IFA_MULTICAST:
	        case IFA_ANYCAST:
            case IFA_LOCAL:
            case IFA_ADDRESS:
                NSNdetailPrint
                (
                    "%s %s",
                    (NULL!=label[at->rta_type])?label[at->rta_type]:"Unknown",
                    inet_ntop
                    ( 
                        (at->rta_len>8)?AF_INET6:AF_INET,
                        RTA_DATA(at),
                        tmp,
                        100
                    )
                );
                if(IFA_LOCAL == at->rta_type || IFA_ADDRESS == at->rta_type)
                    memcpy(ifaceaddr,tmp,100);
                else if(IFA_BROADCAST == at->rta_type )
                    memcpy(bcastaddr,tmp,100);
                break;
            default:
                NSNdetailPrint
                (
                    "attr %hu, len %hu, (unsigned int)0x%x",
                    at->rta_type,
                    at->rta_len,
                    (len-(int)sizeof(struct rtattr)>=(int)sizeof(unsigned int))?*(unsigned int *)RTA_DATA(at):
                        (len-(int)sizeof(struct rtattr)>=(int)sizeof(short))?(unsigned int)*(unsigned short *)RTA_DATA(at):
                            ((int)sizeof(struct rtattr)<len)?(int)*(unsigned char *)RTA_DATA(at):0
                );
                break;
        }
    }
    if('\0'!=ifaceaddr[0])
    {
        snprintf(addrprint,1000,"addr %s/%u",ifaceaddr,(unsigned int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_prefixlen);
        addrprint[999]='\0';
    }
    if('\0'!=bcastaddr[0])
    {
        len=strlen(addrprint);
        snprintf(&(addrprint[len]),1000-len,"\tbroadcast %s",bcastaddr);
    }
    addrprint[999]='\0';
    NSNnormalPrint(addrprint);
}
void print_attr(char *attrtext,short type, short len, void *data)
{
    short datalen=len-sizeof(struct rtattr);
    int wordamnt;
    if(!attrtext)
        NSNdetailPrint("attrtype=%hd, datalen=%hd:",type,datalen);
    else
        NSNdetailPrint("attrtype=%hd (%s), datalen=%hd:",type,attrtext,datalen);
    switch(datalen)
    {
        case 1:
            NSNdetailPrint("data =0x%x",(unsigned)*(unsigned char*)data);
            break;
        case 2:
            NSNdetailPrint("data =0x%hx",*(unsigned short *)data);
        case 3:
            NSNdetailPrint("data =0x%02x 0x%02x 0x%02x",((unsigned char *)data)[0],((unsigned char *)data)[1],((unsigned char *)data)[2]);
            break;
        case 4:
            NSNdetailPrint("data =0x%x",*(unsigned *)data);
            break;
        case 5:
            NSNdetailPrint("data =0x08%x 0x%02x",*(unsigned *)data,(unsigned)((unsigned char *)data)[4]);
            break;
        case 6:
            NSNdetailPrint("data =0x08%x 0x%04hx",*(unsigned *)data,((unsigned short *)data)[2]);
            break;
        case 7:
            NSNdetailPrint("data =0x08%x 0x%04hx 0x%02x",*(unsigned *)data,((unsigned short *)data)[2],(unsigned)((unsigned char *)data)[6]);
            break;
        case 8:
            NSNdetailPrint("data =0x%llx",*(unsigned long long *)data);
            break;
        default:
        {
            for(wordamnt=0;wordamnt<datalen/4;wordamnt++)
            {
                NSNdetailPrint("data[%d]=0x%x",wordamnt,((unsigned *)data)[wordamnt]);
            }
        }
    }
}
#if 0
Issue 1: We cannot do this because we do not know the family specific header size if msg type is unknown

static void print_gen_attrs(struct nlmsghdr *netlinkreq)
{
     struct rtattr *at;
    int len=RTM_PAYLOAD(netlinkreq);

    for(at=RTM_RTA(( NLMSG_DATA(netlinkreq) )); RTA_OK(at,len);at=RTA_NEXT(at,len))
    {
        print_attr(NULL,at->rta_type,at->rta_len,RTA_DATA(at));
    }
}
#endif
#define NEIGH_PAYLOAD(n) NLMSG_PAYLOAD((n),sizeof(struct ndmsg))
static void  print_neigh_attrs(struct nlmsghdr *netlinkreq)
{
    struct rtattr *at;
    int len=NEIGH_PAYLOAD(netlinkreq);

    for(at=RTM_RTA(( NLMSG_DATA(netlinkreq) )); RTA_OK(at,len);at=RTA_NEXT(at,len))
    {
        switch(at->rta_type)
        {
            case NDA_DST:
                print_attr("NDA_DST - n/w layer dst addr",at->rta_type,at->rta_len,RTA_DATA(at));
            break;
            case NDA_LLADDR:
                print_attr("NDA_LLADDR - link layer addr",at->rta_type,at->rta_len,RTA_DATA(at));
            break;
            case NDA_CACHEINFO:
                print_attr("NDA_CACHEINFO - cache stats",at->rta_type,at->rta_len,RTA_DATA(at));
            break;
            default:
            break;
        }
    }
}
static void  print_rt_attrs(struct nlmsghdr *netlinkreq)
{
    struct rtattr *at;
    int len=RTM_PAYLOAD(netlinkreq);
    char routedetails[1000]={0};
    char dst[100]={'\0'};
    char src[100]={'\0'};
    char gw[100]={'\0'};
    int oif=-1;
    int prio=-1;
    int mtu=-1;
    
    for(at=RTM_RTA(( NLMSG_DATA(netlinkreq) )); RTA_OK(at,len);at=RTA_NEXT(at,len))
    {
        switch(at->rta_type)
        {
            case RTA_DST:
                NSNdetailPrint
                (
                    
                    "dst is set to %s",
                    inet_ntop
                    ( 
                        (at->rta_len>8)?AF_INET6:AF_INET,
                        RTA_DATA(at),
                        dst,
                        100
                    )
                );
                break;
            case RTA_SRC:
            {
                char tmp[100];
                NSNdetailPrint
                (
                    "src (key) addr set to %s",
                    inet_ntop
                    (
                        (at->rta_len>8)?AF_INET6:AF_INET,
                        RTA_DATA(at),
                        tmp,
                        100
                    )
                );
            }
            case RTA_PREFSRC:
                NSNdetailPrint
                (
                    "(pref)src addr set to %s",
                    inet_ntop
                    (
                        (at->rta_len>8)?AF_INET6:AF_INET,
                        RTA_DATA(at),
                        src,
                        100
                    )
                );
                break;
            case RTA_GATEWAY:
                NSNdetailPrint
                (   
                    
                    "gw is set to %s",
                    inet_ntop
                    (
                        (at->rta_len>8)?AF_INET6:AF_INET,
                        RTA_DATA(at),
                        gw,
                        100
                    )
                );
                break;
            case RTA_OIF:
                NSNdetailPrint
                (
                    
                    "OIF is set to %u",
                    (oif=*(unsigned int *)RTA_DATA(at))
                );
                break;
            case RTA_METRICS:
                {
                    struct rtattr *subattr;
                    int metlen=at->rta_len;
                    NSNdetailPrint("found RTA_METRICS, parsing...");
                    for(subattr=(struct rtattr *)RTA_DATA(at); RTA_OK(subattr,metlen);subattr=RTA_NEXT(subattr,metlen))
                    {
                        if(subattr->rta_type==RTAX_MTU)
                        {
                            NSNdetailPrint
                            (   
                                "MTU is set to %u",
                                (mtu=*(unsigned int *)RTA_DATA(subattr))
                            );
                        }
                        else
                            NSNdetailPrint
                            (
                                "RTM METRICS contained attr %hu, len %hu, (unsigned int)0x%x",
                                subattr->rta_type,
                                subattr->rta_len,
                                (metlen-(int)sizeof(struct rtattr)>=(int)sizeof(unsigned int))?*(unsigned int *)RTA_DATA(subattr):
                                    (metlen-(int)sizeof(struct rtattr)>=(int)sizeof(short))?(unsigned int)*(unsigned short *)RTA_DATA(subattr):
                                        ((int)sizeof(struct rtattr)<metlen)?(int)*(unsigned char *)RTA_DATA(subattr):0
                            );

                    }

                }
                break;
            case RTA_PRIORITY:
                NSNdetailPrint
                (   
                    
                    "Priority is set to %u",
                    (prio=*(unsigned int *)RTA_DATA(at))
                );
                break;
            default:
                NSNdetailPrint
                (
                    
                    "attr %hu, len %hu, (unsigned int)0x%x",
                    at->rta_type,
                    at->rta_len,
                    (len-(int)sizeof(struct rtattr)>=(int)sizeof(unsigned int))?*(unsigned int *)RTA_DATA(at):
                        (len-(int)sizeof(struct rtattr)>=(int)sizeof(short))?(unsigned int)*(unsigned short *)RTA_DATA(at):
                            ((int)sizeof(struct rtattr)<len)?(int)*(unsigned char *)RTA_DATA(at):0
                );
                break;

        }
    }
    if((int)dst[0])
    {
        len=strlen(dst);
        if(len>100)
        {
            NSNpanicPrint("Internal error at %s:%d",__FILE__,__LINE__);
            exit(1);
        }
        snprintf(routedetails,1000,"dst: %s/%u",dst,(unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_dst_len);
        routedetails[999]='\0';
    }
    if((int)src[0])
    {
        size_t len2;
        len2=strlen(routedetails);
        len=strlen(src);
        if(len>100)
        {
            NSNpanicPrint("Internal error at %s:%d",__FILE__,__LINE__);
            exit(1);
        }
        snprintf(&(routedetails[len2]),1000-len2,"\tsrc: %s",src);
        routedetails[999]='\0';
    }
    if((int)gw[0])
    {
        size_t len2;
        len2=strlen(routedetails);
        len=strlen(gw);
        if(len>100)
        {
            NSNpanicPrint("Internal error at %s:%d",__FILE__,__LINE__);
            exit(1);
        }
        snprintf(&(routedetails[len2]),1000-len2,"\tgw: %s",gw);
        routedetails[999]='\0';
    }
    if(oif!=-1)
    {
        char ifname[IFNAMSIZ];
        size_t len2;
        if(!if_indextoname(oif,ifname))
        {
            NSNpanicPrint("invalid interface in route!");
        }
        else
        {
            len=strlen(ifname);
            len2=strlen(routedetails);
            snprintf(&(routedetails[len2]),1000-len2,"\tdev %s",ifname);
        }
        routedetails[999]='\0';
    }
    if(mtu!=-1)
    {
        len=strlen(routedetails);
        snprintf(&(routedetails[len]),1000-len,"\tMTU %u",(unsigned)mtu);
        routedetails[999]='\0';
    }
    if(prio!=-1)
    {
        len=strlen(routedetails);
        snprintf(&(routedetails[len]),1000-len,"\tMetric %u",(unsigned)prio);
        routedetails[999]='\0';
    }
    NSNnormalPrint(routedetails);
}

static void print_nl_msg(struct nlmsghdr *netlinkreq)
{
    void  (*print_attrs) (struct nlmsghdr *)= /* Issue 1: cant do this func &print_gen_attrs*/ NULL;
    char *actionstr=NULL;
    char *familystr=NULL;
    int getifname=1;

//    print_attrs = NULL;

    NSNdetailPrint
    (
        
        "NLMSG: len %u, type %s, flags %hu, seq %u pid %u",
        netlinkreq->nlmsg_len,
        (char *) NLM_TYPESTRING( (int)(netlinkreq->nlmsg_type) ),
        netlinkreq->nlmsg_flags,
        netlinkreq->nlmsg_seq,
        netlinkreq->nlmsg_pid
    );
    switch(netlinkreq->nlmsg_type)
    {
        case RTM_NEWROUTE:
            actionstr=((netlinkreq->nlmsg_flags&NLM_F_REPLACE)==NLM_F_REPLACE)?"route changed":"route added:";
        case RTM_DELROUTE:
            if(!actionstr)
                actionstr="route deleted:";
        case RTM_GETROUTE:
            if(!actionstr)
                actionstr="route configs read - I why do I get this msg?";
        print_attrs=&print_rt_attrs;
        case RTM_NEWRULE:
            if(!actionstr)
                actionstr="rule created:";
        case RTM_GETRULE:
            if(!actionstr)
                actionstr="rule requested - I why do I get this msg?";
        case RTM_DELRULE:
        {
            if(!actionstr)
                actionstr="rule deleted:";
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
                if((unsigned int)AF_INET==(unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_family)
                {
                    familystr="IPv4";
                }
                else if((unsigned int)AF_INET6==(unsigned int)((struct rtmsg *) NLMSG_DATA(netlinkreq) )->rtm_family)
                {
                    familystr="IPv6";
                }
                else
                {
                    familystr="Unknown family";
                }
                NSNnormalPrint("%s %s",familystr,actionstr);
        }
        break;
        case RTM_NEWADDR:
            if(!actionstr)
                actionstr="address assigned:";
        case RTM_DELADDR:
            if(!actionstr)
                actionstr="address deleted:";
        case RTM_GETADDR:
        {
            char ifname[IFNAMSIZ];

            if(!actionstr)
                actionstr="getaddress request received - why am I receiving this?";
#if 0
            struct ifaddrmsg
{
	__u8		ifa_family;
	__u8		ifa_prefixlen;	/* The prefix length		*/
	__u8		ifa_flags;	/* Flags			*/
	__u8		ifa_scope;	/* Address scope		*/
	__u32		ifa_index;	/* Link index			*/
};
#endif
            NSNdetailPrint
            (
                
                "ifa_family %u, ifa_prefixlen %u, ifa_flags %u, ifa_scope %u, ifa_index %u",
                (unsigned int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_family,
                (unsigned int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_prefixlen,
                (unsigned int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_flags,
                (unsigned int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_scope,
                (unsigned int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_index
            );
            if(AF_INET == (unsigned int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_family)
            {
                familystr="IPv4";
            }
            else if(AF_INET6 == (unsigned int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_family)
            {
                familystr="IPv6";
            }
            else
                familystr="Unknown protocol";

            if(!if_indextoname((unsigned int)((struct ifaddrmsg *) NLMSG_DATA(netlinkreq) )->ifa_index,ifname))
            {
                strcpy(ifname,"unknown");
            }
            NSNnormalPrint("%s %s dev %s",familystr,actionstr,ifname);
            print_attrs=&print_addr_attrs;
        }
        break;
        case RTM_DELLINK:
            getifname=0;
            if(!actionstr)
                actionstr="Interface deleted:";
        case RTM_GETLINK:
            if(!actionstr)
                actionstr="Interface get request received - Why do I receive this??";
                /* State changes are also informed via NEWLINK */
        case RTM_NEWLINK:
        /* Actually, should we only look for NEWLINK messages with ifi_change being zero? */
            if
            (
                !actionstr && 
                ( 0xFFFFFFFF== (unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_change) 
            )
                actionstr="New Interface created:";
        case RTM_SETLINK:
        {

            if(!actionstr )
                actionstr="Interface state changed:";

#if 0
struct ifinfomsg
{
	unsigned char	ifi_family;
	unsigned char	__ifi_pad;
	unsigned short	ifi_type;		/* ARPHRD_* */
	int		ifi_index;		/* Link index	*/
	unsigned	ifi_flags;		/* IFF_* flags	*/
	unsigned	ifi_change;		/* IFF_* change mask */
};


#endif
            NSNdetailPrint
            (
                
                "ifi_family %u, ifi_type %u, ifi_index %u, ifi_flags %u, ifi_change %u",
                (unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_family,
                (unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_type,
                (unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_index,
                (unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_flags,
                (unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_change
            );
            if(getifname)
            {
                char ifname[IFNAMSIZ];
                if(!if_indextoname((unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_index,ifname))
                {
                    NSNpanicPrint("Could not get name for interface %u",(unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_index);
                }
                else
                {
                    NSNnormalPrint
                    (
                        "%s name %s (%d), state %s, oper (running) state %s, driver state %s, 802.1q VLAN %s enabled",
                        actionstr,
                        ifname,
                        (unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_index,
                        ((((unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_flags)&IFF_UP) == IFF_UP)?"UP":"DOWN",
                        ((((unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_flags)&IFF_RUNNING) == IFF_RUNNING)?"RUNNING":"NOT OPERATIONAL",
                        ((((unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_flags)&IFF_LOWER_UP) == IFF_LOWER_UP)?"IFF_LOWER_UP":"LOWER_DOWN",
                        ((((unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_flags)&IFF_802_1Q_VLAN) == IFF_802_1Q_VLAN)?"is":"is not"
                    );
                    NSNnormalPrint
                    (
                        "State changes: UP state :%s, oper state %s, driver state %s, 802.1q VLAN state %s",
                        ((((unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_change)&IFF_UP) == IFF_UP)?"CHANGED":"NOT CHANGED",
                        ((((unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_change)&IFF_RUNNING) == IFF_RUNNING)?"CHANGED":"NOT CHANGED",
                        ((((unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_change)&IFF_LOWER_UP) == IFF_LOWER_UP)?"CHANGED":"NOT CHANGED",
                        ((((unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_change)&IFF_802_1Q_VLAN) == IFF_802_1Q_VLAN)?"CHANGED":"NOT CHANGED"
                    );

                }
                print_attrs=&print_if_attrs;

            }
            else
            {
                NSNnormalPrint("%s index %d",actionstr,(unsigned int)((struct ifinfomsg *) NLMSG_DATA(netlinkreq) )->ifi_index);
                print_attrs=&print_if_del_attrs;
                
            }
        }
        break;
        case NLMSG_ERROR:
#if 0
struct nlmsgerr
{
	int		error;
	struct nlmsghdr msg;
};
#endif
       NSNpanicPrint
            (
                "NETLINK error %d (%s)",
                ((struct nlmsgerr *) NLMSG_DATA(netlinkreq) )->error,
                strerror(-((struct nlmsgerr *) NLMSG_DATA(netlinkreq) )->error)
            );
        break;
        case RTM_NEWNEIGH:
        case RTM_DELNEIGH:
        case RTM_GETNEIGH:
#if 0
struct ndmsg {
    __u8        ndm_family;
    __u8        ndm_pad1;
    __u16       ndm_pad2;
    __s32       ndm_ifindex;
    __u16       ndm_state;
    __u8        ndm_flags;
    __u8        ndm_type;
};
#endif
        if(netlinkreq->nlmsg_len>=sizeof(struct nlmsghdr)+sizeof(struct ndmsg))
        {
            NSNdetailPrint
            (
                
               "ndm_family=%d,ndm_ifindex=%d,ndm_state=%d,ndm_flags=%d,ndm_type=%d",
               (unsigned int)((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_family,
               (unsigned int)((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_ifindex,
               (unsigned int)((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_state,
               (unsigned int)((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_flags,
               (unsigned int)((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_type
            );
            if((((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_state & NUD_INCOMPLETE) == NUD_INCOMPLETE)
                NSNdetailPrint("NUD_INCOMPLETE is SET - currently resolving cache entry");
            if((((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_state & NUD_REACHABLE) == NUD_REACHABLE)
                NSNdetailPrint("NUD_REACHABLE is SET - a confirmed working cache entry");
            if((((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_state & NUD_STALE) == NUD_STALE)
                NSNdetailPrint("NUD_STALE is SET - an expired cache entry");
            if((((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_state & NUD_DELAY) == NUD_DELAY)
                NSNdetailPrint("NUD_DELAY is SET - an entry waiting for a timer (don't ask...)");
            if((((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_state & NUD_PROBE) == NUD_PROBE)
                NSNdetailPrint("NUD_PROBE is SET - a cache entry which is currently reprobed");
            if((((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_state & NUD_FAILED) == NUD_FAILED)
                NSNdetailPrint("NUD_FAILED is SET - an invalid cache entry");
            if((((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_state & NUD_NOARP) == NUD_NOARP)
                NSNdetailPrint("NUD_NOARP is SET - a device with no destination cache");
            if((((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_state & NUD_PERMANENT) == NUD_PERMANENT)
                NSNdetailPrint("NUD_PERMANENT is SET - a static entry");
            if((((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_flags & NTF_PROXY) == NTF_PROXY)
                NSNdetailPrint("NTF_PROXY is SET - proxy arp entry");
            if((((struct ndmsg *) NLMSG_DATA(netlinkreq) )->ndm_flags & NTF_ROUTER) == NTF_ROUTER)
                NSNdetailPrint("NTF_ROUTER is SET - IPv6 router entry");

            print_attrs=&print_neigh_attrs;
        }
        else
            NSNdetailPrint
            ( 
                "RTM_*NEIGH message with unexpected size %u (expected at least %u)",
                netlinkreq->nlmsg_len,
                sizeof(struct nlmsghdr)+sizeof(struct ndmsg)
            );
        break;
        case NLMSG_DONE:
        break;
        default:
            break;
    }
    if(NULL!=print_attrs)
        (*print_attrs)(netlinkreq);
}


int NetworkPoller(SNetworkPollerParams *params)
{
    char buff[4096];

    int sock;
    struct sockaddr_nl addr;
    struct sockaddr_nl senderaddr;
    
    struct iovec iov;
    struct msghdr msg;
    int len;

    struct nlmsghdr *nlmhdr;

    addr.nl_family=AF_NETLINK;
    addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE | RTMGRP_IPV6_IFINFO | RTNLGRP_IPV6_RULE | RTMGRP_NEIGH;
    //addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;

    sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if(sock < 0)
    {
        NSNpanicPrint("FAILED TO START LISTENING INTERFACE NOTIFICATIONS!");
        return -1;
    }
    if(bind(sock,(struct sockaddr *) &addr,sizeof(struct sockaddr_nl)))
    {
        NSNpanicPrint("FAILED to bind netlink sock to groups RTMGRP_LINK and RTMGRP_IPV4_IFADDR");
        return -1;
    }
    if(params->commandsinstalled)
    {
        params->fds[params->fdamnt]=sock;
        params->fdamnt++;
        if(init_childhandling(params->fdamnt,params->fds))
        {
            NSNpanicPrint("Failed to init childprochandling!");
            return -1;
        }
    }
#if 0
    if(0>(flag=fcntl(sock, F_GETFD,0)))
    {
        NSNpanicPrint("FAILED to get netlink socket's file flags!\n");
        return -1;
    }
    flag|=FD_CLOEXEC;
    if(fcntl(sock, F_SETFD, flag))
    {
        NSNpanicPrint("Failed to set FD_CLOEXEC on netlink sock!");
        return -1;
    }
#endif
    memset(&msg,0,sizeof(msg));
    memset(&iov,0,sizeof(iov));

    iov.iov_base=buff;
    iov.iov_len=4096;


    msg.msg_name=(void *)&senderaddr;
    msg.msg_namelen=sizeof(senderaddr);
    msg.msg_iov=&iov;
    msg.msg_iovlen = 1; /* only one iov struct abowe */

    while(1)
    {
        len=recvmsg(sock, &msg, 0);
        for(nlmhdr= (struct nlmsghdr *)buff; NLMSG_OK(nlmhdr,len); nlmhdr=NLMSG_NEXT(nlmhdr,len))
        {
            if(nlmhdr->nlmsg_type== NLMSG_DONE)
            {
                NSNdetailPrint("Last part of multipart msg received!");
            }
            else if(nlmhdr->nlmsg_type == NLMSG_ERROR)
            {
                NSNpanicPrint
                (
                    "ERROR message received from netlink - errorcode %d",
                    ((struct nlmsgerr *) NLMSG_DATA(nlmhdr) )->error
                );
            }
            else
            {
                print_nl_msg(nlmhdr);
                commandprocesses(params->commandsinstalled,nlmhdr);
            }
        }
    }
    return 0;
}


int main(int argc, char *argv[])
{
    int rval;
    int index;
    int bg=0;
    SNetworkPollerParams params;
    char c;

    if(argv[0])
        G_exename=argv[0];
    if(start_stdout())
    {
        printf("Failed to initialize stdoutprinter!\n");
        return EXIT_FAILURE;
    }
    memset(&params,0,sizeof(params));
    init_cmdlists();
    fill_rtmtxt();
    
/*
 RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE | RTMGRP_IPV6_IFINFO | RTNLGRP_IPV6_RULE | RTMGRP_NEIGH
 */
    params.multicastgrps=0;
    params.multicastgrps|= DEFAULT_MULTICAST_GRPS;
//    while((char)-1 != (c = getopt(argc, argv, OPTSTRING)))
    index=0;
    while((char)-1 != (c = getopt_long(argc, argv, OPTSTRING,long_options,&index)))
    {
        switch(c)
        {
            case 'r':
                if(install_routedel_command(optarg))
                {
                    return EXIT_FAILURE;
                }
                params.commandsinstalled|=ROUTEDEL_CMD;
                break;
            case 'R':
                if(install_routeadd_command(optarg))
                {
                    return EXIT_FAILURE;
                }
                params.commandsinstalled|=ROUTEADD_CMD;
                break;
            case 'l':
                if(install_linkdel_command(optarg))
                {
                    return EXIT_FAILURE;
                }
                params.commandsinstalled|=LINKDEL_CMD;
                break;
            case 'L':
                if(install_linkadd_command(optarg))
                {
                    return EXIT_FAILURE;
                }
                params.commandsinstalled|=LINKADD_CMD;
                break;
            case 'a':
                if(install_addrdel_command(optarg))
                {
                    return EXIT_FAILURE;
                }
                params.commandsinstalled|=ADDRDEL_CMD;
                break;
            case 'A':
                if(install_addradd_command(optarg))
                {
                    return EXIT_FAILURE;
                }
                params.commandsinstalled|=ADDRADD_CMD;
                break;
            case 'V':
                setverbosity(BABBLER_VERBOSITY);
                break;
            case 'q':
                setverbosity(QUIET_VERBOSITY);
                break;
            case 't':
                    set_relative_logtimes();
                break;
            case 'v':
                NSNpanicPrint("%s version %s",argv[0],VERSION);
                NSNpanicPrint("Please send bugreports to %s\n",MAINTAINERMAIL);
                return EXIT_SUCCESS;
                break;
            case 's':
            {

                int len;
                if(!optarg)
                {
                    printf("-s without specifier!\n");
                    print_usage();
                    break;
                }
                len=strlen(optarg);
                /* Known optargs:
                 -s <monitored events>\n\n" \
                 "       supported events are:\n" \
                 "       route4 - IPv4 routes\n" \
                 "       route6 - IPv6 routes\n" \
                 "       addr6  - IPv6 addres\n" \
                 "       link6  - IPv6 links\n" \
                 "       rule6  - IPv6 rules\n" \
                 "       neigh  - n
                 */
                if(len<5 || len>6)
                    break;
                if(5==len)
                {
                    if((strcmp("addr6",optarg)))
                    {
                        if((strcmp("link6",optarg)))
                        {
                            if((strcmp("rule6",optarg)))
                            {
                                if((strcmp("neigh",optarg)))
                                {
                                    printf("Unknown event for monitoring '%s'\n",optarg);
                                    return EXIT_FAILURE;
                                }
                                else
                                    params.multicastgrps|=RTMGRP_NEIGH;
                            }
                            else
                                params.multicastgrps|=RTNLGRP_IPV6_RULE;
                        }
                        else
                            params.multicastgrps|=RTMGRP_IPV6_IFINFO;
                    }
                    else
                        params.multicastgrps|=RTMGRP_IPV6_IFADDR;
                }
                if(6==len)
                {
                    if(strcmp("route4",optarg))
                    {
                        if(strcmp("route6",optarg))
                        {
                            printf("Unknown event for monitoring '%s'\n",optarg);
                            return EXIT_FAILURE;
                        }
                        else
                            params.multicastgrps|=RTMGRP_IPV6_ROUTE;
                        
                    }
                    else
                        params.multicastgrps|=RTMGRP_IPV4_ROUTE;

                }
            }
                
                break;
            case '?':
            case 'h':
                NSNpanicPrint("%s version %s\n",argv[0],VERSION);
                print_usage();
                return EXIT_SUCCESS;
                break;
            case 'b':
                bg=1;
                break;
            case 'f':
                if(start_fileout(optarg,&params.fds[params.fdamnt]))
                    return EXIT_FAILURE;
                params.fdamnt++;
                break;
            default:
                break;
        }
    }

    NSNdetailPrint("%s pid %d started","nsn",(int)getpid());
    NSNdetailPrint("Please send bug reports to %s",MAINTAINERMAIL);

    if(bg)
        daemon(1,1);

    if((rval=NetworkPoller(&params)))
    {
        NSNpanicPrint("Poller Failed!\n");
        return -1;
    }
    return rval;
}



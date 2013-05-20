
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




#ifndef NSN_COMMANDLAUNCHER_H
#define NSN_COMMANDLAUNCHER_H

#include <linux/rtnetlink.h>

typedef const enum NSN_cmd_order
{
    NSN_cmd_order_rtd = 0,
    NSN_cmd_order_rta,
    NSN_cmd_order_addd,
    NSN_cmd_order_adda,
    NSN_cmd_order_lina,
    NSN_cmd_order_last
}NSN_cmd_order;

#define ROUTEDEL_CMD (1<<NSN_cmd_order_rtd)
#define ROUTEADD_CMD (1<<NSN_cmd_order_rta)
#define ADDRDEL_CMD (1<<NSN_cmd_order_addd)
#define ADDRADD_CMD (1<<NSN_cmd_order_adda)
#define LINKADD_CMD (1<<NSN_cmd_order_lina)
/* LINK_ADD and LINK_DEL are handled in same function, thus there should not be that many items in enum (or we jump out of jumptable) */
#define LINKDEL_CMD (1<<NSN_cmd_order_last)


typedef struct SNSNcommandlist
{
    struct SNSNcommandlist *head;
    struct SNSNcommandlist *next;
    struct SNSNcommandlist *prev;
    char *command;
}SNSNcommandlist;

void init_cmdlists();
int init_childhandling(int fdamnt,int *fds);
int install_routeadd_command(char *cmd);
int install_routedel_command(char *cmd);
int install_linkadd_command(char *cmd);
int install_linkdel_command(char *cmd);
int install_addradd_command(char *cmd);
int install_addrdel_command(char *cmd);
void commandprocesses(int commandsinstalled,struct nlmsghdr *nlmhdr);
#endif

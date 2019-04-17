# nsn(8) - Network Status Notifier

v0.2 - Fall of Rome, 04 June 2012

```
nsn  [ -tbvVqh?q ] [ .B-s  eventtype ] [ -f logfile ] [ -l --link-down-command command ] [ -L --link-up-command command ] [ -a --addr-removed-command command ] [ -A --addr-added-command command ] [ -r --route-removed-command command ] [ -R --route-added-command command ]
```



# Description

**nsn**(8) is a network status monitoring tool. It does listening changes in network configuration using netlink sockets, and prints informative messages to console or log file. **nsn**(8)**was**written**to**be**used**as**debug**tool**with**network**problems.**It**can**also**serve**as**simple**example**code**for**using**netlink**sockets**to**listen**network**changes.**
 

As of version 0.2 it supports executing command when route/address/interface state changes. Nsn uses 'de facto' **fork**(2) and **exec**(3) method to execute commands as new processes. See **-l -L -a -A -r -R** OPTIONS below for further details.


# Options


* **-s**  
  one of following event types we wish to monitor. It is possible to specify multiple events by specifying multiple 
  **-s**
  options
* **Supported suboptions for -s**  
  
  **route4**
  Listens IPv4 routing table changes
  
  **route6**
  Listens IPv6 routing table changes
  
  **addr6**
  Listens for IPv6 address changes
  
  **link6**
  Listens for changes in IPv6 interfaces
  
  **rule6**
  Listens for IPv6 rules
  
  **neigh**
  Listens for changes in neighbour cache (arp/ndp)
  
  By default changes in IPv4 routes and interfaces are listened.
  
  
* **-f**  
  followed by name of log file. Enables logging to file. 
* **-t**  
  .rB "Use timestamps relative to program startup. Note, version 0.1 used " "-r" " for this purpose."
* **-b**  
  Start at background
  
* **-l --link-down-command, -L --link-up-command**  
  **-l** and **-L** install commands to be executed when link flags **IFF_UP/IFF_RUNNING** changes. (see **&lt;linux/if.h&gt;**) When **nsn** calls command it passes interface index number **ifi_index**, interface address family (like **AF_INET** or **AF_INET6**) and interface name **IFLA_IFNAME** as ascii string (if available) as arguments to command. If interface name is not available, nsn shall pass space character followed by NULL byte as ifname. All arguments are delivered exactly as **netlink** hands tem to nsn, nsn performs no checking for these parameters (except for existance of ifname).
* **-a --addr-removed-command, -A --addr-added-command**  
  **-a** and **-A** install commands to be executed when address is added / removed from interface. Interface index for interface to which address was bound, address family like **AF_INET** or **AF_INET6** and prefix lenght of address are given as parameters to command. Also the address is given as last parameter (if available) If address is not available, zero is passed instead of address. See **rtnetlink**(7) for more accurate explanation of these parameters
* **-r --route-removed-command, -R --route-added-command**  
  **-r** and **-R** install commands to be executed when route is added / removed from routing tables. Address family, destination (if available) and destination lenght (in format xxx/len), routing table, route type, gateway address and source address are given as parameters to command being executed. If destination is not given (like may be for default gateway), zero is given as destination. Similarly if gateway or source addresses are not given, zero is passed as missing address to program being called. See **rtnetlink**(7) for more accurate explanation of these parameters
  
  
  
* **Supported suboptions for -s**  
  
  **route4**
  Listens IPv4 routing table changes
  
  **route6**
  Listens IPv6 routing table changes
  
  **addr6**
  Listens for IPv6 address changes
  
  **link6**
  Listens for changes in IPv6 interfaces
  
  
  
* **-V**  
  Verbose, print details from nlmsgs
* **-q**  
  Quiet, print only errors.
* **-v**  
  display version and exit
* **-h**  
  display help and exit
* **-?**  
  display help and exit
  

# Files

*N/A*
: N/A


# Bugs

* **All versions**
Currently the kernel does not advertice changes to driver state as expected. Thus, if for example the cable is unplugged, we get status notification from kernel but change bits for driver state are not set. NSN does not notice this change.

If someone is interested in implementing initial state getting (sending RTM_GETLINK) and state caching in order to get the changes - feel free =)


* **Version 0.2 - Fall of Rome**  
  The event notifications should not be interpreted as absolute state information, since monitored state may change again between kernel sending notification of change, and nsn calling your program. So information nsn passes may not be valid anymore. However it is guaranteed that change really has happened, and if new changes have happened after that, nsn will fire your command again.
  
  Please inform all new bugs to Mazziesaccount@gmail.com

# Author

Matti Vaittinen &lt;[Mazziesaccount@gmail.com](mailto:Mazziesaccount@gmail.com)&gt; For license information see LICENSE file in package root.


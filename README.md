nsn
===

nsn(8) is a network status monitoring tool. It listens changes in network and can execute commands like shell scripts when status changes are detected. 

It does listening changes in network configuration using netlink sockets, and prints informative messages  to  console or log file. nsn(8) was written to be used as debug tool with network problems but it can also serve as simple example code for using netlink sockets to listen network changes or to find out what netlink sends when routes addresses links or neighbour cache is changed. 

As  of version 0.2 it supports executing command when route/address/interface state changes.

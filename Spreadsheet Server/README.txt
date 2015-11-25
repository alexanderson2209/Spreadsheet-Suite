Project 2: Spreadsheet Server
CS 3505 - Spring 2015
Team: SegFault
 - Alex Anderson
 - Garrett Bigelow
 - CJ Dimaano
 - Eric Stubbs


Instructions for compiling the server:

The server can be compiled by issuing the make command at the command prompt.
The default make rule will only build the server executable.  The make clean
command will remove all object files.  The make cleardata command will clear all
previously generated server data, which includes the users file and the
spreadsheet files.
 
 
Instructions for running the server:

At the command prompt, issue the command:

    ./server [port]
    
By default, the port is set to 2000.  Other valid port numbers are in the range
[2112...2120].  To shut down the server, the command STOP may be entered at any
time during execution.
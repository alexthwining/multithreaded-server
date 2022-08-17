# Multithreaded HTTP Server

Originally completed for school: Nov 2021

This program is a HTTP reverse proxy with load balancing and caching. It will distribute connections over a set of servers. 
It will support concurrency and be multithreaded. It will correspond with the health check that was implemented in assignment 2 to keep track of the performance of the server. 

## Installation

Download the repo and at least make sure `httpproxy.c`, `queue.c`, `queue.h`, and `Makefile` are in the same directory.

## Usage

First build the program

```bash
make
```

Then run

```bash
./httpproxy port server_ports [optional: -N -R -s -m]
```

### Default parameters
- port: starts the proxy on this port
- server_ports: balance the load between however many other ports

### Optional parameters
- `-N`: number of parallel connections
    - default: N=5, must be positive integer
- `-R`: how often(in terms of requests) proxy will retrieve the health check from the servers
    - default: R=5, must be positive integer
- `-s`: capacity of the cache
    - default: 3, must be positive integer
- `-m`: maximum file size in bytes to be stored
    - default: 1024, must be positive integer
        - when `-s` or `-m` is 0, caching is disabled

## Credits

Used https://www.makeareadme.com/ as a template for this README.md


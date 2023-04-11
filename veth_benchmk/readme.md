# veth benchmarking setup

Build the relevant xdp-progs and applications by running

```cmd
# make
```

Setup the containers by running the container_setup.sh script. This will create the
following setup.

```bash
#    +-------------------+                         +------------------+
#    |    cndp-frr1      |                         |    cndp-frr2     |
#    |                   |                         |                  |
#    | +=====+  +=====+  |                         | +=====+ +=====+  |
#    | |veth2|  |veth4|  |                         | |veth6| |veth8|  |
#    | +==|==+  +==|==+  |                         | +==|==+ +==|==+  |
#    +----|--------|-----+                         +----|-------|-----+
#      +==|==+  +==|==+                              +==|==+ +==|==+
#      |veth1|  |veth3|                              |veth5| |veth7|
#      +==|==+  +==|==+                              +==|==+ +==|==+
#         |        |                                    |       |
#         |        |                                    |       |
#       +-|--------|------------------------------------|-------|-+
#       |                     br-0                                |
#       +---------------------------------------------------------+
```

Run the veth_setup.sh script to:

- Install the xdp-redirection program on veth1 and veth5.
- Install the xdp-pass program on veth7 and veth3.

> **_NOTE:_** Modify the MAC addresses in the script as appropriate.

```cmd
# ./veth_setup.sh -n
```

```bash
#    +-------------------+                         +------------------+
#    |    cndp-frr1      |                         |    cndp-frr2     |
#    |                   |                         |                  |
#    | +=====+  +=====+  |                         | +=====+ +=====+  |
#    | |veth2|  |veth4|  |                         | |veth6| |veth8|  |
#    | +==|==+  +==|==+  |                         | +==|==+ +==|==+  |
#    +----|--------|-----+                         +----|-------|-----+
#      +==|==+  +==|==+                              +==|==+ +==|==+
#      |veth1|  |veth3|                              |veth5| |veth7|
#      +==|==+  +==^==+                              +==|==+ +==^==+
#         |        |______redirect veth 5 to veth3______|       |
#         |______________redirect veth 1 to veth7_______________|
```

Connect to both containers in separate terminals and run the cndp applications.

## txgen

```cmd
# docker exec -ti cndp-frr1 /cndp/builddir/usrtools/txgen/app/txgen -c /cndp/builddir/usrtools/txgen/app/txgen.jsonc
```

Configure the txgen app by inputting the following at the `TXGen:/>` prompt:

```
set 0 dst mac 4e:13:c4:59:8f:7e
set 0 dst ip 192.168.100.20
set 0 src ip 192.168.200.10/32
set 0 size 512
```

> **_NOTE:_** Modify the MAC addresses as appropriate.


To start traffic use:

```cmd
TXGen:/> start 0
```

To stop traffic use:

```cmd
TXGen:/> stp
```

## cnet-graph

```cmd
# docker exec -ti cndp-frr2 bash
[root@de1ccffe54fc /]# cd cndp/builddir/examples/cnet-graph/
[root@de1ccffe54fc cnet-graph]# ./cnet-graph -c cnetfwd-graph.jsonc
```

to see the stats for 5 seconds use the gstats command:

```bash
*** CNET-GRAPH Application, Mode: Drop, Burst Size: 128

*** cnet-graph, PID: 49 lcore: 0
(chnl_open               : 163) ERR:  TCP is disabled

** Version: CNDP 22.08.0, Command Line Interface
CNDP-cli:/> gstats 5
+------------------+---------------+---------------+--------+--------+----------+------------+
|Node              |          Calls|        Objects| Realloc|  Objs/c|   KObjs/c|    Cycles/c|
+------------------+---------------+---------------+--------+--------+----------+------------+
|ip4_input         |              0|              0|       1|     0.0|       0.0|         0.0|
|ip4_output        |              0|              0|       1|     0.0|       0.0|         0.0|
|ip4_forward       |              0|              0|       1|     0.0|       0.0|         0.0|
|ip4_proto         |              0|              0|       1|     0.0|       0.0|         0.0|
|udp_input         |              0|              0|       1|     0.0|       0.0|         0.0|
|udp_output        |              0|              0|       1|     0.0|       0.0|         0.0|
|pkt_drop          |              0|              0|       1|     0.0|       0.0|         0.0|
|chnl_callback     |              0|              0|       1|     0.0|       0.0|         0.0|
|chnl_recv         |              0|              0|       1|     0.0|       0.0|         0.0|
|kernel_recv       |        9981680|              0|       2|     0.0|       0.0|      1685.0|
|eth_rx-0          |        9981726|              0|       2|     0.0|       0.0|        74.0|
|eth_rx-1          |        9981767|              0|       2|     0.0|       0.0|       138.0|
|arp_request       |              0|              0|       1|     0.0|       0.0|         0.0|
|eth_tx-0          |              0|              0|       1|     0.0|       0.0|         0.0|
|eth_tx-1          |              0|              0|       1|     0.0|       0.0|         0.0|
|punt_kernel       |              0|              0|       1|     0.0|       0.0|         0.0|
|ptype             |              0|              0|       1|     0.0|       0.0|         0.0|
|gtpu_input        |              0|              0|       1|     0.0|       0.0|         0.0|
+------------------+---------------+---------------+--------+--------+----------+------------+
```
#!/bin/bash

#    +------------------+                         +------------------+
#    |    cndp-frr1     |                         |    cndp-frr2     |
#    |                  |                         |                  |
#    | +==============+ |                         | +==============+ |
#    | |veth2 || veth4| |                         | |veth6 || veth8| |
#    | +===|=======|==+ |                         | +===|=======|==+ |
#    +-----|-------|----+                         +-----|-------|----+
#      +===|=======|==+                             +===|=======|==+
#      |veth1 || veth3|                             |veth5 || veth7|
#      +===|=======^===+                            +===|=======^==+
#          |       |______redirect veth 5 to veth3______|       |
#          |______________redirect veth 1 to veth7______________|
#
#
#####################################
# Cleanup
#####################################

docker stop cndp-frr1 cndp-frr2; docker rm cndp-frr1 cndp-frr2
ip link del veth1; ip link del veth3; ip link del veth5; ip link del veth7
ip link del br0
rm -rf /var/run/netns/*

#####################################
# Start containers and copy configs
#####################################

docker run -dit --name cndp-frr1 --privileged cndp-veth-bench
docker run -dit --name cndp-frr2 --privileged cndp-veth-bench
docker cp crc.diff cndp-frr1:/cndp/crc.diff
docker cp apply-crc.sh cndp-frr1:/cndp/
docker exec cndp-frr1 ./cndp/apply-crc.sh
docker cp cnetfwd-graph.jsonc cndp-frr2:/cndp/builddir/examples/cnet-graph/cnetfwd-graph.jsonc
docker cp txgen.jsonc cndp-frr1:/cndp/builddir/usrtools/txgen/app/txgen.jsonc

#####################################
# Expose NS
#####################################

echo "expose container cndp-frr1 netns"
NETNS1=`docker inspect -f '{{.State.Pid}}' cndp-frr1`

if [ ! -d /var/run/netns ]; then
    mkdir /var/run/netns
fi
if [ -f /var/run/netns/$NETNS1 ]; then
    rm -rf /var/run/netns/$NETNS1
fi

ln -s /proc/$NETNS1/ns/net /var/run/netns/$NETNS1
echo "done. netns: $NETNS1"

echo "expose container cndp-frr2 netns"
NETNS2=`docker inspect -f '{{.State.Pid}}' cndp-frr2`

if [ ! -d /var/run/netns ]; then
    mkdir /var/run/netns
fi
if [ -f /var/run/netns/$NETNS2 ]; then
    rm -rf /var/run/netns/$NETNS2
fi

ln -s /proc/$NETNS2/ns/net /var/run/netns/$NETNS2
echo "done. netns: $NETNS2"

echo "============================="
echo "current network namespaces: "
echo "============================="
ip netns

###############################################
# Setup veth
###############################################


echo "creating and connecting veth interfaces"

ip link add veth1 type veth peer name veth2
ip link set veth2 netns $NETNS1

ip link add veth3 type veth peer name veth4
ip link set veth4 netns $NETNS1

ip netns exec $NETNS1 ip addr add 192.168.200.10/24 dev veth2
ip netns exec $NETNS1 ip link set veth2 up
ip link set veth1 up

ip netns exec $NETNS1 ip addr add 192.168.100.20/24 dev veth4
ip netns exec $NETNS1 ip link set veth4 up
ip link set veth3 up

# Add a bridge named br0
ip link add name br0 type bridge

# Attach the veth pair to bridge
ip link set dev veth1 master br0
ip link set dev veth3 master br0

# Add an IP to the bridge
ip addr add 192.168.100.1/24 dev br0
ip link set dev br0 up

# If you can't ping between the 2 containers:
echo 0 > /proc/sys/net/bridge/bridge-nf-call-iptables
# https://serverfault.com/questions/117788/linux-bridging-not-forwarding-packets

echo "creating and connecting veth interfaces"

ip link add veth5 type veth peer name veth6
ip link set veth6 netns $NETNS2

ip link add veth7 type veth peer name veth8
ip link set veth8 netns $NETNS2

ip netns exec $NETNS2 ip addr add 192.168.100.30/24 dev veth6
ip netns exec $NETNS2 ip link set veth6 up
ip link set veth5 up

ip netns exec $NETNS2 ip addr add 192.168.200.40/24 dev veth8
ip netns exec $NETNS2 ip link set veth8 up
ip link set veth7 up

# Attach the veth pair to bridge
ip link set dev veth5 master br0
ip link set dev veth7 master br0

docker exec -ti cndp-frr2 arp -s 192.168.100.20 36:da:e7:35:a9:bc
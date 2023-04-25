#!/bin/bash

#####################################
# Cleanup
#####################################

docker stop cndp-frr1 cndp-frr2; docker rm cndp-frr1 cndp-frr2
ip link del veth1
ip link del veth3
ip link del br0
rm -rf var/run/netns/*
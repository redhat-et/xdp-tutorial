# !/bin/bash
HOOK=N
while getopts "ns" flag; do
  case $flag in
    n) HOOK=N      ;;
    s) HOOK=S ;;
    *) echo 'error unkown flag' >&2
       exit 1
  esac
done

xdp-loader unload veth1 --all
xdp-loader unload veth3 --all
rm -rf /sys/fs/bpf/veth1
rm -rf /sys/fs/bpf/veth3
./xdp_redirect_user -d veth1 -$HOOK
./xdp_pass_user -d veth3 -$HOOK
./xdp_prog_user -d veth1 -r veth3 --src-mac 02:a7:a2:bc:51:30 --dest-mac 36:da:e7:35:a9:bc

# docker exec cndp-frr1  dnf -y install ethtool
# docker exec cndp-frr2  dnf -y install ethtool

# docker exec cndp-frr1 ethtool -K veth2 gro on
# docker exec cndp-frr2 ethtool -K veth4 gro on
# ethtool -K veth1 gro on
# ethtool -K veth3 gro on

# ethtool -K veth3 rx-udp-gro-forwarding on
# https://developers.redhat.com/articles/2021/11/05/improve-udp-performance-rhel-85#availability_in_red_hat_enterprise_linux_8_5

# Enable GRO on the veth peer in the main network namespace:
# VETH=<veth device name>
# CPUS=`/usr/bin/nproc`
# ethtool -K $VETH gro on
# ethtool -L $VETH rx $CPUS tx $CPUS
# echo 50000 > /sys/class/net/$VETH/gro_flush_timeout


# Enable GRO forwarding on that device:
# ethtool -K $VETH rx-udp-gro-forwarding on


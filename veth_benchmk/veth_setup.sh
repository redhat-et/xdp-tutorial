# !/bin/bash

xdp-loader unload veth1 --all
xdp-loader unload veth7 --all
xdp-loader unload veth5 --all
xdp-loader unload veth3 --all
rm -rf /sys/fs/bpf/veth1
rm -rf /sys/fs/bpf/veth5
# mkdir -p /sys/fs/bpf/veth1
# mkdir -p /sys/fs/bpf/veth5
# # mount bpffs /sys/fs/bpf/veth1 -t bpf
# # mount bpffs /sys/fs/bpf/veth5 -t bpf
./xdp_redirect_user -d veth1 -N
./xdp_pass_user -d veth7 -N
./xdp_redirect_user -d veth5 -N
./xdp_pass_user -d veth3 -N
./xdp_prog_user -d veth1 -r veth7 --src-mac 02:a7:a2:bc:51:30 --dest-mac 4e:13:c4:59:8f:7e
./xdp_prog_user -d veth5 -r veth3 --src-mac 4e:13:c4:59:8f:7e --dest-mac 02:a7:a2:bc:51:30

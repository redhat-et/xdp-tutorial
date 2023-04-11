/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("xdp")
int xdp_pass_func(struct xdp_md *ctx)
{
	//      cat /sys/kernel/debug/tracing/trace_pipe
	bpf_printk("xdp_pass_func %s", __FUNCTION__);
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";

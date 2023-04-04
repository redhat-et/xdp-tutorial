/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <linux/in.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
#include <netinet/ether.h>

/* Header cursor to keep track of current parsing position */
struct hdr_cursor {
    void *pos;
};


static __always_inline int parse_ethhdr(struct hdr_cursor *nh,
                    void *data_end,
                    struct ethhdr **ethhdr)
{
    struct ethhdr *eth = nh->pos;
    int hdrsize = sizeof(*eth);
    __u16 h_proto;

    /* Byte-count bounds check; check if current pointer + size of header
     * is after data_end.
     */
    if (nh->pos + hdrsize > data_end)
        return -1;

    nh->pos += hdrsize;
    *ethhdr = eth;
    h_proto = eth->h_proto;

    return h_proto; /* network-byte-order */
}


struct {
	__uint(type, BPF_MAP_TYPE_DEVMAP);
	__type(key, int);
	__type(value, int);
	__uint(max_entries, 256);
	__uint(pinning, LIBBPF_PIN_BY_NAME);
} tx_port SEC(".maps");


struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__type(key,  unsigned char [ETH_ALEN]);
	__type(value, unsigned char [ETH_ALEN]);
	__uint(max_entries, 1);
	__uint(pinning, LIBBPF_PIN_BY_NAME);
} redirect_params SEC(".maps");

/* Solution to packet03/assignment-3 */
SEC("xdp")
int xdp_prog_redirect(struct xdp_md *ctx)
{
	void *data_end = (void *)(long)ctx->data_end;
	void *data = (void *)(long)ctx->data;
	struct hdr_cursor nh;
	struct ethhdr *eth;
	int eth_type;
	unsigned char *dst;
    int *value;
	int index = 0;

	/* These keep track of the next header type and iterator pointer */
	nh.pos = data;

	/* Parse Ethernet and IP/IPv6 headers */
	eth_type = parse_ethhdr(&nh, data_end, &eth);
	if (eth_type == -1){
//      cat /sys/kernel/debug/tracing/trace_pipe
//		bpf_printk("Dont know the ethtype");
		goto out;
	}

	/* Do we know where to redirect this packet? */
	dst = bpf_map_lookup_elem(&redirect_params, eth->h_source);
	if (!dst) {
//		bpf_printk("bpf_map_lookup_elem failed");
		goto out;
	}

	value = bpf_map_lookup_elem(&tx_port, &index);
	if(!value){
//		bpf_printk("bpf_map_lookup_elem tx_port failed");
		goto out;
	}
//	bpf_printk("REDIRECTING PACKET to ifindex %d", *value);

//	bpf_printk("REDIRECTING PACKET");
	/* Set a proper destination address */
	return bpf_redirect_map(&tx_port, 0, 0);

out:
	return XDP_PASS;
}

char _license[] SEC("license") = "GPL";

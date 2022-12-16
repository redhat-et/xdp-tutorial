/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__type(key, __u32);
	__type(value, __u64);
	__uint(max_entries, 2);
} redirect_err_cnt SEC(".maps");

#define XDP_UNKNOWN	XDP_REDIRECT + 1
struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__type(key, __u32);
	__type(value, __u64);
	__uint(max_entries, 6);
} exception_cnt SEC(".maps");

/* Tracepoint format: /sys/kernel/debug/tracing/events/xdp/xdp_redirect/format
 * Code in:                kernel/include/trace/events/xdp.h
 */
struct xdp_redirect_ctx {
	__u64 pad;
	int prog_id;		//	offset: 0; size:4; signed:1;
	__u32 act;		//	offset: 4  size:4; signed:0;
	int ifindex;		//	offset: 8  size:4; signed:1;
	int err;		//	offset:12  size:4; signed:1;
	int to_ifindex; 	//	offset:16  size:4; signed:1;
	__u32 map_id;		//	offset:20  size:4; signed:0;
	int map_index;		//	offset:24  size:4; signed:1;
};				//	offset:28

enum {
	XDP_REDIRECT_SUCCESS = 0,
	XDP_REDIRECT_ERROR = 1
};

static __always_inline
int xdp_redirect_collect_stat(struct xdp_redirect_ctx *ctx)
{
	__u32 key = XDP_REDIRECT_ERROR;
	int err = ctx->err;
	__u64 *cnt;

	if (!err)
		key = XDP_REDIRECT_SUCCESS;

	cnt  = bpf_map_lookup_elem(&redirect_err_cnt, &key);
	if (!cnt)
		return 1;
	*cnt += 1;

	return 0; /* Indicate event was filtered (no further processing)*/
	/*
	 * Returning 1 here would allow e.g. a perf-record tracepoint
	 * to see and record these events, but it doesn't work well
	 * in-practice as stopping perf-record also unload this
	 * bpf_prog.  Plus, there is additional overhead of doing so.
	 */
}

SEC("tracepoint/xdp/xdp_redirect_err")
int trace_xdp_redirect_err(struct xdp_redirect_ctx *ctx)
{
	return xdp_redirect_collect_stat(ctx);
}

SEC("tracepoint/xdp/xdp_redirect_map_err")
int trace_xdp_redirect_map_err(struct xdp_redirect_ctx *ctx)
{
	return xdp_redirect_collect_stat(ctx);
}

/* Likely unloaded when prog starts */
SEC("tracepoint/xdp/xdp_redirect")
int trace_xdp_redirect(struct xdp_redirect_ctx *ctx)
{
	return xdp_redirect_collect_stat(ctx);
}

/* Likely unloaded when prog starts */
SEC("tracepoint/xdp/xdp_redirect_map")
int trace_xdp_redirect_map(struct xdp_redirect_ctx *ctx)
{
	return xdp_redirect_collect_stat(ctx);
}

/* Tracepoint format: /sys/kernel/debug/tracing/events/xdp/xdp_exception/format
 * Code in:                kernel/include/trace/events/xdp.h
 */
struct xdp_exception_ctx {
	__u64 pad;
	int prog_id;	//	offset:0; size:4; signed:1;
	__u32 act;	//	offset:4; size:4; signed:0;
	int ifindex;	//	offset:8; size:4; signed:1;
};

SEC("tracepoint/xdp/xdp_exception")
int trace_xdp_exception(struct xdp_exception_ctx *ctx)
{
	__u64 *cnt;
	__u32 key;

	key = ctx->act;
	if (key > XDP_REDIRECT)
		key = XDP_UNKNOWN;

	cnt = bpf_map_lookup_elem(&exception_cnt, &key);
	if (!cnt)
		return 1;
	*cnt += 1;

	return 0;
}

/* Common stats data record shared with _user.c */
struct datarec {
	__u64 processed;
	__u64 dropped;
	__u64 info;
	__u64 err;
};
#define MAX_CPUS 64

struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__type(key, __u32);
	__type(value, struct datarec);
	__uint(max_entries, MAX_CPUS);
} cpumap_enqueue_cnt SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__type(key, __u32);
	__type(value, struct datarec);
	__uint(max_entries, 1);
} cpumap_kthread_cnt SEC(".maps");

/* Tracepoint: /sys/kernel/debug/tracing/events/xdp/xdp_cpumap_enqueue/format
 * Code in:         kernel/include/trace/events/xdp.h
 */
struct cpumap_enqueue_ctx {
	__u64 pad;
	int map_id;		//	offset: 0; size:4; signed:1;
	__u32 act;		//	offset: 4; size:4; signed:0;
	int cpu;		//	offset: 8; size:4; signed:1;
	unsigned int drops;	//	offset:12; size:4; signed:0;
	unsigned int processed; //	offset:16; size:4; signed:0;
	int to_cpu;		//	offset:20; size:4; signed:1;
};

SEC("tracepoint/xdp/xdp_cpumap_enqueue")
int trace_xdp_cpumap_enqueue(struct cpumap_enqueue_ctx *ctx)
{
	__u32 to_cpu = ctx->to_cpu;
	struct datarec *rec;

	if (to_cpu >= MAX_CPUS)
		return 1;

	rec = bpf_map_lookup_elem(&cpumap_enqueue_cnt, &to_cpu);
	if (!rec)
		return 0;
	rec->processed += ctx->processed;
	rec->dropped   += ctx->drops;

	/* Record bulk events, then userspace can calc average bulk size */
	if (ctx->processed > 0)
		rec->info += 1;

	return 0;
}

/* Tracepoint: /sys/kernel/debug/tracing/events/xdp/xdp_cpumap_kthread/format
 * Code in:         kernel/include/trace/events/xdp.h
 */
struct cpumap_kthread_ctx {
	__u64 pad;
	int map_id;		//	offset: 0; size:4; signed:1;
	__u32 act;		//	offset: 4; size:4; signed:0;
	int cpu;		//	offset: 8; size:4; signed:1;
	unsigned int drops;	//	offset:12; size:4; signed:0;
	unsigned int processed; //	offset:16; size:4; signed:0;
	int sched;		//	offset:20; size:4; signed:1;
};

SEC("tracepoint/xdp/xdp_cpumap_kthread")
int trace_xdp_cpumap_kthread(struct cpumap_kthread_ctx *ctx)
{
	struct datarec *rec;
	__u32 key = 0;

	rec = bpf_map_lookup_elem(&cpumap_kthread_cnt, &key);
	if (!rec)
		return 0;
	rec->processed += ctx->processed;
	rec->dropped   += ctx->drops;

	/* Count times kthread yielded CPU via schedule call */
	if (ctx->sched)
		rec->info++;

	return 0;
}
struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__type(key, __u32);
	__type(value, struct datarec);
	__uint(max_entries, 1);
} devmap_xmit_cnt SEC(".maps");

/* Tracepoint: /sys/kernel/debug/tracing/events/xdp/xdp_devmap_xmit/format
 * Code in:         kernel/include/trace/events/xdp.h
 */
struct devmap_xmit_ctx {
	__u64 pad;
	int from_ifindex;	//	offset: 0; size:4; signed:1;
	__u32 act;		//	offset: 4; size:4; signed:0;
	int to_ifindex;		//	offset: 8; size:4; signed:1;
	int drops;		//	offset:12; size:4; signed:1;
	int sent;		//	offset:16; size:4; signed:1;
	int err;		//	offset:28; size:4; signed:1;
};

SEC("tracepoint/xdp/xdp_devmap_xmit")
int trace_xdp_devmap_xmit(struct devmap_xmit_ctx *ctx)
{
	struct datarec *rec;
	__u32 key = 0;

	rec = bpf_map_lookup_elem(&devmap_xmit_cnt, &key);
	if (!rec)
		return 0;
	rec->processed += ctx->sent;
	rec->dropped   += ctx->drops;

	/* Record bulk events, then userspace can calc average bulk size */
	rec->info += 1;

	/* Record error cases, where no frame were sent */
	if (ctx->err)
		rec->err++;

	/* Catch API error of drv ndo_xdp_xmit sent more than count */
	if (ctx->drops < 0)
		rec->err++;

	return 1;
}

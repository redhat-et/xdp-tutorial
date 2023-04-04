/* SPDX-License-Identifier: GPL-2.0 */
static const char *__doc__ = "Simple XDP prog doing XDP_PASS\n";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <bsd/string.h>
#include <linux/bpf.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <xdp/libxdp.h>

#include <net/if.h>
#include <linux/if_link.h> /* depend on kernel-headers installed */

#include "../common/common_params.h"
#include "../common/common_user_bpf_xdp.h"

#ifndef PATH_MAX
#define PATH_MAX	4096
#endif

static int _verbose = 1;
const char *pin_basedir =  "/sys/fs/bpf";

/* Pinning maps under /sys/fs/bpf in subdir */
int pin_maps_in_bpf_object(struct bpf_object *bpf_obj, const char *subdir)
{
	char map_filename[PATH_MAX];
	char pin_dir[PATH_MAX];
	int err, len;

	len = snprintf(pin_dir, PATH_MAX, "%s/%s", pin_basedir, subdir);
	if (len < 0) {
		fprintf(stderr, "ERR: creating pin dirname\n");
		return EXIT_FAIL_OPTION;
	}

	len = snprintf(map_filename, PATH_MAX, "%s/%s/",
		       pin_basedir, subdir);
	if (len < 0) {
		fprintf(stderr, "ERR: creating pin dir\n");
		return EXIT_FAIL_OPTION;
	}

	/* Existing/previous XDP prog might not have cleaned up */
	if (access(map_filename, F_OK ) != -1 ) {
		if (_verbose)
			printf(" - Unpinning (remove) prev maps in %s/\n",
			       pin_dir);

		/* Basically calls unlink(3) on map_filename */
		err = bpf_object__unpin_maps(bpf_obj, pin_dir);
		if (err) {
			fprintf(stderr, "ERR: UNpinning maps in %s %s\n", pin_dir, strerror(errno));
			//return EXIT_FAIL_BPF;
		}
	}
	if (_verbose)
		printf(" - Pinning maps in %s/\n", pin_dir);

	/* This will pin all maps in our bpf_object */
	err = bpf_object__pin_maps(bpf_obj, pin_dir);
	if (err){
        printf("%s: Couldn't bpf_object__pin_maps(%s)\n",
	 		__FUNCTION__, strerror(errno));
		return EXIT_FAIL_BPF;
	}

	return 0;
}

static const struct option_wrapper long_options[] = {
	{{"help",        no_argument,		NULL, 'h' },
	 "Show help", false},

	{{"dev",         required_argument,	NULL, 'd' },
	 "Operate on device <ifname>", "<ifname>", true},

	{{"skb-mode",    no_argument,		NULL, 'S' },
	 "Install XDP program in SKB (AKA generic) mode"},

	{{"native-mode", no_argument,		NULL, 'N' },
	 "Install XDP program in native mode"},

	{{"auto-mode",   no_argument,		NULL, 'A' },
	 "Auto-detect SKB or native mode"},

	{{"unload",      required_argument,	NULL, 'U' },
	 "Unload XDP program <id> instead of loading", "<id>"},

	{{"unload-all",  no_argument,           NULL,  4  },
	 "Unload all XDP programs on device"},

	{{0, 0, NULL,  0 }, NULL, false}
};


int main(int argc, char **argv)
{
	// struct bpf_object *obj;
	// struct bpf_program *prog;
	// struct bpf_link *link;

	struct bpf_prog_info info = {};
	__u32 info_len = sizeof(info);
	char filename[] = "xdp_prog_kern_redirect.o";
	char path[512];
	char progname[] = "xdp_prog_redirect";
	struct xdp_program *prog;
	char errmsg[1024];
	int prog_fd = -1, err = EXIT_SUCCESS;

	struct config cfg = {
		.attach_mode = XDP_MODE_UNSPEC,
		.ifindex   = -1,
		.do_unload = false,
	};

	strncpy(cfg.filename, filename, sizeof(cfg.filename));
    strncpy(cfg.progname, progname, sizeof(cfg.progname));

	parse_cmdline_args(argc, argv, long_options, &cfg, __doc__);
	snprintf(path, sizeof(path), "/sys/fs/bpf/%s", cfg.ifname);
	strncpy(cfg.pin_dir, path, sizeof(cfg.pin_dir));

	DECLARE_LIBBPF_OPTS(bpf_object_open_opts, bpf_opts, .pin_root_path = cfg.pin_dir);
	DECLARE_LIBXDP_OPTS(xdp_program_opts, xdp_opts,
                            .open_filename = cfg.filename,
                            .prog_name = cfg.progname,
                            .opts = &bpf_opts);
	/* Required option */
	if (cfg.ifindex == -1) {
		fprintf(stderr, "ERR: required option --dev missing\n");
		usage(argv[0], __doc__, long_options, (argc == 1));
		return EXIT_FAIL_OPTION;
	}


    // obj = bpf_object__open_file(cfg.filename, &bpf_opts);
	// err = libbpf_get_error(obj);
	// if (err) {
	// 	printf("%s: Couldn't open file(%s)\n",
	// 		__FUNCTION__, cfg.filename);
	// 	return err;
	// }

	// prog = bpf_object__find_program_by_name(obj, cfg.progname);
	// if (!prog) {
	// 	printf("%s: Couldn't find xdp program in bpf object!\n",	__FUNCTION__);
	// 	err = -ENOENT;
	// 	return err;
	// }
	// bpf_program__set_type(prog, BPF_PROG_TYPE_XDP);

	// err = bpf_object__load(obj);
	// if (err) {
	//  	printf("%s: Couldn't load BPF-OBJ file(%s) %s\n",
	//  		__FUNCTION__, filename, strerror(errno));
	//  	return err;
	// }

	// printf("%s: bpf: Attach prog to ifindex %d\n", __FUNCTION__, cfg.ifindex);
	// link = bpf_program__attach_xdp(prog, cfg.ifindex);
	// if (!link) {
	// 	printf("%s:ERROR: failed to attach program to %s\n", __FUNCTION__, cfg.ifname);
	// 	return err;
	// }

	// if (_verbose) {
	// 	printf("Success: Loaded BPF-object(%s) and used section(%s)\n",
	// 	       cfg.filename, cfg.progname);
	// 	printf(" - XDP prog attached on device:%s(ifindex:%d)\n",
	// 	       cfg.ifname, cfg.ifindex);
	// }

	// /* Use the --dev name as subdir for exporting/pinning maps */
	// err = pin_maps_in_bpf_object(obj, cfg.ifname);
	// if (err) {
	// 	fprintf(stderr, "ERR: pinning maps\n");
	// 	return err;
	// }
    /* Create an xdp_program froma a BPF ELF object file */
	prog = xdp_program__create(&xdp_opts);
	err = libxdp_get_error(prog);
	if (err) {
		libxdp_strerror(err, errmsg, sizeof(errmsg));
		fprintf(stderr, "Couldn't get XDP program %s: %s\n",
			cfg.progname, errmsg);
		return err;
	}

        /* Attach the xdp_program to the net device XDP hook */
	err = xdp_program__attach(prog, cfg.ifindex, cfg.attach_mode, 0);
	if (err) {
		libxdp_strerror(err, errmsg, sizeof(errmsg));
		fprintf(stderr, "Couldn't attach XDP program on iface '%s' : %s (%d)\n",
			cfg.ifname, errmsg, err);
		return err;
	}

    /* This step is not really needed , BPF-info via bpf-syscall */
	prog_fd = xdp_program__fd(prog);
	err = bpf_obj_get_info_by_fd(prog_fd, &info, &info_len);
	if (err) {
		fprintf(stderr, "ERR: can't get prog info - %s\n",
			strerror(errno));
		return err;
	}
	printf("Success: Loading "
	       "XDP prog name:%s(id:%d) on device:%s(ifindex:%d)\n",
	       info.name, info.id, cfg.ifname, cfg.ifindex);
	return EXIT_OK;
}

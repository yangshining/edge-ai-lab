#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <gpiod.h>
#include <libuboot.h>
#include "rlog.h"

#define CFG_NAME		"/etc/fw_env.config"

static int set_env(char *env, char *value)
{
	int ret;
	struct uboot_ctx *ctx;

	if (libuboot_initialize(&ctx, NULL) < 0) {
		return 1;
	}

	if ((ret = libuboot_read_config(ctx, CFG_NAME)) < 0) {
		goto ERR0;
	}

	if ((ret = libuboot_open(ctx)) < 0) {
		goto ERR0;
	}

	if (0 != libuboot_set_env(ctx, env, value)) {
		goto ERR1;
	}
	printf("Switched to Slot[%s] - Slot Info Transitioned!\n", value);
	ret = libuboot_env_store(ctx);
	if (ret)
		goto ERR1;

	libuboot_close(ctx);
	libuboot_exit(ctx);
	return 0;

ERR1:
	libuboot_close(ctx);
ERR0:
	libuboot_exit(ctx);
	return 1;
}

static int get_env(const char *env_name, char *data, size_t len) {
    int ret;
    struct uboot_ctx *ctx;
    const char *value;

    if (libuboot_initialize(&ctx, NULL) < 0) {
        return 1;
    }

    if ((ret = libuboot_read_config(ctx, CFG_NAME)) < 0) {
        goto ERR;
    }

    if ((ret = libuboot_open(ctx)) < 0) {
        goto ERR;
    }

    value = libuboot_get_env(ctx, env_name);

        if (NULL == value) {
                goto ERR;
        }
        else if (strlen(value) > len) {
                goto ERR;
        }
        else {
                strncpy(data, value, len);
        }

    libuboot_close(ctx);
    libuboot_exit(ctx);
    return 0;

ERR:
    libuboot_close(ctx);
    libuboot_exit(ctx);
    return 1;
}

void usage(char *prog)
{
	printf("usage: %s 1 or 2 ...\n",prog);
	printf("\n");
	printf("1 ro 2 Specify boot partition\n");
}

static void gpio_set(unsigned int gpio_num, int value)
{
	unsigned int offset = gpio_num;
	int v = value;

	gpiod_ctxless_set_value_multiple("gpiochip0", &offset, &v, 1, false, "led_gpio", NULL, NULL);
	return;
}

int main(int argc, char *argv[])
{
	char iangv;
	if(argc != 2) {
		usage(argv[0]);
		exit(0);
	}
	iangv = argv[1][0];

	char zboot_value[16];
    pid_t pid = getpid();
    char process_name[128];
    ssize_t len = readlink("/proc/self/exe", process_name, sizeof(process_name) - 1);
    if (len != -1) {
        process_name[len] = '\0';
    } else {
        perror("readlink");
        return 1;
    }
	get_env("zboot", zboot_value, sizeof(zboot_value));
	switch(iangv)
	{
		case '1' :
			if(0 != strcmp(zboot_value, "1") && 0 != set_env("zboot", "1")){
				printf("set_env failed\n");
				exit(-1);
			}
			break;
		case '2' :
			if(0 != strcmp(zboot_value, "2") && 0 != set_env("zboot", "2")){
				printf("set_env failed\n");
				exit(-1);
			}
			break;
		default :
			usage(argv[0]);
			exit(0);
	}

	write_rlog("Manual restart", "Warm", process_name, pid, "re_boot command invoked");
	usleep(10000);
	system("reboot");

	return 0;
}

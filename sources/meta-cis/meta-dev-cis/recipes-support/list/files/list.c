/*
 * xi.he@zillnk.com.
 *
 * ubus cli example.
 */
#include <stdio.h>
#include <unistd.h>
#include <libubox/blobmsg_json.h>
#include <libubox/ustream.h>
#include <json-c/json.h>
#include <libubus.h>
#include <signal.h>
#include "list.h"

int DEBUG_LOG = 1;
char recievebuf[1024];
/*
enum ubus_msg_status {
	UBUS_STATUS_OK,
	UBUS_STATUS_INVALID_COMMAND,
	UBUS_STATUS_INVALID_ARGUMENT,
	UBUS_STATUS_METHOD_NOT_FOUND,
	UBUS_STATUS_NOT_FOUND,
	UBUS_STATUS_NO_DATA,
	UBUS_STATUS_PERMISSION_DENIED,
	UBUS_STATUS_TIMEOUT,
	UBUS_STATUS_NOT_SUPPORTED,
	UBUS_STATUS_UNKNOWN_ERROR,
	UBUS_STATUS_CONNECTION_FAILED,
	__UBUS_STATUS_LAST
};
*/
#define upgrade_object          "list"
#define upgrade_get_method      "info"
#define EVENT_NAME              "list"
#define LIST_CRC_ENABLE			"crc"

enum {
	IPC_SLOT_LIST_MESSAGE,
	__IPC_SLOT_LIST_MAX,
};

static const struct blobmsg_policy ipc_slot_list_policy[__IPC_SLOT_LIST_MAX] = {
	[IPC_SLOT_LIST_MESSAGE] = { .name = "message", .type = BLOBMSG_TYPE_STRING },
};

struct ubus_context *ctx = NULL;
static struct blob_buf b;

__attribute__((unused)) static void ubus_list_event(struct ubus_context *ctx, struct ubus_event_handler *ev,
			  const char *type, struct blob_attr *msg) {
	char *message;
	struct blob_attr *tb[__IPC_SLOT_LIST_MAX];
	if (!msg)
		return;
	blobmsg_parse(ipc_slot_list_policy, __IPC_SLOT_LIST_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[IPC_SLOT_LIST_MESSAGE]) {
		fprintf(stderr, "No return code received from server\n");
		return;
	}
	message = blobmsg_get_string(tb[IPC_SLOT_LIST_MESSAGE]);
	printf("%s",message);
	uloop_done();
	ubus_free(ctx);
	uloop_end();
	exit(1);
}

__attribute__((unused)) static void upgrade_ubus_list_events() {
	static struct ubus_event_handler listener;
 
	/* 注册特定event的listener。多个event可以使用同一个listener */
	memset(&listener, 0, sizeof(listener));
	listener.cb = ubus_list_event;

	if (ubus_register_event_handler(ctx, &listener, EVENT_NAME)) {
		printf("register event failed.\n");
		return;
	}

}

static int ubus_cli_call(struct ubus_context *ctx, ubus_call_info_t *info)
{
	uint32_t id;
	int ret;
	int timeout = 30;//30ms

	blob_buf_init(&b, 0);
    if(info->jsobj != NULL){
        if (!blobmsg_add_object(&b, info->jsobj)){
            MSG_ERROR("blobmsg_add_object fail.\n");
        }
    }

	ret = ubus_lookup_id(ctx, info->object, &id);
	if (ret){
		MSG_WARNING("Failed to look up %s object ret:%d.\n", info->object, ret);
		return ret;
	}
    //show_ubus_call_json(&b);
	ret = ubus_invoke(ctx, id, info->method, b.head, NULL, (void *)info, timeout * 1000);
	return ret;
}

static int ubus_call(char *object, char *method, char *json) {
	int ret = 0;

	ubus_call_info_t *call_info = NULL;
	struct json_object *jsobj = NULL;

	if(object == NULL|| method == NULL){
		MSG_ERROR("parameters cannot be NULL.\n");
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
    if(json == NULL){
	    jsobj = NULL;
    }else{
        jsobj = json_tokener_parse(json);
    }
	call_info = malloc(sizeof(ubus_call_info_t));
    if(call_info == NULL){
        return UBUS_STATUS_UNKNOWN_ERROR;
    }

	call_info->object= object;
	call_info->method= method;
	call_info->jsobj= jsobj;

    //ubus 执行完成会等待回调函数，调用mqtt_network_info_response 返回正常数据
	ret = ubus_cli_call(ctx, call_info);
	json_object_put(jsobj);

	free(call_info);
	return ret;
}

__attribute__((unused)) static int list(char *json)
{
	int ubus_ret = -1;

	ubus_ret = ubus_call(upgrade_object, upgrade_get_method, json);
	if(ubus_ret){
		MSG_ERROR("ubus call %s %s error:%d.\n", upgrade_object, upgrade_get_method, ubus_ret);
	}
	return ubus_ret;
}

static void usage(char *program)
{
	fprintf(stdout, "%s (compiled %s)\n", program, __DATE__);
	fprintf(stdout, "Usage %s [OPTION]\n",
			program);
	fprintf(stdout,
		" crc,                       : enable crc for list command\n"
	);
}

int main(int argc, char **argv) { 
	const char *ubus_socket = NULL;
	unsigned int crc = 0;
	char list_param[16] = {0};
	signal(SIGINT,SIG_IGN);
	if(argc == 2){
		if(strncmp(LIST_CRC_ENABLE, argv[1], strlen(LIST_CRC_ENABLE)) == 0){
			crc = 1;
		} else {
			usage(argv[0]);
			exit(0);
		}
	}
	sprintf(list_param,"{\"crc\":%d}", crc);
	uloop_init();
	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}
	ubus_add_uloop(ctx);
	upgrade_ubus_list_events();
   	list(list_param);
	uloop_run();
    return 0;
}

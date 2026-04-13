#ifndef _UBUS_LIST_H
#define _UBUS_LIST_H

extern int DEBUG_LOG;

#define MSG_DEBUG(format, ...)                                                                      \
    do                                                                                              \
    {                                                                                               \
    	if (DEBUG_LOG){		                                                                        \
        	fprintf(stdout, "[DEBUG][%s:%d]" format "\r\n", __FILE__, __LINE__, ##__VA_ARGS__);     \
        }					                                                                        \
    } while (0)

#define MSG_ERROR(format, ...)                                                                      \
    do                                                                                              \
    {                                                                                               \
    	if (DEBUG_LOG){		                                                                        \
        	fprintf(stdout, "[ERROR][%s:%d]" format "\r\n", __FILE__, __LINE__, ##__VA_ARGS__);     \
        }					                                                                        \
    } while (0)

#define MSG_WARNING(format, ...)                                                                      \
    do                                                                                              \
    {                                                                                               \
    	if (DEBUG_LOG){		                                                                        \
        	fprintf(stdout, "[WARNING][%s:%d]" format "\r\n", __FILE__, __LINE__, ##__VA_ARGS__);     \
        }					                                                                        \
    } while (0)

typedef struct _ubus_call_info{
	char *object;
	char *method;
    struct json_object *jsobj;
}ubus_call_info_t;

#endif

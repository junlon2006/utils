#ifndef EVENT_LIST_INC_UNI_EVENT_LIST_H_
#define EVENT_LIST_INC_UNI_EVENT_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_LIST_PRIORITY_HIGHEST  (1 << 0)
#define EVENT_LIST_PRIORITY_MEDIUM   (1 << 1)
#define EVENT_LIST_PRIORITY_LOWEST   (1 << 2)

typedef void* EventListHandle;
typedef void (*EventListEventHandler)(void *event);
typedef void (*EventListEventFreeHandler)(void *event);

EventListHandle EventListCreate(EventListEventHandler event_handler,
                                EventListEventFreeHandler free_handler);
int             EventListDestroy(EventListHandle handle);
int             EventListAdd(EventListHandle handle, void *event, int priority);
int             EventListClear(EventListHandle handle);

#ifdef __cplusplus
}
#endif
#endif

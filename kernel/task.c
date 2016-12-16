#define LOG_TAG "task"

#include <debug.h>
#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

struct task_list {
    Task task;
    MessageId id;
    Message message;
    struct task_list *next;
};

static struct task_list *first = NULL;
static struct task_list *tail = NULL;

void MessageLoop(void)
{
    while(true) {
        if(first != NULL) {
            struct task_list *curr = first;
            first = first->next;
            curr->task->handler(curr->task, curr->id, curr->message);
            free(curr);
        }
    }
}

void MessageSend(Task task, MessageId id, Message message)
{
    struct task_list * task_list = PanicUnlessNew(struct task_list);
    task_list->task = task;
    task_list->id = id;
    task_list->message = message;
    task_list->next = NULL;
    if(first == NULL) {
        first = task_list;
        tail = task_list;
    } else {
        tail->next = task_list;
        tail = task_list;
    }
}

void MessageSendLater(Task task, MessageId id, Message message, Delay delay)
{
    LOGW("Not Impl MessageSendLater, delay %d\n", delay);
    MessageSend(task, id, message);
}

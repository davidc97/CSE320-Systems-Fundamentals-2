#include "queue.h"
#include "errno.h"

void P(sem_t *s);
void V(sem_t *s);

queue_t *create_queue(void) {
    queue_t *queue;
    if((queue = (queue_t*)calloc(1, sizeof(queue_t))) == NULL){
        return NULL;
    }
    queue->front = NULL;
    queue->rear = NULL;
    if(sem_init(&queue->items,0,0) == -1){
        return NULL;
    }
    if(pthread_mutex_init(&queue->lock, NULL) != 0){
        return NULL;
    }
    queue->invalid = false;
    return queue;
}

bool invalidate_queue(queue_t *self, item_destructor_f destroy_function) {
    if(self == NULL || destroy_function == NULL){
        errno = EINVAL;
        return false;
    }
    pthread_mutex_lock(&self->lock);
    void *item;
    queue_node_t *node;
    while(self->front != NULL){
        node = self->front;
        item = self->front->item;
        self->front = self->front->next;
        destroy_function(item);
        free(node);
    }
    self->invalid = true;
    pthread_mutex_unlock(&self->lock);
    return true;
}

bool enqueue(queue_t *self, void *item) {
    if(self == NULL || item == NULL){
        errno = EINVAL;
        return false;
    }
    if(self->invalid == true){
        errno = EINVAL;
        return false;
    }
    pthread_mutex_lock(&self->lock);
    queue_node_t *node = (queue_node_t*)calloc(1, sizeof(queue_node_t));
    node->item = item;
    if(self->front == NULL){
        self->front = node;
        self->rear = node;
        node->next = NULL;
    }
    else{
        self->rear->next = node;
        self->rear = node;
    }
    pthread_mutex_unlock(&self->lock);
    V(&self->items);
    return true;
}

void *dequeue(queue_t *self) {
    if(self == NULL){
        errno = EINVAL;
        return NULL;
    }
    if(self->invalid == true){
        errno = EINVAL;
        return NULL;
    }
    void *item;
    P(&self->items);
    pthread_mutex_lock(&self->lock);
    queue_node_t *node = self->front;
    item = node->item;
    self->front = self->front->next;
    free(node);
    pthread_mutex_unlock(&self->lock);
    return item;
}

void P(sem_t *s){
    sem_wait(s);
}
void V(sem_t *s){
    sem_post(s);
}

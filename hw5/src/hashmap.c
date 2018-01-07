#include "utils.h"
#include "errno.h"

#define MAP_KEY(base, len) (map_key_t) {.key_base = base, .key_len = len}
#define MAP_VAL(base, len) (map_val_t) {.val_base = base, .val_len = len}
#define MAP_NODE(key_arg, val_arg, tombstone_arg) (map_node_t) {.key = key_arg, .val = val_arg, .tombstone = tombstone_arg}

hashmap_t *create_map(uint32_t capacity, hash_func_f hash_function, destructor_f destroy_function) {
    if(capacity <= 0){
        errno = EINVAL;
        return NULL;
    }
    if(hash_function == NULL || destroy_function == NULL){
        errno = EINVAL;
        return NULL;
    }
    hashmap_t *map;
    if((map = (hashmap_t*)calloc(1, sizeof(hashmap_t))) == NULL){
        return NULL;
    }
    if(pthread_mutex_init(&map->write_lock, NULL) != 0){
        return NULL;
    }
    if(pthread_mutex_init(&map->fields_lock, NULL) != 0){
        return NULL;
    }
    map_node_t *nodes;
    if((nodes = (map_node_t*)calloc(capacity, sizeof(map_node_t))) == NULL){
        return NULL;
    }
    map->nodes = nodes;
    map->capacity = capacity;
    map->size = 0;
    map->hash_function = hash_function;
    map->destroy_function = destroy_function;
    map->num_readers = 0;
    map->invalid = 0;
    return map;
}

bool put(hashmap_t *self, map_key_t key, map_val_t val, bool force) {
    if(self == NULL || key.key_base == NULL || val.val_base == NULL){
        errno = EINVAL;
        return false;
    }
    if(self->invalid == 1){
        errno = EINVAL;
        return false;
    }
    pthread_mutex_lock(&self->write_lock);
    bool result = false;
    int index = get_index(self,key);
    map_node_t *ptr = self->nodes + index;
    for(int i = 0;i < self->capacity; i++){
        if((uint32_t)(ptr - self->nodes) >= self->capacity){
            ptr = self->nodes;
        }
        if(ptr->key.key_base == key.key_base && ptr->key.key_len == key.key_len && ptr->tombstone == true){
            ptr->val = val;
            ptr->tombstone = false;
            result = true;
            break;
        }
        if(ptr->key.key_base == NULL || ptr->tombstone == true){
            ptr->key = key;
            ptr->val = val;
            ptr->tombstone = false;
            result = true;
            break;
        }
        ptr++;
    }
    if(result == false && force == true){
        ptr = self->nodes + index;
        self->destroy_function(ptr->key,ptr->val);
        ptr->key = key;
        ptr->val = val;
        ptr->tombstone = false;
        result = true;
    }
    else if (result == false && force == false){
        errno = ENOMEM;
    }
    if(result == true){
        self->size++;
    }
    pthread_mutex_unlock(&self->write_lock);
    return result;
}

map_val_t get(hashmap_t *self, map_key_t key) {
    if(self == NULL){
        errno = EINVAL;
        return MAP_VAL(NULL, 0);
    }
    if(self->invalid == 1){
        errno = EINVAL;
        return MAP_VAL(NULL, 0);
    }
    if(key.key_base == NULL || key.key_len == 0){
        errno = EINVAL;
        return MAP_VAL(NULL,0);
    }
    pthread_mutex_lock(&self->fields_lock);
    self->num_readers++;
    if(self->num_readers == 1){
        pthread_mutex_lock(&self->write_lock);
    }
    pthread_mutex_unlock(&self->fields_lock);
    map_val_t result;
    int index = get_index(self,key);
    map_node_t *ptr = self->nodes + index;
    for(int i = 0; i < self->capacity; i++){
        if((uint32_t)(ptr - self->nodes) >= self->capacity){
            ptr = self->nodes;
        }
        if(ptr->key.key_base == key.key_base && ptr->key.key_len == key.key_len){
            result = ptr->val;
            break;
        }
        if(ptr->key.key_base == NULL && ptr->key.key_len == 0 && ptr->tombstone == false){
            result = MAP_VAL(NULL, 0);
            break;
        }
        ptr++;
    }
    pthread_mutex_lock(&self->fields_lock);
    self->num_readers--;
    if(self->num_readers == 0){
        pthread_mutex_unlock(&self->write_lock);
    }
    pthread_mutex_unlock(&self->fields_lock);
    return result;
}

map_node_t delete(hashmap_t *self, map_key_t key) {
    if(self == NULL || key.key_base == NULL){
        errno = EINVAL;
    }
    pthread_mutex_lock(&self->write_lock);
    map_node_t result = MAP_NODE(MAP_KEY(NULL, 0), MAP_VAL(NULL, 0), false);
    int index = get_index(self,key);
    map_node_t *ptr = self->nodes + index;
    for(int i = 0;i < self->capacity; i++){
        if((uint32_t)(ptr - self->nodes) >= self->capacity){
            ptr = self->nodes;
        }
        if(ptr->key.key_base == key.key_base && ptr->key.key_len == key.key_len){
            ptr->tombstone = true;
            result = *ptr;
            self->size--;
            break;
        }
        if(ptr->key.key_base == NULL && ptr->tombstone == false){
            break;
        }
        ptr++;
    }
    pthread_mutex_unlock(&self->write_lock);
    return result;
}

bool clear_map(hashmap_t *self) {
    if(self == NULL){
        errno = EINVAL;
        return false;
    }
    pthread_mutex_lock(&self->write_lock);
    bool result = false;
    map_node_t *ptr = self->nodes;
    for(int i = 0;i < self->capacity; i++){
        if(ptr->key.key_base != NULL){
            self->destroy_function(ptr->key,ptr->val);
        }
        ptr++;
    }
    self->size = 0;
    result = true;
    pthread_mutex_unlock(&self->write_lock);
	return result;
}

bool invalidate_map(hashmap_t *self) {
    if(self == NULL){
        errno = EINVAL;
        return false;
    }
    pthread_mutex_lock(&self->write_lock);
    bool result = false;
    map_node_t *ptr = self->nodes;
    for(int i = 0;i < self->capacity; i++){
        if(ptr->key.key_base != NULL){
            self->destroy_function(ptr->key,ptr->val);
        }
        ptr++;
    }
    self->size = 0;
    self->invalid = true;
    free(self->nodes);
    result = true;
    pthread_mutex_unlock(&self->write_lock);
    return result;
}

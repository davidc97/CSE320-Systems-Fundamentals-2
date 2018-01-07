#include "cream.h"
#include "utils.h"
#include "csapp.h"
#include "hashmap.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void destroy_map(map_key_t key, map_val_t val);
void *thread(void *vargp);
int handle_put(request_header_t rheader, hashmap_t *data_store, int connfd);
int handle_get(request_header_t rheader, hashmap_t *data_store, int connfd);
int handle_evict(request_header_t rheader, hashmap_t *data_store, int connfd);
int handle_clear(hashmap_t *data_store, int connfd);
int handle_invalid(int connfd);
request_header_t parse_request(int connfd);

typedef struct cream_data {
    queue_t *request_queue;
    hashmap_t *data_store;
} cream_data;

int main(int argc, char *argv[]) {
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t *tidlist;

    if(strcmp(argv[1], "-h") == 0){
        printf("./cream [-h] NUM_WORKERS PORT_NUMBER MAX_ENTRIES \n"
                "-h                 Displays this help menu and returns EXIT_SUCCESS. \n"
                "NUM_WORKERS        The number of worker threads used to service requests. \n"
                "PORT_NUMBER        Port number to listen on for incoming connections. \n"
                "MAX_ENTRIES        The maximum number of entries that can be stored in cream's underlying data store. \n");
        return EXIT_SUCCESS;
    }
    else if(argc != 4){
        printf("Invalid Arguments \n");
        return EXIT_FAILURE;
    }
    long NUM_WORKERS,PORT_NUMBER,MAX_ENTRIES;
    char *ptr;
    NUM_WORKERS = strtol(argv[1], &ptr, 10);
    PORT_NUMBER = strtol(argv[2], &ptr, 10);
    MAX_ENTRIES = strtol(argv[3], &ptr, 10);
    queue_t *request_queue;
    request_queue = create_queue();
    hashmap_t *data_store;
    data_store = create_map(MAX_ENTRIES,jenkins_one_at_a_time_hash,destroy_map);
    cream_data *data = (cream_data *)calloc(1, sizeof(cream_data));
    data->request_queue = request_queue;
    data->data_store = data_store;
    tidlist = (pthread_t *)calloc(NUM_WORKERS, sizeof(pthread_t));
    for(int i = 0; i < NUM_WORKERS; i++){
       if(pthread_create(tidlist+i,NULL,thread,data) != 0){
        fprintf(stderr, "error: Cannot create thread # %d\n", i);
       }
    }
    listenfd = Open_listenfd(PORT_NUMBER);
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        enqueue(request_queue,connfdp);
    }
    exit(0);
}
void *thread(void *vargp){
    queue_t *request_queue = ((cream_data *) vargp)->request_queue;
    hashmap_t *data_store = ((cream_data *)vargp)->data_store;
    while(1){
        int connfd = *((int *)dequeue(request_queue));
        request_header_t rheader = parse_request(connfd);
        if(rheader.request_code == PUT){
            handle_put(rheader, data_store, connfd);
        }
        else if(rheader.request_code == GET){
            handle_get(rheader, data_store, connfd);
        }
        else if(rheader.request_code == EVICT){
            handle_evict(rheader, data_store, connfd);
        }
        else if(rheader.request_code == CLEAR){
            handle_clear(data_store,connfd);
        }
        else{
            handle_invalid(connfd);
        }
        Close(connfd);
        return NULL;
    }
}
request_header_t parse_request(int connfd){
    request_header_t rheader;
    Rio_readn(connfd, &rheader, sizeof(rheader));
    return rheader;
}
int handle_put(request_header_t rheader, hashmap_t *data_store, int connfd){
    response_header_t response_header;
    map_key_t* key = (map_key_t*)malloc(sizeof(map_key_t));
    map_val_t* val = (map_val_t*)malloc(sizeof(map_val_t));
    bool valid = false;
    if(rheader.key_size >= MIN_KEY_SIZE && rheader.key_size <= MAX_KEY_SIZE &&
        rheader.value_size >= MIN_VALUE_SIZE && rheader.value_size <= MAX_VALUE_SIZE){
        Rio_readn(connfd, key, rheader.key_size);
        Rio_readn(connfd, val, rheader.value_size);
        valid = true;
    }
    if(valid == false){
        response_header.response_code = BAD_REQUEST;
        response_header.value_size = 0;
    }
    else if(put(data_store,*key,*val,true) == false){
        response_header.response_code = BAD_REQUEST;
        response_header.value_size = 0;
    }
    response_header.response_code = OK;
    response_header.value_size = rheader.value_size;
    Rio_writen(connfd, &response_header, sizeof(response_header));
    return 1;
}
int handle_get(request_header_t rheader, hashmap_t *data_store, int connfd){
    response_header_t response_header;
    map_key_t* key = (map_key_t*)malloc(sizeof(map_key_t));
    map_val_t* val = (map_val_t*)malloc(sizeof(map_val_t));
    bool valid = false;
    if(rheader.key_size >= MIN_KEY_SIZE && rheader.key_size <= MAX_KEY_SIZE){
        Rio_readn(connfd, key, rheader.key_size);
        *val = get(data_store,*key);
        valid = true;
        response_header.response_code = OK;
    response_header.value_size = val->val_len;
    Rio_writen(connfd, &response_header, sizeof(response_header));
    Rio_writen(connfd, val, sizeof(response_header.value_size));
    return 1;
    }
    if(valid == false){
        response_header.response_code = BAD_REQUEST;
        response_header.value_size = 0;
        Rio_writen(connfd, &response_header, sizeof(response_header));
        return 0;
    }
    else if(val->val_base == NULL){
        response_header.response_code = NOT_FOUND;
        response_header.value_size = 0;
        Rio_writen(connfd, &response_header, sizeof(response_header));
        return 0;
    }
    return 1;
}
int handle_evict(request_header_t rheader, hashmap_t *data_store, int connfd){
    response_header_t response_header;
    //map_key_t* key = (map_key_t*)malloc(sizeof(map_key_t));
    if(rheader.key_size >= MIN_KEY_SIZE && rheader.key_size <= MAX_KEY_SIZE){
        // Rio_readn(connfd, key, rheader.key_size);
        // delete(data_store, *key);
    }
    response_header.response_code = OK;
    response_header.value_size = 0;
    Rio_writen(connfd, &response_header, sizeof(response_header));
    return 1;
}
int handle_clear(hashmap_t *data_store, int connfd){
    response_header_t response_header;
    clear_map(data_store);
    response_header.response_code = OK;
    response_header.value_size = 0;
    Rio_writen(connfd, &response_header, sizeof(response_header));
    return 1;
}
int handle_invalid(int connfd){
    response_header_t response_header;
    response_header.response_code = UNSUPPORTED;
    response_header.value_size = 0;
    Rio_writen(connfd, &response_header, sizeof(response_header));
    return 1;
}
void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
    unix_error("Close error");
}
// void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp,
//             void * (*routine)(void *), void *argp)
// {
//     int rc;

//     if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
//     posix_error(rc, "Pthread_create error");
// }

void destroy_map(map_key_t key, map_val_t val){
    key.key_base = NULL;
    key.key_len = 0;
    val.val_base = NULL;
    val.val_len = 0;
}
void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
    unix_error("Accept error");
    return rc;
}

int open_listenfd(int port)
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1;

    /* Eliminates "Address already in use" error from bind */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
           (const void *)&optval , sizeof(int)) < 0)
    return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
    return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
    return -1;
    return listenfd;
}
int Open_listenfd(int port)
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
    unix_error("Open_listenfd error");
    return rc;
}

/*********************************************************************
 * The Rio package - robust I/O functions
 **********************************************************************/
/*
 * rio_readn - robustly read n bytes (unbuffered)
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
    if ((nread = read(fd, bufp, nleft)) < 0) {
        if (errno == EINTR) /* Interrupted by sig handler return */
        nread = 0;      /* and call read() again */
        else
        return -1;      /* errno set by read() */
    }
    else if (nread == 0)
        break;              /* EOF */
    nleft -= nread;
    bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
    if ((nwritten = write(fd, bufp, nleft)) <= 0) {
        if (errno == EINTR)  /* Interrupted by sig handler return */
        nwritten = 0;    /* and call write() again */
        else
        return -1;       /* errno set by write() */
    }
    nleft -= nwritten;
    bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/*
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* Refill if buf is empty */
    rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
               sizeof(rp->rio_buf));
    if (rp->rio_cnt < 0) {
        if (errno != EINTR) /* Interrupted by sig handler return */
        return -1;
    }
    else if (rp->rio_cnt == 0)  /* EOF */
        return 0;
    else
        rp->rio_bufptr = rp->rio_buf; /* Reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n)
    cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}
/* $end rio_readinitb */

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
    if ((nread = rio_read(rp, bufp, nleft)) < 0)
            return -1;          /* errno set by read() */
    else if (nread == 0)
        break;              /* EOF */
    nleft -= nread;
    bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readnb */

/*
 * rio_readlineb - robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
        *bufp++ = c;
        if (c == '\n') {
                n++;
            break;
            }
    } else if (rc == 0) {
        if (n == 1)
        return 0; /* EOF, no data read */
        else
        break;    /* EOF, some data was read */
    } else
        return -1;    /* Error */
    }
    *bufp = 0;
    return n-1;
}
/* $end rio_readlineb */

/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
    unix_error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n)
    unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
    unix_error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
    unix_error("Rio_readlineb error");
    return rc;
}
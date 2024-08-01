static void websocket_write_back(struct lws *wsi){
    //Check if the websocket instance is NULL
    if (wsi == NULL){
        return -1;
    }

    char *out = NULL;
    char str[100];

    for(int i = 0; i < NUM_THREADS; i++){
        sprintf(str, "{\"type\":\"subscribe\",\"symbol\":\"%s\"}\n", trades[i].symbol);
        //Printing the subscription request
        printf(KBLU"Websocket write back: %s\n"RESET, str);
        int len = strlen(str);
        out = (char *)malloc(sizeof(char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
        memcpy(out + LWS_SEND_BUFFER_PRE_PADDING, str, len);
        lws_write(wsi, out+LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);
    }
    
    //Free the memory
    free(out);
}

case LWS_CALLBACK_CLIENT_WRITEABLE :
            printf(KYEL"[Main Service] The websocket is writeable.\n"RESET);
            //Subscribe to the symbols
            websocket_write_back(wsi);
            //Set flags
            writeable_flag = 1;
            break;




// The queue and its functions
typedef struct {
  TradeData buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;
//Initialize the queue
queue *queueInit (void);
//Delete the queue
void queueDelete (queue *q);
//Add element to the queue
void queueAdd (queue *q, TradeData in);
//Delete element from the queue
void queueDel (queue *q, TradeData *out);
//Function of the producer
void *producer (void *q, int thread_id);
//Function of the consumer
void *consumer(void *q, int thread_id);

// Global queue for each stock symbol
queue *queues[NUM_THREADS];
int main(){
    urlPath[0] = '/';
    strncpy(urlPath + 1, urlTempPath, sizeof(urlPath) - 2);
    urlPath[sizeof(urlPath)-1] = '\0';

    //Setting up the client connection info
    clientConnectionInfo.port = 443;
    clientConnectionInfo.path = urlPath;
    clientConnectionInfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    clientConnectionInfo.host = clientConnectionInfo.address;
    clientConnectionInfo.origin = clientConnectionInfo.address;
    clientConnectionInfo.ietf_version_or_minus_one = -1;
    clientConnectionInfo.protocol = protocols[0].name;

    // Print the connection info
    printf("Testing %s\n\n", clientConnectionInfo.address);
    printf("Connecting to %s://%s:%d%s \n\n", urlProtocol, clientConnectionInfo.address, clientConnectionInfo.port, urlPath);

    // Create the websocket instance
    wsi = lws_client_connect_via_info(&clientConnectionInfo);
    if (wsi == NULL) {
        printf(KRED"[Main] Web socket instance creation error.\n"RESET);
        return -1;
    }

    printf(KGRN"[Main] Successful web socket instance creation.\n"RESET);
    // Initialize queues for each symbol
    char symbols[NUM_THREADS][20] = { "NVDA", "GOOGL", "BINANCE:BTCUSDT", "BINANCE:ETHUSDT" };
    for (int i = 0; i < NUM_THREADS; i++) {
        queues[i] = queueInit();
        if (queues[i] == NULL) {
            fprintf(stderr, "Queue initialization failed for %s.\n", symbols[i]);
            exit(1);
        }
    }

    // Create producer and consumer threads for each symbol
    pthread_t pro[NUM_THREADS], con[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        strcpy(args[i].symbol, symbols[i]);
        args[i].fifo = queues[i];
        pthread_create(&pro[i], NULL, producer, &args[i]);
        pthread_create(&con[i], NULL, consumer, &args[i]);
    }
    
    // Loops until the destroy flag is set to 1 to maintain the websocket connection
    // The destroy flag becomes 1 when the user presses Ctr+C
    while(destroy_flag==0){
        //Service the websocket
        lws_service(context, 1000);
        //sleep is used to save energy and reduce CPU usage
        sleep(2);
    }
    
    // Wait for the threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(pro[i], NULL);
        pthread_join(con[i], NULL);
    }
    // Destroy the queues
    for (int i = 0; i < NUM_THREADS; i++) {
        queueDelete(queues[i]);
    }
    // Exit the pthreads
    pthread_exit(NULL);
    
    // Destroy the websocket connection
    lws_context_destroy(context);

    return 0;
}

//Producer and consumer functions
void *producer(void *q, int thread_id){
    queue *fifo;
    fifo=(queue *)q;

    // Add trade data to the queue
    pthread_mutex_lock(fifo->mut);
    while (fifo->full) {
        pthread_cond_wait(fifo->notFull, fifo->mut);
    }
    
    //queueAdd(fifo, TradeData);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notEmpty);
    
    //sleep(1); // Simulate delay


    return NULL;
}
void *consumer(void *q, int thread_id){
    queue *fifo;
    fifo = (queue *)q;
    TradeData d;
    time_t last_time = 0;

    while (1) {
        pthread_mutex_lock(fifo->mut);
        while (fifo->empty) {
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }

        queueDel(fifo, &d);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);

        time_t current_time = time(NULL);

        if (current_time - last_time >= 60) {
            // Compute statistics every minute
            int count = 0;
            double total_price = 0;
            double total_volume = 0;

            pthread_mutex_lock(fifo->mut);
            for (int i = fifo->head; i != fifo->tail; i = (i + 1) % QUEUESIZE) {
                total_price += fifo->buf[i].price;
                total_volume += fifo->buf[i].volume;
                count++;
            }
            pthread_mutex_unlock(fifo->mut);

            double avg_price = total_price / count;

            printf("Symbol: %s, Avg Price: %f, Total Volume: %f\n", d.symbol, avg_price, total_volume);
            last_time = current_time;
        }
    }

    return NULL;
}
//Queue functions
queue *queueInit (void){
    queue *q;
    q = (queue *)malloc (sizeof (queue));
    if (q == NULL){
        return (NULL);
    }
    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (q->mut, NULL);
    q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notEmpty, NULL);

    return (q);
}
void queueDelete (queue *q){
    pthread_mutex_destroy (q->mut);
    free (q->mut);
    pthread_cond_destroy (q->notFull);
    free (q->notFull);
    pthread_cond_destroy (q->notEmpty);
    free (q->notEmpty);
    free (q);
}
void queueAdd (queue *q, TradeData in){
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE){
        q->tail = 0;
    }
    if (q->tail == q->head){
        q->full = 1;
    }
    q->empty = 0;

    return;
}
void queueDel (queue *q, TradeData *out){
    *out = q->buf[q->head];
    q->head++;
    if (q->head == QUEUESIZE){
        q->head = 0;
    }
    if (q->head == q->tail){
        q->empty = 1;
    }
    q->full = 0;

    return;
}
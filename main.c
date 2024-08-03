#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libwebsockets.h>
#include <pthread.h>
#include <jansson.h>
#include <pthread.h>
#include <time.h>

//Colours for terminal text formatting
#define KGRN "\033[0;32;32m" //Green
#define KCYN "\033[0;36m" //Cyan
#define KRED "\033[0;32;31m" //Red
#define KYEL "\033[1;33m" //Yellow
#define KBLU "\033[0;32;34m" //Blue
#define KCYN_L "\033[1;36m" //Light Cyan
#define KBRN "\033[0;33m" //Brown
//Used at the end of each sentance where colour code was used
//It resets the text colour to the terminal default
#define RESET "\033[0m" //Reset

//Number of threads which is also the number of symbols
#define NUM_THREADS 4
//Queue size
#define QUEUESIZE 10
//Number of loops
#define LOOP 10

//flags to determine the state of the websocket
static int destroy_flag = 0; //destroy flag
static int connection_flag = 0; //connection flag
static int writeable_flag = 0; //writeable flag

// Calculate candlestick timer 60s
static time_t last_message_time = 0;

// This function sets the destroy flag to 1 when the SIGINT signal (Ctr+C) is received
static void interrupt_handler(int signal);
// This function sends a message to the websocket
static void websocket_write_back(struct lws *wsi_in);
//A callback function that handles different websocket events
static int ws_service_callback(struct lws *wsi, enum lws_callback_reasons reason, 
                                void *user, void *in, size_t len);

// A dictionary to store the latest trade data for each symbol
typedef struct {
    char symbol[20];
    double price;
    double volume;
    long long timestamp;
} TradeData;

// An array of TradeData structures to store the latest trade data for each symbol
TradeData trades[NUM_THREADS];

// Update the trade data for a given symbol
void update_trade_data(const char* symbol, double price, double volume, long long timestamp);
// Print the latest trade data for each symbol
void print_latest_trades();

//This array defines the protocols used in the websocket
static struct lws_protocols protocols[]={
	{
		"wss", //protocol name
		ws_service_callback, //callback function
		0, //user data size
		0, //receive buffer size *VERY IMPORTANT TO BE 0*, didn't work otherwise
        //any other value terminated the connection sooner than expected without going into 
        //the cases: LWS_CALLBACK_CLIENT_CONNECTION_ERROR and LWS_CALLBACK_CLOSED
        //with the connection flag always being 1 I couldn't even attempt to reconnect
	},
	{ NULL, NULL, 0, 0 } //terminator
};

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
void *producer (int thread_id);
//Function of the consumer
void *consumer(int thread_id);
// Global queue for each stock symbol
queue *queues[NUM_THREADS];

int main(void){
    // Initialize the trades array with symbols
    strcpy(trades[0].symbol, "NVDA");
    strcpy(trades[1].symbol, "GOOGL");
    strcpy(trades[2].symbol, "BINANCE:BTCUSDT");
    strcpy(trades[3].symbol, "BINANCE:ETHUSDT");

    // register the signal SIGINT handler
    struct sigaction act;
    act.sa_handler = interrupt_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction( SIGINT, &act, 0);

    struct lws_context *context = NULL;
    struct lws_context_creation_info info;
    struct lws *wsi = NULL;
    struct lws_protocols protocol;

    // Setting up the context creation info
    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN; 
    info.protocols = protocols; 
    info.gid = -1; 
    info.uid = -1; 
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.max_http_header_pool = 1024; // Increase if necessary
    //info.pt_serv_buf_size = 4096; // Default is 4096 bytes, increase as needed
    info.pt_serv_buf_size = 16384; // Increase buffer size to 16 KB

    // The URL of the websocket
    char inputURL[300] = "ws.finnhub.io/?token=cqe0rvpr01qgmug3d06gcqe0rvpr01qgmug3d070";
    const char *urlProtocol="wss";
    const char *urlTempPath="/";
    char urlPath[300];

    // Creating the context using the context creation info
    context = lws_create_context(&info);
    printf(KRED"[Main] Successful context creation.\n"RESET);

    if (context == NULL){
        printf(KRED"[Main] Context creation error: Context is NULL.\n"RESET);
        return -1;
    }
    struct lws_client_connect_info clientConnectionInfo;
    memset(&clientConnectionInfo, 0, sizeof(clientConnectionInfo));
    clientConnectionInfo.context = context;
    if (lws_parse_uri(inputURL, &urlProtocol, &clientConnectionInfo.address,
                    &clientConnectionInfo.port, &urlTempPath)){
        // nothing
    }else{
        printf("Couldn't parse the URL\n");
    }

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
    for (int i = 0; i < NUM_THREADS; i++) {
        queues[i] = queueInit();
        if (queues[i] == NULL) {
            fprintf(stderr, "Queue initialization failed for %s.\n", trades[i].symbol);
            exit(1);
        }
    }
    // Create producer and consumer threads for each symbol
    pthread_t pro[NUM_THREADS], con[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&pro[i], NULL, producer, queues[i]);
        pthread_create(&con[i], NULL, consumer, queues[i]);
    }

    // Loops until the destroy flag is set to 1 to maintain the websocket connection
    // The destroy flag becomes 1 when the user presses Ctr+C
    while(destroy_flag==0){
        // Service the WebSocket
        lws_service(context, 500);

        // Print the flags status
        printf(KBRN"Flags-Status\n"RESET);
        printf("Connection flag status: %d\n", connection_flag);
        printf("Destroy flag status: %d\n", destroy_flag);
        printf("Writeable flag status: %d\n", writeable_flag);

        // Sleep to save CPU usage
        //sleep(3);
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


static void iterrupt_handler(int signal){
    destroy_flag = 1;
}

static int ws_service_callback(struct lws *wsi, enum lws_callback_reasons reason, 
                                void *user, void *in, size_t len){
    switch (reason) {
        //This case is called when the connection is established
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf(KYEL"[Main Service] Successful Client Connection.\n"RESET);
            //Set flags
            connection_flag = 1;
	        lws_callback_on_writable(wsi);
            break;
        //This case is called when there is an error in the connection
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf(KRED"[Main Service] Client Connection Error: %s.\n"RESET, in);
            //Set flags
            destroy_flag = 1;
            connection_flag = 0;
            break;
        //This case is called when the connection is closed
        case LWS_CALLBACK_CLOSED:
            printf(KYEL"[Main Service] Websocket connection closed\n"RESET);
            //Set flags
            destroy_flag = 1;
            connection_flag = 0;
            break;
        //This case is called when the client receives a message from the websocket
        case LWS_CALLBACK_CLIENT_RECEIVE:
            printf(KCYN_L"[Main Service] The Client received a message:%s\n"RESET, (char *)in);
            // Get the current time
            time_t current_time = time(NULL);

            // Check if 5 seconds have passed since the last message
            if (difftime(current_time, last_message_time) < 5) {
                // If not enough time has passed, break out of the case
                break;
            }
            // Update the last message received time
            last_message_time = current_time;
            // Parse the received message
            json_t *root;
            json_error_t error;
            root = json_loads((char *)in, 0, &error);
            if (!root) {
                printf("error: on line %d: %s\n", error.line, error.text);
                break;
            }

            json_t *data = json_object_get(root, "data");
            if (!json_is_array(data)) {
                json_decref(root);
                break;
            }

            size_t index;
            json_t *value;
            json_array_foreach(data, index, value) {
                const char *symbol = json_string_value(json_object_get(value, "s"));
                double price = json_real_value(json_object_get(value, "p"));
                double volume = json_real_value(json_object_get(value, "v"));
                long long timestamp = json_integer_value(json_object_get(value, "t"));
                update_trade_data(symbol, price, volume, timestamp);
            }

            // Print the latest trades
            print_latest_trades();

            json_decref(root);
            break;
        
        //This case is called when the client is able to send a message to the websocket
        //It sends a subscription message to the websocket and sets the writeable flag to 1
        case LWS_CALLBACK_CLIENT_WRITEABLE :
            printf(KYEL"[Main Service] The websocket is writeable.\n"RESET);
            //Subscribe to the symbols
            websocket_write_back(wsi);
            //Set flags
            writeable_flag = 1;
            break;
        //The default case just breaks the switch statement
        default:
            break;
    }

    return 0;
}

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

//Update and print trade data
void update_trade_data(const char* symbol, double price, double volume, long long timestamp) {
    for (int i = 0; i < NUM_THREADS; i++) {
        if (strcmp(trades[i].symbol, symbol) == 0) {
            trades[i].price = price;
            trades[i].volume = volume;
            trades[i].timestamp = timestamp;
            return;
        }
    }
}
void print_latest_trades() {
    printf(KBRN"Latest Trades:\n"RESET);
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("%s - Price: %.2f, Volume: %.8f, Timestamp: %lld\n", 
            trades[i].symbol, trades[i].price, trades[i].volume, trades[i].timestamp);
    }
    printf("\n");
}
//Producer and consumer functions
void *producer(int thread_id){
    queue *fifo;
    fifo = queues[thread_id];

    // instead of a while there can be a for loop that runs for a certain number of iterations
    // simulating production rate
    while (!destroy_flag) {
        TradeData in;
        pthread_mutex_lock(fifo->mut);
        while (fifo->full) {
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }
        // this in has to be specified
        queueAdd(fifo, in);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }

    return NULL;
}
void *consumer(int thread_id){
    queue *fifo;
    fifo = queues[thread_id];

    TradeData out;

    FILE *file;
    char filename[256];
    snprintf(filename, sizeof(filename), "trade_data_%s.txt", trades[thread_id].symbol);
    file = fopen(filename, "a");
    if (!file) {
        fprintf(stderr, "Failed to open file for %s.\n", trades[thread_id].symbol);
        pthread_exit(NULL);
    }

    time_t start_time = time(NULL);

    // instead of a while there can be a for loop that runs for a certain number of iterations
    // simulating consumption rate
    while (!destroy_flag) {
        pthread_mutex_lock(fifo->mut);
        while (fifo->empty) {
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }
        queueDel(fifo, &out);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);

        fprintf(file, "Symbol: %s, Price: %.2f, Volume: %.2f, Timestamp: %lld\n", out.symbol, out.price, out.volume, out.timestamp);
        time_t current_time = time(NULL);
        if (difftime(current_time, start_time) >= 60) {
            // Calculate metrics (e.g., VWAP, Total Volume, etc.) here based on collected data.
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

            printf("Symbol: %s, Avg Price: %f, Total Volume: %f\n", 
                    out.symbol, avg_price, total_volume);
            // Reset the start time for the next minute.
            start_time = current_time;
        }
    }

    fclose(file);

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
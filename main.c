#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libwebsockets.h>
#include <pthread.h>
#include <jansson.h>
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
#define NUM_THREADS 2
//Queue size
#define QUEUESIZE 200
// Delay array size 
#define DELAYSIZE 1000

struct timespec start, end;

//flags to determine the state of the websocket
static int destroy_flag = 0; //destroy flag
static int connection_flag = 0; //connection flag
static int writeable_flag = 0; //writeable flag

// Declare candlestick variables
double total_volume[NUM_THREADS];
double min_price[NUM_THREADS];
double max_price[NUM_THREADS];
double open_price[NUM_THREADS];
double close_price[NUM_THREADS];

// Declare moving average variables
int minute[NUM_THREADS];
double price_sum[NUM_THREADS][15];
int count[NUM_THREADS][15];
double avg[NUM_THREADS];
int total_count[NUM_THREADS];

// Declare delay variables
double dequeue_time[NUM_THREADS];
double delay[NUM_THREADS];
double idle_time[NUM_THREADS];

// This function sets the destroy flag to 1 when the SIGINT signal (Ctr+C) is received
// This is used to close the websocket connection and free the memory
static void interrupt_handler(int signal);
// This function sends a message to the websocket
static void websocket_write_back(struct lws *wsi_in);
//A callback function that handles different websocket events
static int ws_service_callback(struct lws *wsi, enum lws_callback_reasons reason, 
                                void *user, void *in, size_t len);

// A dictionary to store the latest trade data for each symbol
typedef struct {
    int thread_id;
    char symbol[20];
    double price;
    double volume;
    double timestamp;
    double enqueue_time;
} TradeData;

// An array of TradeData structures to store the latest trade data for each symbol
TradeData trades[NUM_THREADS];
// Write the trade data to a file
void write_trade_to_file(const char* symbol, double price, double volume, long long timestamp);
// Write the candlestick data to a file
void write_candlestick_to_file(const char* symbol, double open_price, double close_price, double min_price, double max_price, double total_volume);
// Write the moving average price to a file
void write_avg_to_file(const char* symbol, double price_sum);
// Write the delay between enqueuing and dequeuing a Tradedata to a file
void write_delay_to_file(const char* symbol, double delay);
// Clear the contents of a file
void clear_file(const char* filename);



//This array defines the protocols used in the websocket
static struct lws_protocols protocols[]={
	{
		"example-protocol", //protocol name
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
void *producer (void *arg);
//Function of the consumer
void *consumer(void *arg);
// Global queue for each stock symbol
queue *queues[NUM_THREADS];// Create producer and consumer threads for each symbol
// Create producer and consumer threads for each symbol
pthread_t pro[NUM_THREADS], con[NUM_THREADS];

// Candlestick timer
time_t start_time[NUM_THREADS];
time_t current_time[NUM_THREADS];

int main(void){
    // Initialize the trades array with symbols
    strcpy(trades[0].symbol, "BINANCE:BTCUSDT");
    strcpy(trades[1].symbol, "BINANCE:ETHUSDT");
    //strcpy(trades[2].symbol, "NVDA");
    //strcpy(trades[3].symbol, "GOOGL");
    
    // Clear the contents of the log files
    for (int i = 0; i < NUM_THREADS; i++) {
        char trade_filename[50];
        char candlestick_filename[50];
        char avg_filename[50];
        char delay_filename[50];
        snprintf(trade_filename, sizeof(trade_filename), "logs/%s.txt", trades[i].symbol);
        snprintf(candlestick_filename, sizeof(candlestick_filename), "candlesticks/%s_candlestick.txt", trades[i].symbol);
        snprintf(avg_filename, sizeof(avg_filename), "averages/%s_avg.txt", trades[i].symbol);
        snprintf(delay_filename, sizeof(delay_filename), "delays/%s_delays.csv", trades[i].symbol);
        clear_file(trade_filename);
        clear_file(candlestick_filename);
        clear_file(avg_filename);
        clear_file(delay_filename);
    }

    // Initialize the candlestick data and the moving average data for each symbol
    for (int i = 0; i < NUM_THREADS; i++) {
        // candlestick
        min_price[i] = 0;
        max_price[i] = 0;
        total_volume[i] = 0;
        
        // moving average
        minute[i] = 0;
        total_count[i] = 0;
        avg[i]=0;
        for(int j=0;j<15;j++){
            price_sum[i][j]=0;
            count[i][j]=0;
        }
        idle_time[i] = 0.0;
    }

    // Register the signal SIGINT handler
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
    info.pt_serv_buf_size = 4096; // Default is 4096 bytes, increase buffer size to 16 KB if needed 16384 bytes

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

    // Determine the time the program divides into threads enters the main loop
    struct timespec program_start, program_end;
    clock_gettime(CLOCK_MONOTONIC, &program_start);

    // Create a the conusmer threads to manage the data from the producer threads
    // while we wait for the next message
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&con[i], NULL, consumer, (void *)(intptr_t)i);
    }

    // Loops until the destroy flag is set to 1 to maintain the websocket connection
    // The destroy flag becomes 1 when the user presses Ctr+C
    while(!destroy_flag){
        // Service the WebSocket
        lws_service(context, 1000);

        // Print the flags status
        printf(KBRN"Flags-Status\n"RESET);
        printf("C: %d, W: %d, D: %d\n", connection_flag, writeable_flag, destroy_flag);

    }
    // Determine the time the program exits the main loop
    clock_gettime(CLOCK_MONOTONIC, &program_end);
    double total_execution_time = (program_end.tv_sec - program_start.tv_sec) + 
                              (program_end.tv_nsec - program_start.tv_nsec) / 1e9;
    
    printf(KBRN"[Main] Total Execution Time: %.4f seconds.\n"RESET, total_execution_time);
    double total_idle_time = 0.0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_idle_time += idle_time[i];
    }
    printf(KBRN"[Main] CPU Idle Percentage: %.4f.\n"RESET, (total_idle_time / (total_execution_time * NUM_THREADS))*100);
    // Destroy the websocket connection
    lws_context_destroy(context);

    // Join the pthreads
    for (int i = 0; i < NUM_THREADS; i++){
        pthread_join(con[i], NULL);
        pthread_join(pro[i], NULL);
    }
    // Exit the pthreads
    pthread_exit(NULL);

    // Destroy the queues
    for (int i = 0; i < NUM_THREADS; i++) {
        queueDelete(queues[i]);
    }

    // Free the delay array
    for (int i = 0; i < NUM_THREADS; i++) {
        free_delay_array(i);
    }
    return 0;
}


static void interrupt_handler(int signal){
    destroy_flag = 1;
    // Get the pthreads unstuck from any condition wait
    for(int i = 0; i < NUM_THREADS; i++){
        pthread_cond_broadcast(queues[i]->notEmpty);
        pthread_cond_broadcast(queues[i]->notFull);
    }
    printf(KYEL"[Main] Program terminated.\n"RESET);
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
                double price = json_number_value(json_object_get(value, "p"));
                double volume = json_number_value(json_object_get(value, "v"));
                long long timestamp = json_integer_value(json_object_get(value, "t"));
                for (int i = 0; i < NUM_THREADS; i++) {
                    if (strcmp(trades[i].symbol, symbol) == 0) {
                        trades[i].thread_id = i;
                        trades[i].price = price;
                        trades[i].volume = volume;
                        trades[i].timestamp = timestamp;
                        // Get the current time
                        if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
                            perror(KRED"Error getting the enqueue time"RESET);
                            return 1;
                        }
                        trades[i].enqueue_time = start.tv_sec + start.tv_nsec / 1e9;
                        pthread_create(&pro[i], NULL, producer, (void *)&trades[i]);
                    }
                }

            }

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
        printf(KRED"[Websocket write back] Websocket instance is NULL.\n"RESET);
        return;
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
void write_trade_to_file(const char* symbol, double price, double volume, long long timestamp) {
    char filename[50];
    if (strlen(symbol)!=0){
        snprintf(filename, sizeof(filename), "logs/%s.txt", symbol);
        
        FILE *file = fopen(filename, "a");
        if (file == NULL) {
            printf("Error opening file %s\n", filename);
            return;
        }
        
        fprintf(file, "Price: %.2f, Volume: %.8f, Timestamp: %lld\n", price, volume, timestamp);
        fclose(file);
    }
}

void write_candlestick_to_file(const char* symbol, double open_price, double close_price, double min_price, double max_price, double total_volume) {
    char filename[50];
    if (strlen(symbol)!=0){
        snprintf(filename, sizeof(filename), "candlesticks/%s_candlestick.txt", symbol);
        
        FILE *file = fopen(filename, "a");
        if (file == NULL) {
            printf("Error opening file %s\n", filename);
            return;
        }
        fprintf(file, "Open: %.2f, Close: %.2f, Min: %.2f, Max: %.2f, Volume: %.8f\n", 
                open_price, close_price, min_price, max_price, total_volume);

        fclose(file);
    }
}

void write_avg_to_file(const char* symbol, double price_sum) {
    char filename[50];
    if (strlen(symbol)!=0){
        snprintf(filename, sizeof(filename), "averages/%s_avg.txt", symbol);
        
        FILE *file = fopen(filename, "a");
        if (file == NULL) {
            printf("Error opening file %s\n", filename);
            return;
        }
        fprintf(file, "Average: %.2f\n", price_sum);
        fclose(file);
    }
}

// Function to write delay times to a CSV file
void write_delay_to_file(const char* symbol, double delay) {
    char filename[50];
    if (strlen(symbol)!=0){
        snprintf(filename, sizeof(filename), "delays/%s_delays.csv", symbol);

        FILE *file = fopen(filename, "a");
        if (file == NULL) {
            printf("Error opening file %s\n", filename);
            return;
        }
        fprintf(file, "%.9f\n", delay);
        fclose(file);
    }
}

void clear_file(const char* filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening file %s to clear contents.\n", filename);
        return;
    }
    fclose(file);
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
//Producer and consumer functions
void *producer(void *arg){
    TradeData *in = (TradeData *)arg;
    queue *fifo;
    fifo = queues[in->thread_id];

    pthread_mutex_lock(fifo->mut);
    while (fifo->full && !destroy_flag) {
        pthread_cond_wait(fifo->notFull, fifo->mut);
    }
    // this in has to be specified
    queueAdd(fifo, *in);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notEmpty);

    return NULL;
}
void *consumer(void *arg){
    int thread_id = (intptr_t)arg;
    queue *fifo;
    fifo = queues[thread_id];
    
    // Idle time timer
    struct timespec idle_start, idle_end;

    while(!destroy_flag){
        pthread_mutex_lock(fifo->mut);
        while (fifo->empty && !destroy_flag) {
            // Start the idle timer
            clock_gettime(CLOCK_MONOTONIC, &idle_start);
            // Wait for the queue to be not empty
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
            // Stop the idle timer
            clock_gettime(CLOCK_MONOTONIC, &idle_end);
            idle_time[thread_id] += (idle_end.tv_sec - idle_start.tv_sec) + 
                         (idle_end.tv_nsec - idle_start.tv_nsec) / 1e9;
        }
        // Extract one trade from the queue
        queueDel(fifo, &trades[thread_id]);

        // Calculate the delay time between enqueuing and dequeuing the trade
        if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
            perror(KRED"Error getting the dequeue time"RESET);
            return NULL;
        }
        dequeue_time[thread_id] = end.tv_sec + end.tv_nsec / 1e9;
        delay[thread_id] = dequeue_time[thread_id] - trades[thread_id].enqueue_time;
        // Debugging print
        //printf(KCYN"Delay time for %s: %.9f\n"RESET, trades[thread_id].symbol, delay[thread_id]);
        write_delay_to_file(trades[thread_id].symbol, delay[thread_id]);

        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);

        // Print the trade data inside a txt file
        write_trade_to_file(trades[thread_id].symbol, trades[thread_id].price, trades[thread_id].volume, trades[thread_id].timestamp);
        
        current_time[thread_id] = time(NULL);

        // Update the candlestick data
        if (count[thread_id][minute[thread_id]] == 0) {
            open_price[thread_id] = trades[thread_id].price;
            min_price[thread_id] = trades[thread_id].price;
            max_price[thread_id] = trades[thread_id].price;
        }
        else {
            if (trades[thread_id].price < min_price[thread_id]) {
                min_price[thread_id] = trades[thread_id].price;
            }
            if (trades[thread_id].price > max_price[thread_id]) {
                max_price[thread_id] = trades[thread_id].price;
            }
        }
        total_volume[thread_id] += trades[thread_id].volume;
        
        // calulating the average price based on the previous average price
        // every minute has its own average price and we keep track of the last 15 minutes
        price_sum[thread_id][minute[thread_id]] += trades[thread_id].price; // price_avg*count=price1+price2+price3+...+priceN 
        count[thread_id][minute[thread_id]]++;
        close_price[thread_id] = trades[thread_id].price;

        if (difftime(current_time[thread_id], start_time[thread_id]) >= 5){
            // Write the candlestick data to a file
            write_candlestick_to_file(trades[thread_id].symbol, open_price[thread_id], close_price[thread_id], min_price[thread_id], max_price[thread_id], total_volume[thread_id]);
            
            // Calculate the moving average price
            for(int i=0;i<15;i++){
                avg[thread_id] += price_sum[thread_id][i];
                total_count[thread_id] += count[thread_id][i];
            }
            avg[thread_id] = total_count[thread_id] > 0 ? avg[thread_id] / total_count[thread_id] : 0;

            // Write the moving average price to a file
            write_avg_to_file(trades[thread_id].symbol, avg[thread_id]);

            // Reset the timer
            start_time[thread_id] = current_time[thread_id];

            // Reset candlestick data
            total_volume[thread_id] = 0;
            min_price[thread_id] = 0;
            max_price[thread_id] = 0;
            open_price[thread_id] = 0;
            close_price[thread_id] = 0;

            // Reset the moving average price and its counter
            minute[thread_id] = (minute[thread_id] + 1) % 15;
            price_sum[thread_id][minute[thread_id]] = 0;
            count[thread_id][minute[thread_id]] = 0;
            avg[thread_id] = 0;
            total_count[thread_id] = 0;

            // Clear the contents of the log files
            for (int i = 0; i < NUM_THREADS; i++) {
                char trade_filename[50];
                if (strlen(trades[i].symbol)!=0){
                    snprintf(trade_filename, sizeof(trade_filename), "logs/%s.txt", trades[i].symbol);
                    clear_file(trade_filename);
                }
            }
        }
        if(destroy_flag){
            break;
        }
    }
    return NULL;
}

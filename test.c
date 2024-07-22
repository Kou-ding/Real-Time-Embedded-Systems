#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libwebsockets.h>
#include <asm-generic/siginfo.h>
#include <libwebsockets/lws-misc.h>

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

// RX buffer size receiving the data from the websocket
#define EXAMPLE_RX_BUFFER_BYTES (100)

//flags to determine the state of the websocket
static int destroy_flag = 0; //destroy flag
static int connection_flag = 0; //connection flag
static int writeable_flag = 0; //writeable flag

// This function sets the destroy flag to 1 when the SIGINT signal (Ctr+C) is received
// This is used to close the websocket connection and free the memory
static void INT_HANDLER(int signo) {
    destroy_flag = 1;
};

// This struct is used to store the session data
// NOT USED INSIDE THE CODE!!!!!!!!
struct session_data {
    int fd;
};

// This struct is used to store the context and the websocket instance
struct pthread_routine_tool {
    struct lws_context *context;
    struct lws *wsi;
};

// This function sends a message to the websocket
// It take the websocket instance wsi_in, the message str and the size of the message str_size_in as parameters
//
static int websocket_write_back(struct lws *wsi_in, char *str, int str_size_in){
    if (str == NULL || wsi_in == NULL){
        return -1;
    }

    int n;
    int len;
    char *out = NULL;

    if (str_size_in < 1){
        len = strlen(str);
    } else {
        len = str_size_in;
    }

    out = (char *)malloc(sizeof(char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
    
    //Setting up the buffer
    memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, str, len );
    
    //Writing the buffer
    n = lws_write(wsi_in, out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);

    printf(KBLU"[websocket_write_back] %s\n"RESET, str);
    
    //Free the buffer
    free(out);

    return n;
};

//A callback function that handles different websocket events
static int ws_service_callback(struct lws *wsi, 
                                enum lws_callback_reasons reason, 
                                void *user,
                                void *in, size_t len){
    switch (reason) {
        //This case is called when the connection is established
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf(KYEL"[Main Service] Successful Client Connection.\n"RESET);
            connection_flag = 1;
	        lws_callback_on_writable(wsi);
            break;
        //This case is called when there is an error in the connection
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf(KRED"[Main Service] Client Connection Error: %s.\n"RESET, in);
            destroy_flag = 1;
            connection_flag = 0;
            break;
        //This case is called when the connection is closed
        case LWS_CALLBACK_CLOSED:
            printf(KYEL"[Main Service] Websocket connection closed\n"RESET);
            destroy_flag = 1;
            connection_flag = 0;
            break;
        //This case is called when the client receives a message from the websocket
        case LWS_CALLBACK_CLIENT_RECEIVE:
            printf(KCYN_L"[Main Service] The Client received a message:%s\n"RESET, (char *)in);
            break;
        
        //This case is called when the client is able to send a message to the websocket
        //It sends a subscription message to the websocket and sets the writeable flag to 1
        case LWS_CALLBACK_CLIENT_WRITEABLE :
            printf(KYEL"[Main Service] The websocket is writeable.\n"RESET);
            char* out = NULL;
            int sym_num = 2;
            char symb_arr[4][5] = {"APPL\0", "AMZN\0", "BINANCE:BTCUSDT\0", "IC MARKETS:1\0"};
            char str[50];

            for(int i = 0; i < 4; i++){
                sprintf(str, "{\"type\":\"subscribe\",\"symbol\":\"%s\"}", symb_arr[i]);
                printf(str);
                int len = strlen(str);
                
                out = (char *)malloc(sizeof(char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
                memcpy(out + LWS_SEND_BUFFER_PRE_PADDING, str, len);
                
                lws_write(wsi, out+LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);
            }
            free(out);
            writeable_flag = 1;
            break;
        //The default case just breaks the switch statement
        default:
            break;
    }

    return 0;
};

static struct lws_protocols protocols[] ={
	{
		"example-protocol",
		ws_service_callback,
		0,
		EXAMPLE_RX_BUFFER_BYTES,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

struct sigaction {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

int main(void){
    /* register the signal SIGINT handler */
    struct sigaction act;
    act.sa_handler = INT_HANDLER;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction( SIGINT, &act, 0);

    struct lws_context *context = NULL;
    struct lws_context_creation_info info;
    struct lws *wsi = NULL;
    struct lws_protocols protocol;

    memset(&info, 0, sizeof info);

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    //1st step: Send an http request to the server asking to open a connection
    //2nd step: The server sends a response to the client 101 Switching Protocols
    //Step 1 and 2 comprise the handshake process
    //Step 3: In our case, we ask for certain stack data and then the server sends that data to us
    char inputURL[300] = "ws.finnhub.io/?token=cqe0rvpr01qgmug3d06gcqe0rvpr01qgmug3d070";
    const char *urlProtocol, *urlTempPath;
	char urlPath[300];
    context = lws_create_context(&info);
    printf(KRED"[Main] context created.\n"RESET);

    if (context == NULL) {
        printf(KRED"[Main] context is NULL.\n"RESET);
        return -1;
    }
    struct lws_client_connect_info clientConnectionInfo;
    memset(&clientConnectionInfo, 0, sizeof(clientConnectionInfo));
    clientConnectionInfo.context = context;
    if (lws_parse_uri(inputURL, &urlProtocol, &clientConnectionInfo.address,
	    &clientConnectionInfo.port, &urlTempPath))
    {
	    printf("Couldn't parse URL\n");
    }

    urlPath[0] = '/';
    strncpy(urlPath + 1, urlTempPath, sizeof(urlPath) - 2);
    urlPath[sizeof(urlPath)-1] = '\0';
    clientConnectionInfo.port = 443;
    clientConnectionInfo.path = urlPath;
    clientConnectionInfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    
    clientConnectionInfo.host = clientConnectionInfo.address;
    clientConnectionInfo.origin = clientConnectionInfo.address;
    clientConnectionInfo.ietf_version_or_minus_one = -1;
    clientConnectionInfo.protocol = protocols[0].name;
    printf("Testing %s\n\n", clientConnectionInfo.address);
    printf("Connecting to %s://%s:%d%s \n\n", urlProtocol, 
    clientConnectionInfo.address, clientConnectionInfo.port, urlPath);

    wsi = lws_client_connect_via_info(&clientConnectionInfo);
    if (wsi == NULL) {
        printf(KRED"[Main] wsi create error.\n"RESET);
        return -1;
    }

    printf(KGRN"[Main] wsi create success.\n"RESET);

    while(!destroy_flag)
    {
        lws_service(context, 50);
    }

    lws_context_destroy(context);

    return 0;
}

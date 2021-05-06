/*
File:   RESTserver.hpp
Author: Hanson
Desc:   Define types and function prototypes of the REST server
*/

#define MG_ENABLE_SOCKETPAIR 1

#if !defined(_MSC_VER)
#include "mongoose.h"
#else
extern "C" {
#include "mongoose.h"
}
#endif

#include <map>
#include <string>

// handler type is for the server event handlers
typedef void (*handler)(mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data);

// For internal use only. Stores router info, including method and event handler
typedef struct _handlerInfo {
    std::string method;         // Empty string ("") means method will be ignored
    handler     eventHandler;   // Remember to check for NULL function pointers
} handlerInfo;

// handler_identifier can be used to remove router rules
typedef std::pair<std::map<std::string, handlerInfo>::iterator, bool> handler_identifier;

class RESTserver {
public:
    /*
    Function:   addHandler
    Desc:       Add a new rule into the router
    Args:       method: The request method. Case insensitive. e.g.: POST, GET
    Return:     A handler_identifier, which can be used to remove the rule with removeHandler()
    */
    handler_identifier addHandler(std::string method, std::string path, handler eventHandler);

    /*
    Function:   removeHandler
    Desc:       Remove a rule from the router
    Args:       identifier: A handler_identifier returned by addHandler
    WARNING:    You should not remove the same handler_identifier twice
    */
    void removeHandler(handler_identifier identifier);

    /*
    Function:   setDefaultHandler
    Desc:       Set the default handler for paths that doesn't have a corresponding router rule
    Args:       eventHandler: A handler function
    */
    void setDefaultHandler(handler eventHandler);

    /*
    Function:   removeDefaultHandler
    Desc:       Remove the default handler, use the built-in handler instead
    */
    void removeDefaultHandler();

    /*
    Function:   setWrongMethodHandler
    Desc:       Set the handler for requests that are sent with wrong methods
    Args:       eventHandler: A handler function
    */
    void setWrongMethodHandler(handler eventHandler);

    /*
    Function:   removeWrongMethodHandler
    Desc:       Remove the wrong method handler, use the built-in handler instead
    */
    void removeWrongMethodHandler();

    // For internal use only. Matches the provided method and path with the corresponding handler
    handler matchHandler(std::string method, std::string path);

    /*
    Function:   setPollHandler
    Desc:       Set the handler for poll event, which will be called periodically
    Args:       eventHandler: A handler function
    */
    void setPollHandler(handler pollHandler);

    /*
    Function:   removePollHandler
    Desc:       Remove the handler for poll event
    */
    void removePollHandler();

    // For internal use only. Obtain the handler function for poll event
    handler getPollHandler();
    
    /*
    Function:   startServer
    Desc:       Start the server
    Args:       connectionString: The address to listen. E.g. localhost:8000
    .           pollFrequency: The frequency to invoke poll event. In milliseconds
    .           userdata: A pointer to user-defined data
    WARNING:    This is a blocking operation. The function won't return until the server is stopped
    */
    void startServer(std::string connectionString, int pollFrequency, void *userdata);
    
    /*
    Function:   stopServer
    Desc:       Stop the server
    */
    void stopServer();

private:
    std::map<std::string, handlerInfo> router;
    handlerInfo defaultHandler = { "", (handler)NULL };
    handlerInfo wrongMethodHandler = { "", (handler)NULL };
    handlerInfo pollHandler = { "", (handler)NULL };

    // If the server is stopping
    bool stopping = false;
};

// For internal use only. Stores HTTP request dispatcher info, including pointer to current class and user-defined data
typedef struct _dispatcherInfo {
    RESTserver  *ptrToClass;
    void        *userdata;
} dispatcherInfo;

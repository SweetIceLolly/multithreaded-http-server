/*
File:   RESTserver.cpp
Author: Hanson
Desc:   Implement functionalities for the REST server
*/ 

#include "RESTserver.hpp"

static void builtInHandler(mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data);
static void httpRequestDispatch(struct mg_connection *connection, int ev, void *ev_data, void *fn_data);

/*
Function:   ucase
Desc:       Convert a string to uppercase
Args:       target: the string to be converted to uppercase
WARNING:    This will change the original string
*/
std::string ucase(std::string& target) {
    for (size_t i = 0; i < target.length(); i++) {
        target[i] &= ~32;
    }
    return target;
}

handler_identifier RESTserver::addHandler(std::string method, std::string path, handler eventHandler) {
    handlerInfo info;
    info.eventHandler = eventHandler;
    info.method = ucase(method);
    return this->router.insert(std::pair<std::string, handlerInfo>(path, info));
}

void RESTserver::removeHandler(handler_identifier identifier) {
    this->router.erase(identifier.first);
}

void RESTserver::setDefaultHandler(handler eventHandler) {
    this->defaultHandler = { "", eventHandler };
}

void RESTserver::removeDefaultHandler() {
    this->defaultHandler = { "", (handler)NULL };
}

void RESTserver::setPollHandler(handler pollHandler) {
    this->pollHandler = { "", pollHandler };
}

void RESTserver::removePollHandler() {
    this->pollHandler = { "", (handler)NULL };
}

/*
Function:   getPollHandler
Desc:       For internal use only. Obtain the handler function for poll event
Returnl:    A handler function that is guaranteed not NULL
*/
handler RESTserver::getPollHandler() {
    if (this->pollHandler.eventHandler) {
        return this->pollHandler.eventHandler;
    }
    else {
        // Return a function that does nothing
        return [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {};
    }
}

void RESTserver::setWrongMethodHandler(handler eventHandler) {
    this->wrongMethodHandler = { "", eventHandler };
}


void RESTserver::removeWrongMethodHandler() {
    this->wrongMethodHandler = { "", (handler)NULL };
}

void RESTserver::startServer(std::string connectionString, int pollFrequency, void *userdata) {
    struct mg_mgr mgr;
    dispatcherInfo info;

    info.ptrToClass = this;
    info.userdata = userdata;

    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, connectionString.c_str(), httpRequestDispatch, &info);

    for (;;) {
        if (this->stopping) {
            break;
        }
        mg_mgr_poll(&mgr, pollFrequency);
    }
    mg_mgr_free(&mgr);
}

void RESTserver::stopServer() {
    this->stopping = true;
}

/*
Function:   matchHandler
Desc:       For internal use only. Matches the provided method and path with the corresponding handler
Args:       method: Parsed method string from the HTTP message
.           path: Parsed path string from the HTTP message
Return:     A handler function that is guaranteed not NULL
*/
handler RESTserver::matchHandler(std::string method, std::string path) {
    auto info = this->router.find(path);        // Find corresponding request handler

    if (info != this->router.end()) {
        // There's a corresponding entry in the router
        if (info->second.method.at(0) == '\0' || ucase(method) == info->second.method) {
            // The request method matches
            return info->second.eventHandler;
        }
        else {
            // The request method does not match
            if (this->wrongMethodHandler.eventHandler) {
                // There's a user-set wrong method handler
                return this->wrongMethodHandler.eventHandler;
            }
            else {
                // No user-set wrong method handler, use the built-in function instead
                return builtInHandler;
            }
        }
    }
    else {
        // No corresponding entry in the router
        if (this->defaultHandler.eventHandler) {
            // There's a user-set default handler
            return this->defaultHandler.eventHandler;
        }
        else {
            // No user-set default handler, use the built-in function instead
            return builtInHandler;
        }
    }
}

/*
Function:   builtInHandler
Desc:       For internal use only. The built in handler to handle requests that don't have a corresponding handler
Args:       connection: Mongoose connection
.           ev: Event type
.           ev_data: Event data
.           fn_data: User-defined data
*/
static void builtInHandler(mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
    mg_printf(connection,
        "HTTP/1.1 404\r\n"
        "Content-Length: 9\r\n"
        "\r\n"
        "Not found"
    );
}

/*
Function:   httpRequestDispatch
Desc:       For internal use only. Dispatch requests to corresponding handlers
Args:       connection: Mongoose connection
.           ev: Event type
.           ev_data: Event data
.           fn_data: User-defined data. Here it will be a pointer to dispatcherInfo
*/
static void httpRequestDispatch(struct mg_connection *connection, int ev, void *ev_data, void *fn_data) {
    RESTserver *ptrToClass = ((dispatcherInfo *)fn_data)->ptrToClass;   // Obtain a pointer to the corresponding class

    if (ev == MG_EV_HTTP_MSG) {
        // Handle HTTP request
        struct mg_http_message *httpMsg = (struct mg_http_message *)ev_data;

        // Find a matching handler and call it
        auto handler = ptrToClass->matchHandler(
            std::string(httpMsg->method.ptr, httpMsg->method.len),
            std::string(httpMsg->uri.ptr, httpMsg->uri.len)
        );
        handler(connection, ev, (mg_http_message *)ev_data, fn_data);
    }
    else if (ev == MG_EV_POLL) {
        // Handle poll event
        auto handler = ptrToClass->getPollHandler();
        handler(connection, ev, (mg_http_message *)ev_data, fn_data);
    }
}

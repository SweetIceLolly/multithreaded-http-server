/*
File:   pickles.cpp
Author: Hanson
Desc:   Test the functionality of the server, thread pool, and the json library
*/ 

#include "RESTserver/RESTserver.hpp"
#include "ThreadPool/ThreadPool.hpp"
#include "utils/json.hpp"
#include <signal.h>

#if !defined(_MSC_VER)
#define _strdup strdup
#define closesocket close
#endif

RESTserver server;
ThreadPool threadPool;
using json = nlohmann::json;

typedef struct _response {
    char *data;
    int httpCode;
    char *headers;
} response;

typedef struct _calc_param {
    double  val;
    int     socket;
} calc_param;

static void handleCalc(void *calcParam) {
    calc_param *param = (calc_param*)calcParam;
    response res;
    res.data = _strdup(std::to_string(param->val + 10).c_str());
    res.headers = _strdup("");
    res.httpCode = 200;
    send(param->socket, (char *)&res, sizeof(res), 0);
    closesocket(param->socket);
}

static void handleJson(void *socket) {
    json j = {
        {"pi", 3.141},
        {"happy", true},
        {"name", "Niels"},
        {"nothing", nullptr},
        {"answer", {
            {"everything", 42}
        }},
        {"list", {1, 0, 2}},
        {"object", {
          {"currency", "USD"},
          {"value", 42.99}
        }}
    };
    response res;
    res.data = _strdup(j.dump().c_str());
    res.httpCode = 200;
    res.headers = _strdup("Content-Type: application/json\r\n");
    send((int)(long)socket, (char *)&res, sizeof(res), 0);
    closesocket((int)(long)socket);
}

int main() {
    server.setDefaultHandler(
        [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
            mg_http_reply(connection, 404, NULL, "API not found");
        }
    );

    server.setWrongMethodHandler(
        [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
            mg_http_reply(connection, 400, NULL, "Invalid request method");
        }
    );

    server.setPollHandler(
        [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
            if (connection->socketpair_socket != 0) {
                response res = { 0 };
                if (recv(connection->socketpair_socket, (char *)&res, sizeof(res), 0) == sizeof(res)) {
                    mg_http_reply(connection, res.httpCode, res.headers, res.data);
                    closesocket(connection->socketpair_socket);
                    connection->socketpair_socket = 0;
                    free(res.data);
                    free(res.headers);
                }
            }
        }
    );

    server.addHandler("GET", "/calc",
        [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
            char buf[10];
            int len = mg_http_get_var(&ev_data->query, "value", buf, 10);
            double n = atof(buf);
            int blocking = -1, non_blocking = -1;

            mg_socketpair(&blocking, &non_blocking);
            static calc_param param;
            param.val = n;
            param.socket = blocking;
            connection->socketpair_socket = non_blocking;
            threadPool.addJob(job(handleCalc, (void *)&param));
        }
    );

    server.addHandler("GET", "/hello",
        [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
            mg_http_reply(connection, 200, NULL, "Hello");
        }
    );

    server.addHandler("GET", "/testjson",
        [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
            int blocking = -1, non_blocking = -1;

            mg_socketpair(&blocking, &non_blocking);
            connection->socketpair_socket = non_blocking;
            threadPool.addJob(job(handleJson, (void *)blocking));
        }
    );

    server.addHandler("GET", "/wait",
        [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
            threadPool.waitForAllJobsDone();
            mg_http_reply(connection, 200, NULL, "Okay");
        }
    );

    signal(SIGINT,
        [](int signum) {
            printf("Exit signal caught! Shutting down...\n");
            server.stopServer();
        }
    );
    threadPool.init(8);
    server.startServer("localhost:8000", 50, NULL);
    threadPool.shutdown();
    printf("All clear! See you next time!\n");

    return 0;
}

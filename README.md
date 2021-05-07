# Pickles
Good performance multithreaded HTTP REST server written in C++

# Description
There are two major components in this tiny project: The HTTP server and the thread pool. Both of the components can function independently.

In the RESTserver folder: `mongoose.c` and `mongoose.h` are Cesanta Mongoose, with some minor modifications to make them suit for this project. The `RESTserver` class is the encapsulation of Mongoose using C++ classes, allowing router and handlers of different request methods.

In the ThreadPool folder: The ThreadPool class is the encapsulation of `std::thread` and provides the basic functionality of a thread pool.

# "Boast"

## Cross-platform
Compiled successfully on Windows using MSVC and on Linux using g++.

## Intuitive and Elegant
With just several lines, you can build your own REST server in C++!
```cpp
#include "RESTserver/RESTserver.hpp"
RESTserver server;
int main() {
    server.addHandler("GET", "/hello",
        [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
            mg_http_reply(connection, 200, NULL, "Hello world!");
        }
    );
    server.startServer("localhost:8000", 1000, NULL);
    return 0;
}
```
Compile with: `g++ -Wall -O2 example.cpp RESTserver/mongoose.c RESTserver/RESTserver.cpp -o example`

## Good Performance
I can't say it's high performance but the performance is not bad :D

Tested on my laptop:

OS: Deepin 20.2 Linux version 5.10.18-amd64-desktop

RAM: 8GB DDR4 2667MHz

CPU: Intel(R) Core(TM) i5-8265U @ 1.60GHz, 4 cores, 8 Threads

Benchmark software: Apache JMeter 5.4.1

### Test 1: Hello World example
This test uses the above compiled Hello World example.

In Apache JMeter, I used a thread group with 8 threads and infinite loops to request `localhost:8000/hello`. Below is the summary report when the test has been ran for 1 minute:
![image](https://user-images.githubusercontent.com/47358542/117364463-ec1ddf80-ae8b-11eb-9d95-0c89411e3992.png)
The Hello World example, with `-O2` optimization handled 4940000+ requests in a minute with average speed 82000+ requests per second. Below is the peak speed with 5 seconds test time:
![image](https://user-images.githubusercontent.com/47358542/117364685-369f5c00-ae8c-11eb-8150-271e56042bfd.png)
The server processed 510000+ requests in 5 seconds with peak speed reached 111000+ requests per second. This is not bad at all for a Hello World example :)

### Test 2: HTTP Requests with JSONs
This test is using the POST method to request and return JSONs. Below is the code used, compiled with `-O2` optimization.

```cpp
#include "RESTserver/RESTserver.hpp"
#include "utils/json.hpp"

RESTserver server;
using json = nlohmann::json;

int main() {
    server.addHandler("POST", "/json",
        [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
            char *buffer = new char[ev_data->body.len + 1];
            strncpy(buffer, ev_data->body.ptr, ev_data->body.len);
            buffer[ev_data->body.len] = '\0';
            json j = json::parse(buffer);
            int val = j.value("value", 0);
            json res = {
                { "plus", val + 20 },
                { "minus", val - 69 },
                { "plus_float", val + 3.14 },
                { "list", { val, val * 2, val * 3.14 } },
                { "object", {
                    { "item1", val },
                    { "item2", true },
                    { "item3", val - 3.14 },
                    { "anotherList", { val - 1, val - 2.5, "somestr", false, val } }
                }}
            };
            mg_http_reply(connection, 200, NULL, res.dump().c_str());
            delete buffer;
        }
    );

    server.startServer("localhost:8000", 1000, NULL);
    return 0;
}
```
The program first parses the requested json and obtain the `value` entry, do some arithmetics and form the new JSON data, then seralizes the new JSON data into string and respond. Data types included objects, lists, integers, floating point numbers, and boolean values. Here is a sample request and response:

(request)
```json
{
	"something": "unrelated",
	"someobject": {
		"useless": { "ha": 123 },
		"blah": false,
		"bepsu": [1, 2.5, false, {}]
	},
	"somelist": [[], {}, [123, 567], true, "sss", 1],
	"value": -54303,
	"someother": "things"
}
```

(response)
```json
{
   "list":[
      -54303,
      -108606,
      -170511.42
   ],
   "minus":-54372,
   "object":{
      "anotherList":[
         -54304,
         -54305.5,
         "somestr",
         false,
         -54303
      ],
      "item1":-54303,
      "item2":true,
      "item3":-54306.14
   },
   "plus":-54283,
   "plus_float":-54299.86
}
```
In Apache JMeter, I used a thread group with 8 threads and infinite loops to request `localhost:8000/json`. I added a random variable to generate random values for the `value` entry for every request. Below is the summary report when the test has been ran for 1 minute:
![image](https://user-images.githubusercontent.com/47358542/117379592-c56ca280-aea5-11eb-8f20-0cf97dbbd882.png)
The program handled 231000+ requests in a minute with average speed 38000+ requests per second. Below is the peak speed with 5 seconds test time:
![image](https://user-images.githubusercontent.com/47358542/117379779-3e6bfa00-aea6-11eb-8bf4-c69494e591e1.png)
The server processed 200000+ requests in 5 seconds with peak speed reached 44000+ requests per second.

## Low RAM Usage

If you code carefully, the server won't have any memory leak and maintain low RAM usage. The examples in this document seems doesn't have any memory leak.

- The Hello World example uses 144KB of RAM after it started up. After it has processed millions of requests and still processing requests at about 80000reqs/sec, it maintains RAM usage at 144KB.

- The JSON request example uses 144KB of RAM after it started up. After it has processed millions of requests and still processing requests at about 35000reqs/sec, it maintains RAM usage at 144KB.

- The thread pool with calculation-intensive tasks example uses 152KB of RAM after it started up. After it has processed hundreds of requests and still processing requests at about 8reqs/sec, it maintains RAM usage at 152KB.

# Weakness

## Bad Performance with Intensive Tasks in Threads
### Test 3: Thread Pool with Calculation-intensive Tasks
This test simulates calculation-intensive situations, in which the tasks will be carried out in alternate threads so that the main thread (which handles new requests) is not blocked. Below is the code used, compiled with `g++ -Wall -O2 tpexample.cpp RESTserver/mongoose.c RESTserver/RESTserver.cpp ThreadPool/ThreadPool.cpp -lpthread -o tpexample`.

```cpp
#include "RESTserver/RESTserver.hpp"
#include "ThreadPool/ThreadPool.hpp"
#include <math.h>

RESTserver server;
ThreadPool threadPool;

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
    double tmp;

    tmp = 0;
    for (int i = 0; i < 5000000; i++) {
        tmp += pow(-1, i) * pow(param->val, i + 1.0) / (i + 1.0);
    }

    res.data = strdup(std::to_string(tmp).c_str());
    res.headers = strdup("");
    res.httpCode = 200;
    send(param->socket, (char *)&res, sizeof(res), 0);
    close(param->socket);
}

int main() {
    server.setPollHandler(
        [](struct mg_connection *connection, int ev, mg_http_message *ev_data, void *fn_data) {
            if (connection->socketpair_socket != 0) {
                response res = { 0 };
                if (recv(connection->socketpair_socket, (char *)&res, sizeof(res), 0) == sizeof(res)) {
                    mg_http_reply(connection, res.httpCode, res.headers, res.data);
                    close(connection->socketpair_socket);
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

    threadPool.init(8);
    server.startServer("localhost:8000", 50, NULL);
    threadPool.shutdown();

    return 0;
}
```
The program uses socket pair technique (which comes together with Mongoose). When the main thread receives request, it will create a socket pair, pass the job parameters and the socket to the thread, and return. When the job is finished, the thread will store the response in a static variable (notice that the thread function `handleCalc` is static), send the address to the response data to the paried socket and return. In each poll event, the paired socket will check if there are finished requests and respond them to the client. The requests will be `GET` requests with a parameter `value`. The value `x` will be a number between -1 and 1. The calculation task is to calculate ![image](https://user-images.githubusercontent.com/47358542/117383688-5431ed00-aeaf-11eb-9776-6aab18ee0f68.png) , which will converge to `ln(x+1)`. With a quick decompilation, we can see the compiler didn't optimize the calculation:

![image](https://user-images.githubusercontent.com/47358542/117399840-6a4fa580-aecf-11eb-880c-b30331bcec4f.png)

Sample request URL: `http://localhost:8000/calc?value=0.605690`

Sample response: `0.473554`

In Apache JMeter, I used a thread group with 8 threads and infinite loops to request `localhost:8000/calc`. I added a random variable to generate random values for the `value` parameter for every request. The test has ran for 10 seconds and the result is quite disappointing:

![image](https://user-images.githubusercontent.com/47358542/117394838-e85a7f00-aec4-11eb-898f-36f31709638b.png)

The server handled 94 requests in a 10 seconds which is about 9 requests per second.

Why is it so slow? I found out the program didn't utilize the full CPU resource - the CPU utlization is only about 12%. This is unexpected because the use of thread pool is meant to utilize all CPU resource. I am not sure why this happens and will investigate on it.

## Not that Elegant
- Doesn't support URL regex match - since this program directly maps URL string to handler functions

- Unfriendly combination of thread pool and the server. As you can see, the above example of thread pool is not elegant. The user has to write a lot of additional code to make the thread pool function together with the server. This should be improved.

- A lot of code is written in C style without the use of C++ features. For example, the thread pool library can be more elegant. This is my first time implementing a thread pool, so don't be too hard on me :D

- Poor encapsulation of the server. A lot of function is not encapsulated the RESTserver class, resulted in the user have to call a lot of functions directly.

## Bad Performance on Windows Compared to Linux

When the same tests are carried out on Windows, the speed is much worse. The Hello World example reached about 6000reqs/sec, the JSON example reached about 3500reqs/sec. I believe this is due to the operating system difference in the underlying syscalls like socket, memory allocation, etc.

However, RAM usage remains good on Windows. These examples are able to maintain a stable RAM usage on Windows and seems doesn't have memory leaks.

## May Not be Capable as a Webpage Server

This program is intended to be used to implement an RESTful server to handle POST/GET requests. You may be able to serve simple webpages using this program, but I am not sure if it will work well.

# Conclusion

This server is expected to perform well in normal tasks (e.g. JSON parsing and serializing, database operations). It is also capable to handle long operations (e.g. operations that take 3 seconds) by putting them into the thread pool. However, if there's a lot of long operations (especially when they come with high-concurrency), this program could be overwhelmed and probably crash (rare but happened). It is worth to mention that, with the use of thread pool, this server can handle other requests while processing long requests. In other words, one request will not block other requests.

# Hopefully...

- I will perform more tests and make improvements (like exceptions handling) to make sure this server is robust

- I will try to improve the thread pool, and probably introduce a priority queue to prioritize jobs

- I will try to combine this server with database operations to see if it can perform well

- I will try to improve the flexibility of the RESTserver class (e.g. URL regex support)

- I will try to improve the encapsulation of the RESTserver class (e.g. have functions to use socket pair, extraction of request arguments, extraction of request body, etc.)

# Thanks
- Cesanta Mongoose, the heart and soul of this project! https://github.com/cesanta/mongoose
- nlohmann's json library for C++, provides fast and memory-efficient JSON functionalities: https://github.com/nlohmann/json
- John's Blog - Thread pool in C, helped me implement the thread pool: https://nachtimwald.com/2019/04/12/thread-pool-in-c/

#include "ProxyServer.h"

int main() {
    ProxyServer server(8888);

    if(!server.start()) {
        return 1;            // Abnormal termination OR Error Occured
    }

    return 0;                // Successful Execution
}
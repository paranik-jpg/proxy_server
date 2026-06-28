#include "ProxyServer.h"

int main() {
    ProxyServer server(8888);
    if(!server.start()) {
        return 1;
    }    
    return 0;
}
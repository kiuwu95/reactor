#include"include/Client.h"

static Client clt;

int main() {
    if (!clt.ConnectServer()) {
        perror("ConnectServer");
        return -1;
    }
    clt.GetMessage();
}
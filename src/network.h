#include <stddef.h>

#ifndef NETWORK_H
#define NETWORK_H

void send_data(const char *server_address, int port, const void *data, size_t size);
void receive_data(int port, void **data, size_t *size);

#endif // NETWORK_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <time.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define LOG_FILE "bluetooth_log.bin"

typedef struct {
    uint64_t timestamp_ms;
    uint16_t length;
    uint8_t data[BUFFER_SIZE];
} log_entry_t;

// Enumerate all paired devices
int enumerate_devices(bdaddr_t **device_list, char ***device_names) {
    inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, i;
    int count = 0;

    dev_id = hci_get_route(NULL);
    if (dev_id < 0) {
        perror("hci_get_route");
        return -1;
    }

    sock = hci_open_dev(dev_id);
    if (sock < 0) {
        perror("hci_open_dev");
        return -1;
    }

    max_rsp = 255;
    ii = (inquiry_info *)malloc(max_rsp * sizeof(inquiry_info));
    if (!ii) {
        close(sock);
        return -1;
    }

    num_rsp = hci_inquiry(dev_id, 8, max_rsp, NULL, &ii, IREQ_CACHE_FLUSH);
    if (num_rsp < 0) {
        perror("hci_inquiry");
        free(ii);
        close(sock);
        return -1;
    }

    *device_list = (bdaddr_t *)malloc(num_rsp * sizeof(bdaddr_t));
    *device_names = (char **)malloc(num_rsp * sizeof(char *));

    if (!*device_list || !*device_names) {
        free(ii);
        close(sock);
        return -1;
    }

    for (i = 0; i < num_rsp; i++) {
        char name[248] = { 0 };
        char addr[18] = { 0 };

        ba2str(&(ii + i)->bdaddr, addr);
        memcpy(&(*device_list)[i], &(ii + i)->bdaddr, sizeof(bdaddr_t));

        // Try to get device name
        if (hci_read_remote_name(sock, &(ii + i)->bdaddr, sizeof(name), name, 0) < 0)
            strcpy(name, "Unknown");

        (*device_names)[i] = (char *)malloc(strlen(name) + 1);
        strcpy((*device_names)[i], name);

        printf("[%d] %s - %s\n", i, addr, name);
        count++;
    }

    free(ii);
    close(sock);
    return count;
}

int connect_rfcomm(const bdaddr_t *device_addr, int channel) {
    struct sockaddr_rc addr = { 0 };
    int sock;

    sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    memcpy(&addr.rc_bdaddr, device_addr, sizeof(bdaddr_t));
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = channel;

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    char addr_str[18];
    ba2str(device_addr, addr_str);
    printf("Connected to %s on channel %d\n", addr_str, channel);
    return sock;
}

void log_data(FILE *fp, uint64_t start_time, const uint8_t *data, size_t length) {
    uint64_t now = (uint64_t)(time(NULL) * 1000);
    uint64_t elapsed = now - start_time;

    log_entry_t entry;
    entry.timestamp_ms = elapsed;
    entry.length = (uint16_t)(length > 65535 ? 65535 : length);
    memcpy(entry.data, data, entry.length);

    fwrite(&entry.timestamp_ms, sizeof(uint64_t), 1, fp);
    fwrite(&entry.length, sizeof(uint16_t), 1, fp);
    fwrite(entry.data, 1, entry.length, fp);
    fflush(fp);
}

int main(int argc, char *argv[]) {
    bdaddr_t *device_list;
    char **device_names;
    int num_devices, choice, channel;

    num_devices = enumerate_devices(&device_list, &device_names);
    if (num_devices <= 0) {
        fprintf(stderr, "No devices found.\n");
        return 1;
    }

    printf("\nSelect device to log from (0-%d): ", num_devices - 1);
    scanf("%d", &choice);

    if (choice < 0 || choice >= num_devices) {
        fprintf(stderr, "Invalid choice.\n");
        return 1;
    }

    printf("Enter RFCOMM channel (default 1): ");
    if (scanf("%d", &channel) != 1) {
        channel = 1;
    }

    int sock = connect_rfcomm(&device_list[choice], channel);
    if (sock < 0) {
        return 1;
    }

    FILE *log_fp = fopen(LOG_FILE, "wb");
    if (!log_fp) {
        perror("fopen");
        close(sock);
        return 1;
    }

    uint64_t start_time = (uint64_t)(time(NULL) * 1000);
    uint8_t buffer[BUFFER_SIZE];
    int bytes;

    printf("Logging to %s. Press Ctrl+C to stop.\n", LOG_FILE);

    while (1) {
        bytes = read(sock, buffer, sizeof(buffer));
        if (bytes < 0) {
            perror("read");
            break;
        } else if (bytes == 0) {
            printf("Device disconnected.\n");
            break;
        }

        log_data(log_fp, start_time, buffer, bytes);
        printf("Logged %d bytes\n", bytes);
    }

    fclose(log_fp);
    close(sock);
    printf("Logging complete. Data saved to %s\n", LOG_FILE);

    // Cleanup
    for (int i = 0; i < num_devices; i++) {
        free(device_names[i]);
    }
    free(device_names);
    free(device_list);

    return 0;
}

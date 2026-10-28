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

// Scan for BLE devices
int scan_ble_devices(bdaddr_t **device_list, char ***device_names) {
    int dev_id, sock, count = 0;
    struct hci_request rq;
    le_set_scan_parameters_cp scan_params;
    le_set_scan_enable_cp scan_enable;
    uint8_t status;

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

    // Set scan parameters
    memset(&scan_params, 0, sizeof(scan_params));
    scan_params.type = 0x01;           // Active scan
    scan_params.interval = htobs(0x0010);
    scan_params.window = htobs(0x0010);
    scan_params.own_bdaddr_type = 0x00;
    scan_params.filter = 0x00;

    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_SCAN_PARAMETERS;
    rq.cparam = &scan_params;
    rq.clen = LE_SET_SCAN_PARAMETERS_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;

    if (hci_send_req(sock, &rq, 1000) < 0) {
        perror("hci_send_req (scan params)");
        close(sock);
        return -1;
    }

    // Enable scan
    memset(&scan_enable, 0, sizeof(scan_enable));
    scan_enable.enable = 0x01;
    scan_enable.filter_dup = 0x00;

    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_SCAN_ENABLE;
    rq.cparam = &scan_enable;
    rq.clen = LE_SET_SCAN_ENABLE_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;

    if (hci_send_req(sock, &rq, 1000) < 0) {
        perror("hci_send_req (scan enable)");
        close(sock);
        return -1;
    }

    printf("Scanning for BLE devices for 5 seconds...\n");
    
    // Listen for advertising packets for 5 seconds
    // This is simplified; a real implementation would parse HCI events
    sleep(5);

    close(sock);
    return count;
}

// Fallback: use bluetoothctl to list devices
int enumerate_devices_bluetoothctl(bdaddr_t **device_list, char ***device_names) {
    FILE *fp = popen("bluetoothctl devices", "r");
    if (!fp) {
        perror("popen");
        return -1;
    }

    char line[256];
    int count = 0;
    int capacity = 10;

    *device_list = (bdaddr_t *)malloc(capacity * sizeof(bdaddr_t));
    *device_names = (char **)malloc(capacity * sizeof(char *));

    if (!*device_list || !*device_names) {
        pclose(fp);
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char addr_str[18];
        char name[256];

        // Parse: "Device 00:11:22:33:44:55 My Device Name"
        if (sscanf(line, "Device %17s %255[^\n]", addr_str, name) == 2) {
            if (count >= capacity) {
                capacity *= 2;
                *device_list = (bdaddr_t *)realloc(*device_list, capacity * sizeof(bdaddr_t));
                *device_names = (char **)realloc(*device_names, capacity * sizeof(char *));
            }

            str2ba(addr_str, &(*device_list)[count]);
            (*device_names)[count] = (char *)malloc(strlen(name) + 1);
            strcpy((*device_names)[count], name);

            printf("[%d] %s - %s\n", count, addr_str, name);
            count++;
        }
    }

    pclose(fp);
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

    // Use bluetoothctl as fallback (works for both Classic and BLE)
    printf("Enumerating paired devices...\n");
    num_devices = enumerate_devices_bluetoothctl(&device_list, &device_names);

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

    for (int i = 0; i < num_devices; i++) {
        free(device_names[i]);
    }
    free(device_names);
    free(device_list);

    return 0;
}

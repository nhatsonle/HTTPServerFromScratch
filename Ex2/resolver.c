#define _DEFAULT_SOURCE // For strdup on some systems
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define LOG_FILE "resolver.log"
#define INPUT_BUFFER_SIZE 2048

// --- Khai báo các hàm ---

void run_interactive_mode();
void run_batch_mode(const char *filename);
void process_line(char *line);
void process_query(const char *query);
void resolve_ip(const char *ip_str, double *query_time);
void resolve_hostname(const char *hostname, double *query_time);
void log_entry(const char *query, const char *result);
void check_special_ip(const struct sockaddr *sa);

// --- Hàm Main ---

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // Chế độ tương tác nếu không có tham số
        run_interactive_mode();
    } else if (argc == 2) {
        // Chế độ batch nếu có một tham số là tên file
        run_batch_mode(argv[1]);
    } else {
        fprintf(stderr, "Cú pháp:\n");
        fprintf(stderr, "  Chế độ tương tác: %s\n", argv[0]);
        fprintf(stderr, "  Chế độ batch:     %s <tên file>\n", argv[0]);
        return 1;
    }
    return 0;
}

// --- Các chế độ chạy ---

void run_interactive_mode() {
    char line[INPUT_BUFFER_SIZE];
    printf("Chế độ phân giải tương tác. Nhập chuỗi rỗng để thoát.\n");
    while (1) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL || line[0] == '\n') {
            break;
        }
        process_line(line);
    }
    printf("Đã thoát.\n");
}

void run_batch_mode(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Không thể mở file");
        exit(1);
    }
    char line[INPUT_BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        process_line(line);
    }
    fclose(file);
}

// --- Xử lý đầu vào ---

/**
 * @brief Xử lý một dòng đầu vào, có thể chứa nhiều truy vấn cách nhau bởi dấu cách.
 */
void process_line(char *line) {
    // Xóa ký tự xuống dòng ở cuối
    line[strcspn(line, "\r\n")] = 0;
    
    // Dùng strtok để tách các truy vấn
    char *query = strtok(line, " \t");
    while (query != NULL) {
        if (strlen(query) > 0) {
            process_query(query);
        }
        query = strtok(NULL, " \t");
    }
}

/**
 * @brief Hàm trung tâm xử lý một truy vấn duy nhất.
 */
void process_query(const char *query) {
    struct sockaddr_storage sa_storage;
    double query_time = 0.0;
    
    printf("\n--- Truy vấn: %s ---\n", query);

    // Thử phân tích chuỗi như một địa chỉ IP (cả v4 và v6)
    if (inet_pton(AF_INET, query, &((struct sockaddr_in *)&sa_storage)->sin_addr) == 1) {
        ((struct sockaddr_in *)&sa_storage)->sin_family = AF_INET;
        resolve_ip(query, &query_time);
    } else if (inet_pton(AF_INET6, query, &((struct sockaddr_in6 *)&sa_storage)->sin6_addr) == 1) {
        ((struct sockaddr_in6 *)&sa_storage)->sin6_family = AF_INET6;
        resolve_ip(query, &query_time);
    } else {
        // Nếu không phải là IP, coi nó là tên miền
        resolve_hostname(query, &query_time);
    }
    printf("Thời gian truy vấn: %.4f ms\n", query_time * 1000.0);
    printf("--------------------------\n");
}


// --- Các hàm phân giải ---

void resolve_ip(const char *ip_str, double *query_time) {
    struct sockaddr_storage sa_storage;
    socklen_t sa_len;

    // Dựng lại cấu trúc sockaddr từ chuỗi IP
    if (strchr(ip_str, ':')) { // IPv6
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&sa_storage;
        addr6->sin6_family = AF_INET6;
        inet_pton(AF_INET6, ip_str, &addr6->sin6_addr);
        sa_len = sizeof(struct sockaddr_in6);
    } else { // IPv4
        struct sockaddr_in *addr4 = (struct sockaddr_in *)&sa_storage;
        addr4->sin_family = AF_INET;
        inet_pton(AF_INET, ip_str, &addr4->sin_addr);
        sa_len = sizeof(struct sockaddr_in);
    }

    check_special_ip((struct sockaddr *)&sa_storage);

    char hostname[NI_MAXHOST];
    char result_log[INPUT_BUFFER_SIZE] = "";

    clock_t start = clock();
    int res = getnameinfo((struct sockaddr *)&sa_storage, sa_len, hostname, NI_MAXHOST, NULL, 0, 0);
    clock_t end = clock();
    *query_time = ((double)(end - start)) / CLOCKS_PER_SEC;

    if (res != 0) {
        printf("Kết quả: Không tìm thấy thông tin.\n");
        snprintf(result_log, sizeof(result_log), "Not found information.");
    } else {
        printf("Tên miền chính thức: %s\n", hostname);
        snprintf(result_log, sizeof(result_log), "Official name: %s", hostname);
    }
    log_entry(ip_str, result_log);
}

void resolve_hostname(const char *hostname, double *query_time) {
    struct addrinfo hints, *result, *p;
    char ip_str[INET6_ADDRSTRLEN];
    char result_log[INPUT_BUFFER_SIZE] = "";
    char temp_log[INET6_ADDRSTRLEN + 2];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // Hỗ trợ cả IPv4 và IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME; // Yêu cầu lấy tên chuẩn (CNAME)

    clock_t start = clock();
    int status = getaddrinfo(hostname, NULL, &hints, &result);
    clock_t end = clock();
    *query_time = ((double)(end - start)) / CLOCKS_PER_SEC;

    if (status != 0) {
        printf("Kết quả: Không tìm thấy thông tin.\n");
        snprintf(result_log, sizeof(result_log), "Not found information.");
    } else {
        // In CNAME nếu có
        if (result->ai_canonname) {
            printf("Tên chuẩn (CNAME): %s\n", result->ai_canonname);
            snprintf(result_log, sizeof(result_log), "CNAME: %s | ", result->ai_canonname);
        }

        int first_ip = 1;
        for (p = result; p != NULL; p = p->ai_next) {
            void *addr;
            if (p->ai_family == AF_INET) { // IPv4
                addr = &((struct sockaddr_in *)p->ai_addr)->sin_addr;
            } else { // IPv6
                addr = &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
            }
            inet_ntop(p->ai_family, addr, ip_str, sizeof(ip_str));

            if (first_ip) {
                printf("IP chính thức: %s\n", ip_str);
                snprintf(temp_log, sizeof(temp_log), "Official IP: %s", ip_str);
                strncat(result_log, temp_log, sizeof(result_log) - strlen(result_log) - 1);
                if(p->ai_next) printf("IP bí danh:\n");
                first_ip = 0;
            } else {
                printf("  %s\n", ip_str);
                snprintf(temp_log, sizeof(temp_log), ", %s", ip_str);
                 strncat(result_log, temp_log, sizeof(result_log) - strlen(result_log) - 1);
            }
        }
        freeaddrinfo(result);
    }
    log_entry(hostname, result_log);
}

// --- Các hàm tiện ích ---

void log_entry(const char *query, const char *result) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (!log_file) return;

    time_t now = time(NULL);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log_file, "[%s] QUERY: \"%s\" -> RESULT: \"%s\"\n", time_str, query, result);
    fclose(log_file);
}

void check_special_ip(const struct sockaddr *sa) {
    char warning[] = "Cảnh báo: special IP address — may not have DNS record";
    
    if (sa->sa_family == AF_INET) {
        const struct sockaddr_in *addr4 = (const struct sockaddr_in *)sa;
        const unsigned char *bytes = (const unsigned char *)&addr4->sin_addr;
        // 127.0.0.0/8 (loopback)
        if (bytes[0] == 127) { printf("%s\n", warning); return; }
        // 10.0.0.0/8 (private)
        if (bytes[0] == 10) { printf("%s\n", warning); return; }
        // 172.16.0.0/12 (private)
        if (bytes[0] == 172 && (bytes[1] >= 16 && bytes[1] <= 31)) { printf("%s\n", warning); return; }
        // 192.168.0.0/16 (private)
        if (bytes[0] == 192 && bytes[1] == 168) { printf("%s\n", warning); return; }

    } else if (sa->sa_family == AF_INET6) {
        const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *)sa;
        const unsigned char *bytes = (const unsigned char *)&addr6->sin6_addr;
        // ::1 (loopback)
        if (IN6_IS_ADDR_LOOPBACK(&addr6->sin6_addr)) { printf("%s\n", warning); return; }
        // fc00::/7 (Unique Local Address - private)
        if ((bytes[0] & 0xFE) == 0xFC) { printf("%s\n", warning); return; }
    }
}
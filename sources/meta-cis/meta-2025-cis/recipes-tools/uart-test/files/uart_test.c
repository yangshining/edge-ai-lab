#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#define SERIAL_PORT "/dev/ttyS1"
#define BAUDRATE B9600

volatile sig_atomic_t stop_flag = 0;

void sigint_handler(int signum) {
    stop_flag = 1;
}

int main() {
    int fd;
    struct termios options;
    char buffer[256];
    char input[256];

    fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    tcgetattr(fd, &options);
    cfsetispeed(&options, BAUDRATE);
    cfsetospeed(&options, BAUDRATE);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    tcsetattr(fd, TCSANOW, &options);

    signal(SIGINT, sigint_handler);

    printf("Enter data to send (Ctrl+C to exit):\n");

    while (!stop_flag) {
        printf("Input: ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; 
        }

        input[strcspn(input, "\n")] = '\0';

        ssize_t nbytes = write(fd, input, strlen(input));
        if (nbytes == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        nbytes = read(fd, buffer, sizeof(buffer));
        if (nbytes > 0) {
            buffer[nbytes] = '\0';
            printf("Received %ld bytes from serial port: %s\n", (long)nbytes, buffer);
        }
    }

    close(fd);

    printf("Program terminated.\n");

    return 0;
}


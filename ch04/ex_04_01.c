#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "tlpi_hdr.h"

int main(int argc, char *argv[]) {
    // Check cmd line
    if (argc < 2 || (argc == 3 && strcmp(argv[1], "-a") != 0) || argc > 3) {
        usageErr("%s [-a] FILE\n", argv[0]);
    }
    
    // Open output file
    const char *fout_name = NULL;
    int flags = O_WRONLY | O_CREAT;
    switch (argc) {
        case 2:
            fout_name = argv[1];
            flags |= O_TRUNC;
            break;
        case 3:
            fout_name = argv[2];
            flags |= O_APPEND;
            break;
        default:
            fatal("This point is unreachable");
            break;
    }
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd_out = open(fout_name, flags, mode);
    if (fd_out == -1) {
        errExit("opening FILE (%s)", fout_name);
    }

    // Emulate tee
    const int MAX_READ = 4096;
    char buf[MAX_READ];
    int num_read;
    while ((num_read = read(STDIN_FILENO, buf, MAX_READ)) > 0) {
        int num_write;

        num_write = write(STDOUT_FILENO, buf, num_read);
        if (num_write == -1) {
            errExit("writing STDOUT_FILENO");
        }
        if (num_write != num_read) {
            fatal("incomplete write to STDOUT_FILENO");
        }

        num_write = write(fd_out, buf, num_read);
        if (num_write == -1) {
            errExit("writing FILE (%s)", fout_name);
        }
        if (num_write != num_read) {
            fatal("incomplete write to FILE (%s)", fout_name);
        }
    }

    if (num_read == -1) {
        errExit("reading STDIN_FILENO");
    }

    // Close files
    if (close(fd_out) == -1) {
        errExit("closing FILE (%s)", fout_name);
    }

	return EXIT_SUCCESS;
}

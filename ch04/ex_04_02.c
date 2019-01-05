// The program has the following issues:
// - If the file ends with null bytes, the program fails to write them.
//   The reason for that is the following: lseek() does not perform
//   any writing, while the final write() writes null bytes 
// - The reliable_write() is actually not reliable, as it does not
//   handle the case of an incomplete write
// - The detection of holes is very naive: it treats each single
//   null byte as a hole

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "tlpi_hdr.h"

void reliable_write(const char *fname, int fd, const char *buf, int num);

// Actually, it is not reliable
void reliable_write(const char *fname, int fd, const char *buf, int num) {
    int num_write = write(fd, buf, num);

    if (num_write == -1) {
        errExit("writing DEST (%s)", fname);
    }

    if (num_write != num) {
        errExit("incomplete writing DEST (%s)", fname);
    }
}

int main(int argc, char *argv[]) {
    // Check cmd line
    if (argc != 3) {
        usageErr("%s SOURCE DEST\n", argv[0]);
    }

    // Open source file
    const char *finp_name = argv[1];
    int fd_inp = open(finp_name, O_RDONLY);
    if (fd_inp == -1) {
        errExit("opening SOURCE (%s)", finp_name);
    }

    // Create dest file
    const char *fout_name = argv[2];
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd_out = open(fout_name, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd_out == -1) {
        errExit("opening DEST (%s)", fout_name);
    }

    // Copy source file to dest file
    for (;;) {
        // Read from source file
        const int BUF_SIZE = 4096;
        char buf_inp[BUF_SIZE];
        int num_read = read(fd_inp, buf_inp, BUF_SIZE);
        
        if (num_read == -1) {
            errExit("reading SOURCE (%s)", finp_name);
        }

        if (num_read == 0) {
            break;
        }

        // Write to dest file
        // A naive approach: treat each zero as a hole, but write/move in batches
        char buf_out[BUF_SIZE];
        int out_size = 0;
        for (int inp_idx = 0; inp_idx < num_read; ++inp_idx) {
            if (buf_inp[inp_idx] != '\0') {
                buf_out[out_size++] = buf_inp[inp_idx];
            } else {
                // Flush buffer
                reliable_write(fout_name, fd_out, buf_out, out_size);
                out_size = 0;

                // Count zeros
                int num_zeros = 1;
                for (++inp_idx; inp_idx < num_read; ++inp_idx) {
                    if (buf_inp[inp_idx] == '\0') {
                        ++num_zeros;
                    } else {
                        --inp_idx;
                        break;
                    }
                }
                
                // Move file pointer
                if (lseek(fd_out, num_zeros, SEEK_CUR) == (off_t) -1) {
                    errExit("repositioning DEST (%s)", fout_name);
                }
            }
        }

        // Write the remainder of a buffer
        reliable_write(fout_name, fd_out, buf_out, out_size);
    }
    
    // Close files
    if (close(fd_inp) != 0) {
        errExit("closing SOURCE (%s)", finp_name);
    }

    if (close(fd_out) != 0) {
        errExit("closing DEST (%s)", fout_name);
    }

    return EXIT_SUCCESS;
}

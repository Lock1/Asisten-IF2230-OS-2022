#include "header/kernel.h"
#include "header/filesystem.h"
#include "header/std_datatype.h"
#include "header/std_opr.h"


int main() {
    makeInterrupt21();
    clearScreen();

    while (true);
}

void clearScreen() {
    // Bonus : Menggunakan interrupt
    // interrupt(0x10, 0x0003, 0, 0, 0);

    // Solusi naif, kosongkan buffer
    unsigned int i = 0;
    for (i = 0; i < 10000; i += 2) {
        putInMemory(0xB000, 0x8000 + i, '\0');   // Kosongkan char buffer
        putInMemory(0xB000, 0x8001 + i,  0xF);   // Ganti color ke white (0xF)
    }

    interrupt(0x10, 0x0200, 0, 0, 0x0000);
}

void printString(char *string) {
    int i = 0, AX = 0;

    // Ulangi proses penulisan karakter hingga ditemukan null terminator
    while (string[i] != 0x00) {
        AX = 0x0E00 | string[i];
        interrupt(0x10, AX, 0x000F, 0, 0);
        i++;
    }
}

void readString(char *string) {
    char singleCharBuffer;
    int currentIndex = 0;
    int AXoutput;

    // Ulangi pembacaan hingga ditemukan karakter '\r' atau carriage return
    // Tombol enter menghasilkan '\r' dari pembacaan INT 16h
    do {
        singleCharBuffer = (char) interrupt(0x16, 0x0000, 0, 0, 0);

        if (singleCharBuffer != '\r') {
            // Menuliskan karakter ke layar
            AXoutput = 0x0E00 | singleCharBuffer;
            interrupt(0x10, AXoutput, 0x000F, 0, 0);

            string[currentIndex] = singleCharBuffer;
            currentIndex++;
        }
        else
            printString("\r\n");

    } while (singleCharBuffer != '\r');
}



void readSector(byte *buffer, int sector_number) {
    int sector_read_count = 0x01;
    int cylinder, sector;
    int head, drive;

    cylinder = div(sector_number, 36) << 8; // CL
    sector   = mod(sector_number, 18) + 1;  // CH

    head  = mod(div(sector_number, 18), 2) << 8; // DH
    drive = 0x00; // DL

    interrupt(
        0x13, // Interrupt number
        0x0200 | sector_read_count, // AX
        buffer, // BX
        cylinder | sector, // CX
        head | drive // DX
    );
}

void writeSector(byte *buffer, int sector_number) {
    int sector_write_count = 0x01;
    int cylinder, sector;
    int head, drive;

    cylinder = div(sector_number, 36) << 8; // CL
    sector   = mod(sector_number, 18) + 1;  // CH

    head  = mod(div(sector_number, 18), 2) << 8; // DH
    drive = 0x00; // DL

    interrupt(
        0x13, // Interrupt number
        0x0300 | sector_write_count, // AX
        buffer, // BX
        cylinder | sector, // CX
        head | drive // DX
    );
}

void write(byte *buffer, char *node_name, enum fs_retcode *return_code, byte parent_index) {

}

void read(byte *buffer, char *node_name, enum fs_retcode *return_code, byte parent_index) {
    struct node_filesystem node_fs_buffer;
    struct node_entry node_buffer;
    bool filename_match_found;
    int i;

    // TODO : Test garbage or not
    readSector(&(node_fs_buffer.nodes[0]),  FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Iterasi seluruh node
    filename_match_found = false;
    for (i = 0; i < 64 && !filename_match_found; i++) {
        memcpy(node_buffer, node_fs_buffer.nodes[i], sizeof(struct node_entry));

        if (node_buffer.parent_node_index == parent_index && !strcmp(node_buffer.name, node_name))
            filename_match_found = true;
    }

    // Fetch data atau exit code
    if (filename_match_found) {
        if (node_buffer.sector_entry_index == FS_NODE_S_IDX_FOLDER)
            *return_code = FS_R_TYPE_IS_FOLDER;
        else {
            struct sector_filesystem sector_fs_buffer;
            struct sector_entry sector_entry_buffer;
            readSector(&(sector_fs_buffer.sector_list[0]), FS_SECTOR_SECTOR_NUMBER);

            memcpy(
                sector_entry_buffer,
                sector_fs_buffer.sector_list[node_buffer.sector_entry_index],
                sizeof(struct sector_entry)
                );

            for (i = 0; i < 16; i++) {
                byte sector_number_to_read = sector_entry_buffer.sector_numbers[i];
                if (sector_number_to_read != 0x00)
                    readSector(buffer + i*512, sector_number_to_read);
                else
                    break; // Sector_number == 0 -> Tidak valid, selesaikan pembacaan
            }
        }
    }
    else
        *return_code = FS_R_NODE_NOT_FOUND;
}







void handleInterrupt21(int AX, int BX, int CX, int DX) {
    switch (AX) {
        case 0x0:
            printString(BX);
            break;
        case 0x1:
            readString(BX);
            break;
        default:
            printString("Invalid Interrupt");
    }
}



// DEBUG
// int strlen(char *string) {
//     int i = 0;
//     while (string[i] != '\0')
//         i++;
//     return i;
// }
//
// void strrev(char *string) {
//     int i = 0, length = strlen(string);
//     char temp;
//     while (i < length/2) {
//         temp = string[i];
//         string[i] = string[length - 1 - i];
//         string[length - 1 - i] = temp;
//         i++;
//     }
// }
//
// void inttostr(char *buffer, int n) {
//     int i = 0;
//     bool is_negative = false;
//     if (n < 0) {
//         n *= -1;
//         is_negative = true;
//     }
//     while (n > 10) {
//         buffer[i] = '0' + mod(n, 10);
//         i++;
//         n /= 10;
//     }
//     buffer[i] = '0' + mod(n, 10); // First digit
//     i++;
//     if (is_negative) {
//         buffer[i] = '-';
//         i++;
//     }
//     buffer[i] = '\0';
//     strrev(buffer);
// }

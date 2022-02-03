#include "header/kernel.h"
#include "header/filesystem.h"
#include "header/std_datatype.h"
#include "header/std_opr.h"


int main() {
    struct file_metadata data;
    byte ok[1024];
    enum fs_retcode ret;
    fillKernelMap();
    makeInterrupt21();
    clearScreen();

    clear(ok, 1024);
    readSector(ok, 0);
    readSector(ok+512, 1);
    data.buffer = ok;
    data.node_name = "test";
    data.parent_index = FS_NODE_P_IDX_ROOT;
    data.filesize = 740;
    write(&data, &ret);

    while (true);
}

void fillKernelMap() {
    struct map_filesystem map_fs_buffer;
    int i;

    readSector(&map_fs_buffer, FS_MAP_SECTOR_NUMBER);
    for (i = 0; i < 16; i++)
        map_fs_buffer.is_filled[i] = true;

    writeSector(&map_fs_buffer, FS_MAP_SECTOR_NUMBER);
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

void write(struct file_metadata *metadata, enum fs_retcode *return_code) {
    struct node_filesystem node_fs_buffer;
    struct map_filesystem map_fs_buffer;
    struct sector_filesystem sector_fs_buffer;
    struct node_entry node_buffer;
    bool node_write_index_found, sector_write_index_found;
    bool writing_file, enough_empty_space;
    bool unique_filename, invalid_parent_index;
    unsigned int node_write_index, sector_write_index;
    unsigned int empty_space_size;
    int i;

    // Tahap 1 : Pengecekan pada filesystem node
    readSector(&(node_fs_buffer.nodes[0]),  FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    unique_filename        = true;
    node_write_index_found = false;
    for (i = 0; i < 64 && unique_filename; i++) {
        memcpy(&node_buffer, &(node_fs_buffer.nodes[i]), sizeof(struct node_entry));

        // Cari dan simpan index yang berisikan node kosong pada filesystem node
        if (node_buffer.sector_entry_index == 0x00
              && node_buffer.parent_node_index == 0x00
              && !node_write_index_found) {
            node_write_index  = i;
            node_write_index_found = true;
        }

        // Validasi nama node
        if (node_buffer.parent_node_index == metadata->parent_index && !strcmp(node_buffer.name, metadata->node_name))
            unique_filename = false;
    }

    // Tahap 2 : Pengecekan parent index
    invalid_parent_index = false;
    if (metadata->parent_index != FS_NODE_P_IDX_ROOT) {
        if (node_fs_buffer.nodes[metadata->parent_index].sector_entry_index == FS_NODE_S_IDX_FOLDER)
            invalid_parent_index = true;
    }

    // Tahap 3 : Pengecekan tipe penulisan
    if (metadata->filesize != 0)
        writing_file = true;
    else
        writing_file = false;

    // Tahap 4 : Pengecekan ukuran untuk file
    if (writing_file) {
        readSector(map_fs_buffer.is_filled, FS_MAP_SECTOR_NUMBER);
        enough_empty_space = false;
        empty_space_size   = 0;

        // Catatan : Meskipun map dapat digunakan sebagai penanda 512 sektor,
        //       hanya 0-255 dapat diakses dengan 1 byte sector_number pada sector
        for (i = 0; i < 256 && !enough_empty_space; i++) {
            if (!map_fs_buffer.is_filled[i])
                empty_space_size += 512;

            if (metadata->filesize <= empty_space_size)
                enough_empty_space = true;
        }
    }
    else
        enough_empty_space = true; // Jika folder abaikan tahap ini

    // Tahap 5 : Pengecekan filesystem sector
    if (writing_file) {
        readSector(sector_fs_buffer.sector_list, FS_SECTOR_SECTOR_NUMBER);
        sector_write_index_found = false;

        for (i = 0; i < 64 && !sector_write_index_found; i++) {
            struct sector_entry sector_entry_buffer;
            bool is_sector_entry_empty;
            int j;

            memcpy(&sector_entry_buffer, &(sector_fs_buffer.sector_list[i]), sizeof(struct sector_entry));

            is_sector_entry_empty = true;
            for (j = 0; j < 16; j++) {
                if (sector_entry_buffer.sector_numbers[j] != 0x00)
                    is_sector_entry_empty = false;
            }

            if (is_sector_entry_empty) {
                sector_write_index       = i;
                sector_write_index_found = true;
            }
        }
    }
    else
        sector_write_index_found = true;


    // Tahap 6 : Penulisan
    if (node_write_index_found
          && sector_write_index_found
          && unique_filename
          && !invalid_parent_index
          && enough_empty_space) {

        node_fs_buffer.nodes[node_write_index].parent_node_index = metadata->parent_index; // Penulisan byte "P"
        strcpy(node_fs_buffer.nodes[node_write_index].name, metadata->node_name);          // Penulisan nama node

        // Menuliskan folder / file
        if (!writing_file)
            node_fs_buffer.nodes[node_write_index].sector_entry_index = FS_NODE_S_IDX_FOLDER;
        else {
            bool writing_completed = false;
            unsigned int written_filesize = 0;
            int j = 0;
            node_fs_buffer.nodes[node_write_index].sector_entry_index = sector_write_index;

            for (i = 0; i < 256 && !writing_completed; i++) {
                if (!map_fs_buffer.is_filled[i]) {
                    map_fs_buffer.is_filled[i] = true;
                    written_filesize += 512;

                    sector_fs_buffer.sector_list[sector_write_index].sector_numbers[j] = i;

                    writeSector(metadata->buffer + j*512, i);
                    j++;
                }

                if (written_filesize >= metadata->filesize)
                    writing_completed = true;
            }
        }

        // Update filesystem
        writeSector(&(node_fs_buffer.nodes[0]), FS_NODE_SECTOR_NUMBER);
        writeSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
        if (writing_file) {
            writeSector(&(map_fs_buffer), FS_MAP_SECTOR_NUMBER);
            writeSector(&(sector_fs_buffer), FS_SECTOR_SECTOR_NUMBER);
        }
    }

    // Tahap 7 : Return code
    if (!unique_filename)
        *return_code = FS_W_FILE_ALREADY_EXIST;
    else if (!node_write_index_found)
        *return_code = FS_W_MAXIMUM_NODE_ENTRY;
    else if (writing_file && !enough_empty_space)
        *return_code = FS_W_MAXIMUM_SECTOR_ENTRY;
    else if (invalid_parent_index)
        *return_code = FS_W_INVALID_FOLDER;
    else
        *return_code = FS_SUCCESS;
}

void read(struct file_metadata *metadata, enum fs_retcode *return_code) {
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
        memcpy(&node_buffer, &(node_fs_buffer.nodes[i]), sizeof(struct node_entry));

        // Pastikan parent index dan nama sama
        if (node_buffer.parent_node_index == metadata->parent_index && !strcmp(node_buffer.name, metadata->node_name))
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
                &sector_entry_buffer,
                &(sector_fs_buffer.sector_list[node_buffer.sector_entry_index]),
                sizeof(struct sector_entry)
                );

            for (i = 0; i < 16; i++) {
                byte sector_number_to_read = sector_entry_buffer.sector_numbers[i];
                if (sector_number_to_read != 0x00)
                    readSector(metadata->buffer + i*512, sector_number_to_read);
                else
                    break; // Sector_number == 0 -> Tidak valid, selesaikan pembacaan
            }

            *return_code = FS_SUCCESS;
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

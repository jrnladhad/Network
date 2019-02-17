#include <stdio.h>

int main(int argc, char* argv[]) {
    if(argc < 3){
        printf("Too few arguments!");
        return 0;
    }

    // Opening source file.
    FILE *fsrc = fopen(argv[1], "rb");
    if(fsrc == NULL){
        // Handling error while opening the file
        perror("Error: ");
        return -1;
    }

    // Opening destination file.
    FILE *fout = fopen(argv[2], "wb");
    if(fout == NULL){
        // Handling error while opening the file
        perror("Error: ");
        return -1;
    }

    // Temporary storage for reading from source file and writing to destination file.
    char buffer[10];
    size_t n, m;

    do{
        n = fread(buffer, 1, sizeof(buffer), fsrc);
        if(n)
            m = fwrite(buffer, 1, n, fout);
        else
            m = 0;
    }while((n > 0) && (n == m));
    
    // Closing both the files.  
    fclose(fsrc);
    fclose(fout);

    return 0;
}
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#define DEFAULT_PORT_BASE 0x70
#define MAX_NTAG_BYTES 8
#define MAX_LINE_LEN 256
#define MAX_PATH_LEN 260

unsigned int PORT_BASE = DEFAULT_PORT_BASE;

void getPath(const char *fullpath, char *path) {
    char *lastSlash;
    strcpy(path, fullpath);
    lastSlash = strrchr(path, '\\');
    if (lastSlash != NULL) {
        *lastSlash = '\0';
    }
}

void readNtag(char* ntag) {
    unsigned int port = PORT_BASE;
    unsigned char byteRead;
    unsigned int ntagIndex = 0;

    for (ntagIndex = 0; ntagIndex < MAX_NTAG_BYTES; ntagIndex++) {
        outp(port, 0x40 + ntagIndex);
        byteRead = inp(port + 1);

        if (byteRead == 0x00) break;

        ntag[ntagIndex] = byteRead;
    }

    ntag[ntagIndex] = '\0';
}

void invertNtag(char* ntag) {
   unsigned int len = strlen(ntag);
   unsigned int i;
   char temp;

   for (i = 0; i < len / 2; i++) {
       temp = ntag[i];
       ntag[i] = ntag[len - i - 1];
       ntag[len - i - 1] = temp;
   }
}

void getConfigFilePath(char *configFilePath, char *executablePath) {
    char *lastSlash;

    strcpy(configFilePath, executablePath);
    lastSlash = strrchr(configFilePath, '\\');
    if (lastSlash != NULL) {
        *(lastSlash + 1) = '\0';
    } else {
        strcpy(configFilePath, "");
    }
    strcat(configFilePath, "launcher.cfg");
}

int main(int argc, char *argv[]) {
    char ntag[MAX_NTAG_BYTES + 1];
    FILE *file;
    FILE *exec;
    char line[MAX_LINE_LEN];
    char *fileNtag, *pathExec;
    char configFilePath[MAX_PATH_LEN];
    char pathFolder[MAX_PATH_LEN];

    exec = fopen("LAUNCHER.BAT", "w");
    if (exec == NULL) {
        perror("Error opening/creating LAUNCHER.BAT");
        return 1;
    }
    fclose(exec);

    if (argc > 1) {
        PORT_BASE = (unsigned int)strtol(argv[1], NULL, 0);
    }

    getConfigFilePath(configFilePath, argv[0]);

    readNtag(ntag);
    invertNtag(ntag);

    file = fopen(configFilePath, "r");
    if (file == NULL) {
        printf("Error opening %s\n", configFilePath);
        return 1;
    }

    if (strlen(ntag) == 0) {
	return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = 0;
        fileNtag = strtok(line, ",");
        pathExec = strtok(NULL, "\n");
        getPath(pathExec, pathFolder);

        if (fileNtag != NULL && pathExec != NULL && strcmp(ntag, fileNtag) == 0) {
            fclose(file);
            if (strcmp(pathExec, "SYSTEM") == 0) {
                exit(0);
            }

            printf("Loading %s...\n", pathExec);

            exec = fopen("LAUNCHER.BAT", "w");

            if (exec == NULL) {
                perror("Error writing launcher.bat");
                return 1;
            }

            fprintf(exec, "@ECHO OFF\n");
            fprintf(exec, "CD %s\n", pathFolder);
            fprintf(exec, "%s\n", pathExec);
            fclose(exec);


            exit(0);
        }
    }
    printf("Launcher: AppId %s is not registered\n", ntag);

    fclose(file);
    return 1;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "easyosdir.h"

#define MAX_FILENAME_LENGTH 256
#define MAX_FILE_SIZE 1024

char current_directory[MAX_FILENAME_LENGTH] = "/";
char log_buffer[MAX_FILE_SIZE] = ""; // 로그 데이터를 저장할 버퍼

// 파일 복사 함수
int copy_file(const char *source_path, const char *destination_path) {
    FILE *source_file = fopen(source_path, "rb");
    if (source_file == NULL) {
        return 0; // 복사 실패
    }

    FILE *destination_file = fopen(destination_path, "wb");
    if (destination_file == NULL) {
        fclose(source_file);
        return 0; // 복사 실패
    }

    char buffer[MAX_FILE_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0) {
        fwrite(buffer, 1, bytes_read, destination_file);
    }

    fclose(source_file);
    fclose(destination_file);
    return 1; // 복사 성공
}

// 파일 또는 디렉토리 복사 함수 (재귀적)
int copy_file_or_directory(const char *source_path, const char *destination_path) {
    struct stat source_stat;
    if (stat(source_path, &source_stat) != 0) {
        return 0; // 복사 실패
    }

    if (S_ISDIR(source_stat.st_mode)) { // 디렉토리 복사
        if (!easyosdir_create(destination_path, FILE_TYPE_DIRECTORY)) {
            return 0; // 디렉토리 생성 실패
        }

        DIR *source_dir = opendir(source_path);
        if (source_dir == NULL) {
            return 0; // 디렉토리 열기 실패
        }

        struct dirent *entry;
        while ((entry = readdir(source_dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char sub_source_path[MAX_FILENAME_LENGTH];
            snprintf(sub_source_path, sizeof(sub_source_path), "%s/%s", source_path, entry->d_name);

            char sub_destination_path[MAX_FILENAME_LENGTH];
            snprintf(sub_destination_path, sizeof(sub_destination_path), "%s/%s", destination_path, entry->d_name);

            if (!copy_file_or_directory(sub_source_path, sub_destination_path)) {
                return 0; // 복사 실패
            }
        }

        closedir(source_dir);
        return 1; // 디렉토리 복사 성공
    } else { // 파일 복사
        return copy_file(source_path, destination_path);
    }
}

// 파일 삭제 함수
int delete_file(const char *path) {
    if (easyosdir_remove(path)) {
        return 1; // 삭제 성공
    } else {
        return 0; // 삭제 실패
    }
}

// 로그 데이터를 파일에서 읽어오는 함수
void load_log_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        char line[MAX_FILE_SIZE];
        while (fgets(line, sizeof(line), file) != NULL) {
            strcat(log_buffer, line);
        }
        fclose(file);
    } else {
        perror("파일 열기 실패");
    }
}

// 로그 데이터를 버퍼에 저장하는 함수
void save_log_to_buffer(const char *log_data) {
    if (strlen(log_buffer) + strlen(log_data) < MAX_FILE_SIZE) {
        strcat(log_buffer, log_data);
    }
}

// 버퍼에 저장된 로그 데이터를 파일에 저장하는 함수
int save_buffer_to_file(const char *filename) {
    FILE *file = fopen(filename, "a");
    if (file != NULL) {
        fprintf(file, "%s\n", log_buffer);
        fclose(file);
        return 1; // 저장 성공
    } else {
        perror("파일 열기 실패");
        return 0; // 저장 실패
    }
}

// "run_bin" 명령어 처리
int run_binary(const char *bin_name) {
    char full_path[MAX_FILENAME_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", current_directory, bin_name);

    char *argv[] = { bin_name, NULL };
    if (easyosdir_execute_bin(full_path, argv)) {
        printf("바이너리 파일 실행 성공\n");
        save_log_to_buffer("run_bin 명령을 실행했습니다.");
        return 1;
    } else {
        printf("바이너리 파일 실행 실패\n");
        save_log_to_buffer("run_bin 명령을 실행하는 도중 오류가 발생했습니다.");
        return 0;
    }
}

int main() {
    char command[256];

    // 파일 시스템 초기화
    easyosdir_init();

    // 로그 데이터 파일에서 읽어오기
    load_log_from_file("log.txt");

    // 네트워크 초기화
    if (network_init() < 0) {
        perror("네트워크 초기화 실패");
        return 1;
    }

    while (1) {
        printf("ZpC%s> ", current_directory);
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0'; // 개행 문자 제거

        if (strncmp(command, "copy", 4) == 0) {
            // "copy" 명령 처리
            char source[MAX_FILENAME_LENGTH];
            char destination[MAX_FILENAME_LENGTH];
            if (sscanf(command, "copy %s %s", source, destination) == 2) {
                if (copy_file_or_directory(source, destination)) {
                    printf("복사 완료\n");
                    save_log_to_buffer("copy 명령을 실행했습니다.");
                } else {
                    printf("복사 실패\n");
                    save_log_to_buffer("copy 명령을 실행하는 도중 오류가 발생했습니다.");
                }
            } else {
                printf("사용법: copy [원본 경로]“);
        } else if (strncmp(command, "paste", 5) == 0) {
            // "paste" 명령 처리
            char source[MAX_FILENAME_LENGTH];
            char destination[MAX_FILENAME_LENGTH];
            if (sscanf(command, "paste %s %s", source, destination) == 2) {
                if (paste(source, destination)) {
                    save_log_to_buffer("paste 명령을 실행했습니다.");
                } else {
                    save_log_to_buffer("paste 명령을 실행하는 도중 오류가 발생했습니다.");
                }
            } else {
                printf("사용법: paste [원본 경로] [대상 경로]\n");
            }
        } else if (strncmp(command, "cut", 3) == 0) {
            // "cut" 명령 처리
            char source[MAX_FILENAME_LENGTH];
            char destination[MAX_FILENAME_LENGTH];
            if (sscanf(command, "cut %s %s", source, destination) == 2) {
                if (cut(source, destination)) {
                    save_log_to_buffer("cut 명령을 실행했습니다.");
                } else {
                    save_log_to_buffer("cut 명령을 실행하는 도중 오류가 발생했습니다.");
                }
            } else {
                printf("사용법: cut [원본 경로] [대상 경로]\n");
            }
        } else if (strncmp(command, "delete", 6) == 0) {
            // "delete" 명령 처리
            char target[MAX_FILENAME_LENGTH];
            if (sscanf(command, "delete %s", target) == 1) {
                if (delete_file(target)) {
                    printf("삭제 완료\n");
                    save_log_to_buffer("delete 명령을 실행했습니다.");
                } else {
                    printf("삭제 실패\n");
                    save_log_to_buffer("delete 명령을 실행하는 도중 오류가 발생했습니다.");
                }
            } else {
                printf("사용법: delete [대상 경로]\n");
            }
        } else if (strncmp(command, "run_bin", 7) == 0) {
            // "run_bin" 명령 처리
            char bin_name[MAX_FILENAME_LENGTH];
            if (sscanf(command, "run_bin %s", bin_name) == 1) {
                if (run_binary(bin_name)) {
                    save_log_to_buffer("run_bin 명령을 실행했습니다.");
                } else {
                    save_log_to_buffer("run_bin 명령을 실행하는 도중 오류가 발생했습니다.");
                }
            } else {
                printf("사용법: run_bin [바이너리 파일 이름]\n");
            }
        } else if (strncmp(command, "save", 4) == 0) {
            // "save" 명령 처리 (로그 데이터 저장)
            char filename[MAX_FILENAME_LENGTH];
            if (sscanf(command, "save %s", filename) == 1) {
                if (save_buffer_to_file(filename)) {
                    printf("로그 데이터를 저장했습니다.\n");
                } else {
                    printf("로그 데이터 저장 실패\n");
                }
            } else {
                printf("사용법: save [파일 이름]\n");
            }
        } else if (strncmp(command, "list", 4) == 0) {
            // "list" 명령 처리 (디렉토리 내 파일 목록 출력)
            char dir[MAX_FILENAME_LENGTH];
            if (sscanf(command, "list %s", dir) == 1) {
                easyosdir_list(dir);
            } else {
                printf("사용법: list [디렉토리 경로]\n");
            }
        } else if (strncmp(command, "cd", 2) == 0) {
            // "cd" 명령 처리 (현재 디렉토리 변경)
            char new_dir[MAX_FILENAME_LENGTH];
            if (sscanf(command, "cd %s", new_dir) == 1) {
                if (chdir(new_dir) == 0) {
                    strcpy(current_directory, new_dir);
                } else {
                    printf("디렉토리 변경 실패\n");
                }
            } else {
                printf("사용법: cd [디렉토리 경로]\n");
            }
        } else if (strncmp(command, "help", 4) == 0) {
            // "help" 명령 처리 (도움말 표시)
            printf("사용 가능한 명령어:\n");
            printf("copy [원본 경로] [대상 경로]: 파일 또는 디렉토리 복사\n");
            printf("paste [원본 경로] [대상 경로]: 붙여넣기\n");
            printf("cut [원본 경로] [대상 경로]: 잘라내기\n");
            printf("delete [대상 경로]: 파일 또는 디렉토리 삭제\n");
            printf("run_bin [바이너리 파일 이름]: 바이너리 파일 실행\n");
            printf("save [파일 이름]: 로그 데이터 저장\n");
            printf("list [디렉토리 경로]: 디렉토리 내 파일 목록 출력\n");
            printf("cd [디렉토리 경로]: 현재 디렉토리 변경\n");
            printf("help: 도움말 표시\n");
        } else {
            // 이전의 다른 명령어 처리 코드를 유지합니다.
            // 이 곳에 추가적인 명령어 처리 코드를 계속하여 작성할 수 있습니다.
            printf("지원하지 않는 명령어입니다.\n");
        }
    }

    

    return 0;
}

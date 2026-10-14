#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>

#define MAX_PATH_LENGTH 260
#define ID_DIR_NAME_EDIT 101
#define ID_FILE_NAME_EDIT 102
#define ID_FILE_CONTENT_EDIT 103
#define ID_CREATE_DIR 104
#define ID_CREATE_FILE 105
#define ID_WRITE_FILE 106
#define ID_READ_FILE 107
#define ID_DELETE_FILE 108
#define ID_DELETE_DIR 109
#define ID_ENTER_DIR 110
#define ID_EXIT_DIR 111
#define ID_LOG_CHECK 112
#define ID_EXIT 113
#define ID_DIR_LIST 114
#define ID_FILE_LIST 115

typedef struct {
    int inode_count;
    int block_count;
} Superblock;

typedef struct {
    char name[MAX_PATH_LENGTH];
    int size;
    int is_directory;
} Inode;

typedef struct {
    Superblock superblock;
    Inode inodes[1024];
} FileSystem;

FileSystem fs;
char currentPath[MAX_PATH_LENGTH] = ".";

HWND hwndDirNameEdit, hwndFileNameEdit, hwndFileContentEdit;
HWND hwndDirList, hwndFileList;

void init_filesystem() {
    fs.superblock.inode_count = 0;
    fs.superblock.block_count = 0;
}

void create_inode(const char *name, int is_directory) {
    Inode *inode = &fs.inodes[fs.superblock.inode_count++];
    strncpy(inode->name, name, sizeof(inode->name));
    inode->size = 0;
    inode->is_directory = is_directory;
}

void delete_inode(const char *name) {
    for (int i = 0; i < fs.superblock.inode_count; i++) {
        if (strcmp(fs.inodes[i].name, name) == 0) {
            for (int j = i; j < fs.superblock.inode_count - 1; j++) {
                fs.inodes[j] = fs.inodes[j + 1];
            }
            fs.superblock.inode_count--;
            break;
        }
    }
}

void create_directory(const char *path) {
    if (CreateDirectory(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        create_inode(path, 1);
    } else {
        char errorMessage[MAX_PATH_LENGTH];
        snprintf(errorMessage, sizeof(errorMessage), "Failed to create directory: %d", GetLastError());
        MessageBox(NULL, errorMessage, "Error", MB_OK | MB_ICONERROR);
    }
}

void create_file(const char *path) {
    FILE *file = fopen(path, "w");
    if (file) {
        fclose(file);
        create_inode(path, 0);
    } else {
        char errorMessage[MAX_PATH_LENGTH];
        snprintf(errorMessage, sizeof(errorMessage), "Failed to create file: %d", GetLastError());
        MessageBox(NULL, errorMessage, "Error", MB_OK | MB_ICONERROR);
    }
}

void write_file(const char *path, const char *content) {
    FILE *file = fopen(path, "a");
    if (file) {
        fprintf(file, "%s", content);
        fclose(file);
    } else {
        char errorMessage[MAX_PATH_LENGTH];
        snprintf(errorMessage, sizeof(errorMessage), "Failed to write to file: %d", GetLastError());
        MessageBox(NULL, errorMessage, "Error", MB_OK | MB_ICONERROR);
    }
}

void read_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (file) {
        char buffer[1024];
        fread(buffer, 1, sizeof(buffer) - 1, file);
        buffer[sizeof(buffer) - 1] = '\0';
        MessageBox(NULL, buffer, "File Content", MB_OK | MB_ICONINFORMATION);
        fclose(file);
    } else {
        char errorMessage[MAX_PATH_LENGTH];
        snprintf(errorMessage, sizeof(errorMessage), "Failed to read file: %d", GetLastError());
        MessageBox(NULL, errorMessage, "Error", MB_OK | MB_ICONERROR);
    }
}

void delete_file(const char *path) {
    if (DeleteFile(path)) {
        delete_inode(path);
    } else {
        char errorMessage[MAX_PATH_LENGTH];
        snprintf(errorMessage, sizeof(errorMessage), "Failed to delete file: %d", GetLastError());
        MessageBox(NULL, errorMessage, "Error", MB_OK | MB_ICONERROR);
    }
}

void delete_directory(const char *path) {
    if (RemoveDirectory(path)) {
        delete_inode(path);
    } else {
        char errorMessage[MAX_PATH_LENGTH];
        snprintf(errorMessage, sizeof(errorMessage), "Failed to delete directory: %d", GetLastError());
        MessageBox(NULL, errorMessage, "Error", MB_OK | MB_ICONERROR);
    }
}

void log_event(const char *event) {
    FILE *log_file = fopen("log.txt", "a");
    if (log_file) {
        fprintf(log_file, "%s\n", event);
        fclose(log_file);
    }
}

void list_directories(HWND hwndListBox) {
    SendMessage(hwndListBox, LB_RESETCONTENT, 0, 0);
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile("*", &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
                SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)findFileData.cFileName);
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    FindClose(hFind);
}

void list_files(HWND hwndListBox) {
    SendMessage(hwndListBox, LB_RESETCONTENT, 0, 0);
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile("*", &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)findFileData.cFileName);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    FindClose(hFind);
}

void show_error(const char *message) {
    MessageBox(NULL, message, "Error", MB_OK | MB_ICONERROR);
}

void display_log_content() {
    FILE *log_file = fopen("log.txt", "r");
    if (!log_file) {
        MessageBox(NULL, "Failed to open log file", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    char log_content[8192];
    size_t len = fread(log_content, 1, sizeof(log_content) - 1, log_file);
    log_content[len] = '\0';
    fclose(log_file);

    MessageBox(NULL, log_content, "Log Content", MB_OK | MB_ICONINFORMATION);
}

void prompt_directory(char *dir) {
    BROWSEINFO bi = {0};
    bi.lpszTitle = "Choose a directory";
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != 0) {
        SHGetPathFromIDList(pidl, dir);
        IMalloc *imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->lpVtbl->Free(imalloc, pidl);
            imalloc->lpVtbl->Release(imalloc);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindow("static", "Directory Name:", WS_VISIBLE | WS_CHILD, 10, 10, 100, 20, hwnd, NULL, NULL, NULL);
            hwndDirNameEdit = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 120, 10, 200, 20, hwnd, (HMENU)ID_DIR_NAME_EDIT, NULL, NULL);

            CreateWindow("static", "File Name:", WS_VISIBLE | WS_CHILD, 10, 40, 100, 20, hwnd, NULL, NULL, NULL);
            hwndFileNameEdit = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 120, 40, 200, 20, hwnd, (HMENU)ID_FILE_NAME_EDIT, NULL, NULL);

            CreateWindow("static", "File Content:", WS_VISIBLE | WS_CHILD, 10, 70, 100, 20, hwnd, NULL, NULL, NULL);
            hwndFileContentEdit = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 120, 70, 200, 60, hwnd, (HMENU)ID_FILE_CONTENT_EDIT, NULL, NULL);

            CreateWindow("button", "Create Directory", WS_VISIBLE | WS_CHILD, 10, 140, 150, 30, hwnd, (HMENU)ID_CREATE_DIR, NULL, NULL);
            CreateWindow("button", "Create File", WS_VISIBLE | WS_CHILD, 170, 140, 150, 30, hwnd, (HMENU)ID_CREATE_FILE, NULL, NULL);
            CreateWindow("button", "Write to File", WS_VISIBLE | WS_CHILD, 10, 180, 150, 30, hwnd, (HMENU)ID_WRITE_FILE, NULL, NULL);
            CreateWindow("button", "Read File", WS_VISIBLE | WS_CHILD, 170, 180, 150, 30, hwnd, (HMENU)ID_READ_FILE, NULL, NULL);
            CreateWindow("button", "Delete File", WS_VISIBLE | WS_CHILD, 10, 220, 150, 30, hwnd, (HMENU)ID_DELETE_FILE, NULL, NULL);
            CreateWindow("button", "Delete Directory", WS_VISIBLE | WS_CHILD, 170, 220, 150, 30, hwnd, (HMENU)ID_DELETE_DIR, NULL, NULL);
            CreateWindow("button", "Enter Directory", WS_VISIBLE | WS_CHILD, 10, 260, 150, 30, hwnd, (HMENU)ID_ENTER_DIR, NULL, NULL);
            CreateWindow("button", "Exit Directory", WS_VISIBLE | WS_CHILD, 170, 260, 150, 30, hwnd, (HMENU)ID_EXIT_DIR, NULL, NULL);
            CreateWindow("button", "Check Log", WS_VISIBLE | WS_CHILD, 10, 300, 150, 30, hwnd, (HMENU)ID_LOG_CHECK, NULL, NULL);
            CreateWindow("button", "Exit", WS_VISIBLE | WS_CHILD, 170, 300, 150, 30, hwnd, (HMENU)ID_EXIT, NULL, NULL);

            CreateWindow("static", "Directories:", WS_VISIBLE | WS_CHILD, 350, 10, 100, 20, hwnd, NULL, NULL, NULL);
            hwndDirList = CreateWindow("listbox", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 350, 30, 200, 100, hwnd, (HMENU)ID_DIR_LIST, NULL, NULL);

            CreateWindow("static", "Files:", WS_VISIBLE | WS_CHILD, 350, 140, 100, 20, hwnd, NULL, NULL, NULL);
            hwndFileList = CreateWindow("listbox", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 350, 160, 200, 100, hwnd, (HMENU)ID_FILE_LIST, NULL, NULL);

            list_directories(hwndDirList);
            list_files(hwndFileList);
        }
        break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_CREATE_DIR: {
                    char dirPath[MAX_PATH_LENGTH];
                    prompt_directory(dirPath);

                    char dirName[MAX_PATH_LENGTH];
                    GetWindowText(hwndDirNameEdit, dirName, sizeof(dirName));

                    char fullPath[MAX_PATH_LENGTH];
                    snprintf(fullPath, sizeof(fullPath), "%s\\%s", dirPath, dirName);
                    create_directory(fullPath);
                    list_directories(hwndDirList);
                    log_event("Created directory");
                }
                break;

                case ID_CREATE_FILE: {
                    char dirPath[MAX_PATH_LENGTH];
                    prompt_directory(dirPath);

                    char fileName[MAX_PATH_LENGTH];
                    GetWindowText(hwndFileNameEdit, fileName, sizeof(fileName));

                    char fullPath[MAX_PATH_LENGTH];
                    snprintf(fullPath, sizeof(fullPath), "%s\\%s", dirPath, fileName);
                    create_file(fullPath);
                    list_files(hwndFileList);
                    log_event("Created file");
                }
                break;

                case ID_WRITE_FILE: {
                    char dirPath[MAX_PATH_LENGTH];
                    prompt_directory(dirPath);

                    char fileName[MAX_PATH_LENGTH];
                    GetWindowText(hwndFileNameEdit, fileName, sizeof(fileName));

                    char content[1024];
                    GetWindowText(hwndFileContentEdit, content, sizeof(content));

                    char fullPath[MAX_PATH_LENGTH];
                    snprintf(fullPath, sizeof(fullPath), "%s\\%s", dirPath, fileName);
                    write_file(fullPath, content);
                    log_event("Wrote to file");
                }
                break;

                case ID_READ_FILE: {
                    char dirPath[MAX_PATH_LENGTH];
                    prompt_directory(dirPath);

                    char fileName[MAX_PATH_LENGTH];
                    GetWindowText(hwndFileNameEdit, fileName, sizeof(fileName));

                    char fullPath[MAX_PATH_LENGTH];
                    snprintf(fullPath, sizeof(fullPath), "%s\\%s", dirPath, fileName);
                    read_file(fullPath);
                    log_event("Read file");
                }
                break;

                case ID_DELETE_FILE: {
                    char dirPath[MAX_PATH_LENGTH];
                    prompt_directory(dirPath);

                    char fileName[MAX_PATH_LENGTH];
                    GetWindowText(hwndFileNameEdit, fileName, sizeof(fileName));

                    char fullPath[MAX_PATH_LENGTH];
                    snprintf(fullPath, sizeof(fullPath), "%s\\%s", dirPath, fileName);
                    delete_file(fullPath);
                    list_files(hwndFileList);
                    log_event("Deleted file");
                }
                break;

                case ID_DELETE_DIR: {
                    char dirPath[MAX_PATH_LENGTH];
                    prompt_directory(dirPath);

                    char dirName[MAX_PATH_LENGTH];
                    GetWindowText(hwndDirNameEdit, dirName, sizeof(dirName));

                    char fullPath[MAX_PATH_LENGTH];
                    snprintf(fullPath, sizeof(fullPath), "%s\\%s", dirPath, dirName);
                    delete_directory(fullPath);
                    list_directories(hwndDirList);
                    log_event("Deleted directory");
                }
                break;

                case ID_ENTER_DIR: {
                    char dirPath[MAX_PATH_LENGTH];
                    prompt_directory(dirPath);

                    SetCurrentDirectory(dirPath);
                    list_directories(hwndDirList);
                    list_files(hwndFileList);
                    log_event("Entered directory");
                }
                break;

                case ID_EXIT_DIR: {
                    SetCurrentDirectory(currentPath);
                    list_directories(hwndDirList);
                    list_files(hwndFileList);
                    log_event("Exited directory");
                }
                break;

                case ID_LOG_CHECK:
                    display_log_content();
                    break;

                case ID_EXIT:
                    PostQuitMessage(0);
                    break;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "VirtualFileSystem";

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, "Virtual File System", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, NULL, NULL, hInstance, NULL);
    MSG msg;

    init_filesystem();

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

#include <iostream>
#include <fstream>
#include <stack>

#ifdef _WIN32
#define CLEAR_COMMAND "cls"
#else
#define CLEAR_COMMAND "clear"
#endif

#define INITIAL_BUFFER_SIZE 100

class TextEditor {
private:
    int current_line;
    char* clipboard;
    char** lines;

    std::stack<char**> undo_stack;
    std::stack<char**> redo_stack;
    std::stack<int> undo_line_counts;
    std::stack<int> redo_line_counts;

    void save_snapshot(std::stack<char**>& stack, std::stack<int>& line_count_stack) {
        char** snapshot = new char* [current_line];
        for (int i = 0; i < current_line; ++i) {
            size_t length = strlen(lines[i]) + 1;
            snapshot[i] = new char[length];
            strcpy_s(snapshot[i], length, lines[i]);
        }
        stack.push(snapshot);
        line_count_stack.push(current_line);
    }

    void restore_snapshot(std::stack<char**>& stack, std::stack<int>& line_count_stack) {
        if (stack.empty()) {
            return;
        }

        if (lines != nullptr) {
            for (int i = 0; i < current_line; ++i) {
                delete[] lines[i];
            }
            delete[] lines;
        }

        lines = stack.top();
        current_line = line_count_stack.top();
        stack.pop();
        line_count_stack.pop();
    }

    void clear_stack(std::stack<char**>& stack, std::stack<int>& line_count_stack) {
        while (!stack.empty()) {
            char** snapshot = stack.top();
            int count = line_count_stack.top();
            for (int i = 0; i < count; ++i) {
                delete[] snapshot[i];
            }
            delete[] snapshot;
            stack.pop();
            line_count_stack.pop();
        }
    }

public:
    TextEditor() {
        current_line = 0;
        clipboard = nullptr;
        lines = nullptr;

    }

    ~TextEditor() {
        if (lines != nullptr) {
            for (int i = 0; i < current_line; i++) {
                delete[] lines[i];
            }
            delete[] lines;
        }
        if (clipboard != nullptr) {
            delete[] clipboard;
        }
        clear_stack(undo_stack, undo_line_counts);
        clear_stack(redo_stack, redo_line_counts);
    }


    void clear_console() {
        system(CLEAR_COMMAND);
    }

    void append_text(const char* to_append) {
        if (current_line == 0) {
            std::cout << "No lines to append text to." << std::endl;
            return;
        }

        size_t original_length = strlen(lines[current_line - 1]);
        size_t to_append_length = strlen(to_append);
        char* temp = (char*)realloc(lines[current_line - 1], original_length + to_append_length + 2);
        if (temp == nullptr) {
            std::cout << "Memory allocation failed" << std::endl;
            return;
        }

        lines[current_line - 1] = temp;

        if (original_length > 0) {
            strcat_s(lines[current_line - 1], original_length + to_append_length + 2, " ");
        }
        strcat_s(lines[current_line - 1], original_length + to_append_length + 2, to_append);

    }
    void start_new_line() {
        save_snapshot(undo_stack, undo_line_counts);
        clear_stack(redo_stack, redo_line_counts);
        char** temp = new char* [current_line + 1];
        for (int i = 0; i < current_line; i++) {
            temp[i] = lines[i];
        }
        temp[current_line] = new char[INITIAL_BUFFER_SIZE];
        strcpy_s(temp[current_line], INITIAL_BUFFER_SIZE, "");
        delete[] lines;
        lines = temp;
        current_line++;

    }

    void save_to_file(const char* filename) {
        std::ofstream file(filename);
        if (!file) {
            std::cout << "Error opening file for writing" << std::endl;
            return;
        }

        for (int i = 0; i < current_line; i++) {
            if (lines[i] != nullptr) {
                file << lines[i] << std::endl;
            }
        }

        file.close();
        std::cout << "Text has been saved successfully to " << filename << std::endl;
    }

    void load_from_file(const char* filename) {
        std::ifstream file(filename);
        if (!file) {
            std::cout << "Error opening file for reading" << std::endl;
            return;
        }
        if (lines != nullptr) {
            for (int i = 0; i < current_line; i++) {
                free(lines[i]);
            }
            free(lines);
            lines = nullptr;
        }
        int current_line1 = 0;

        size_t buffer_size = INITIAL_BUFFER_SIZE;
        char* buffer = (char*)malloc(buffer_size);
        if (buffer == nullptr) {
            std::cout << "Memory allocation failed" << std::endl;
            file.close();
            return;
        }

        lines = (char**)malloc(sizeof(char*));
        if (lines == nullptr) {
            std::cout << "Memory allocation failed" << std::endl;
            file.close();
            free(buffer);
            return;
        }

        while (file.getline(buffer, buffer_size)) {
            size_t len = strlen(buffer);
            while (len > 0 && buffer[len - 1] != '\n') {
                buffer_size *= 2;
                buffer = (char*)realloc(buffer, buffer_size);
                if (buffer == nullptr) {
                    std::cout << "Memory allocation failed" << std::endl;
                    file.close();
                    for (int i = 0; i < current_line1; i++) {
                        free(lines[i]);
                    }
                    free(lines);
                    return;
                }
                if (!file.getline(buffer + len, buffer_size - len)) {
                    break;
                }
                len = strlen(buffer);
            }

            buffer[strcspn(buffer, "\n")] = '\0';

            lines[current_line1] = _strdup(buffer);
            if (lines[current_line1] == nullptr) {
                std::cout << "Memory allocation failed" << std::endl;
                file.close();
                free(buffer);
                for (int i = 0; i < current_line1; i++) {
                    free(lines[i]);
                }
                free(lines);
                return;
            }
            current_line1++;

            // Resize the lines array if needed
            char** temp = (char**)realloc(lines, (current_line1 + 1) * sizeof(char*));
            if (temp == nullptr) {
                std::cout << "Memory allocation failed" << std::endl;
                file.close();
                free(buffer);
                for (int i = 0; i < current_line1; i++) {
                    free(lines[i]);
                }
                free(lines);
                return;
            }
            lines = temp;
        }

        free(buffer);
        file.close();

        current_line = current_line1;

        std::cout << "Text loaded successfully from " << filename << ":" << std::endl;
        for (int i = 0; i < current_line; i++) {
            std::cout << lines[i] << std::endl;
        }
    }

    void insert_text(int line, int index, const char* text) {
        save_snapshot(undo_stack, undo_line_counts);
        clear_stack(redo_stack, redo_line_counts);
        if (line >= current_line || line < 0) {
            std::cout << "Invalid line number." << std::endl;
            return;
        }
        size_t line_length = strlen(lines[line]);
        size_t text_length = strlen(text);

        if (index > line_length || index < 0) {
            std::cout << "Invalid index." << std::endl;
            return;
        }

        lines[line] = (char*)realloc(lines[line], line_length + text_length + 1);
        if (lines[line] == nullptr) {
            std::cout << "Memory allocation failed" << std::endl;
            return;
        }

        memmove(&lines[line][index + text_length], &lines[line][index], line_length - index + 1);
        memcpy(&lines[line][index], text, text_length);
    }

    void insert_text_with_replacement(int line, int index, const char* text) {
        save_snapshot(undo_stack, undo_line_counts);
        clear_stack(redo_stack, redo_line_counts);
        if (line >= current_line || line < 0) {
            std::cout << "Invalid line number." << std::endl;
            return;
        }
        size_t line_length = strlen(lines[line]);
        size_t text_length = strlen(text);

        if (index > line_length || index < 0) {
            std::cout << "Invalid index." << std::endl;
            return;
        }

        if (index + text_length > line_length) {
            lines[line] = (char*)realloc(lines[line], index + text_length + 1);
            if (lines[line] == nullptr) {
                std::cout << "Memory allocation failed" << std::endl;
                return;
            }
        }
        memcpy(&lines[line][index], text, text_length);
        if (index + text_length > line_length) {
            lines[line][index + text_length] = '\0';
        }
    }

    void search_text(const char* text_to_search) {
        int found = 0;
        size_t search_len = strlen(text_to_search);
        for (int i = 0; i < current_line; i++) {
            char* pos = lines[i];
            while ((pos = strstr(pos, text_to_search)) != nullptr) {
                int index = pos - lines[i];
                std::cout << "Line " << i << ", Index " << index << std::endl;
                pos += search_len;
                found = 1;
            }
        }
        if (!found) {
            std::cout << "Text not found." << std::endl;
        }
    }

    void delete_text(int line, int index, int length) {
        save_snapshot(undo_stack, undo_line_counts);
        clear_stack(redo_stack, redo_line_counts);
        if (line >= current_line || line < 0) {
            std::cout << "Invalid line number." << std::endl;
            return;
        }
        size_t line_length = strlen(lines[line]);
        if (index >= line_length || index < 0 || length < 0 || index + length > line_length) {
            std::cout << "Invalid index or length." << std::endl;
            return;
        }

        memmove(&lines[line][index], &lines[line][index + length], line_length - index - length + 1);
        lines[line] = (char*)realloc(lines[line], line_length - length + 1);
        if (lines[line] == nullptr) {
            std::cout << "Memory allocation failed" << std::endl;
            return;
        }
    }

    void copy_text(int line, int index, int length) {
        if (line >= current_line || line < 0) {
            std::cout << "Invalid line number." << std::endl;
            return;
        }
        size_t line_length = strlen(lines[line]);
        if (index >= line_length || index < 0 || length < 0 || index + length > line_length) {
            std::cout << "Invalid index or length." << std::endl;
            return;
        }

        if (clipboard != nullptr) {
            delete[] clipboard;
        }
        clipboard = new char[length + 1];
        if (clipboard == nullptr) {
            std::cout << "Memory allocation failed" << std::endl;
            return;
        }

        strncpy_s(clipboard, length + 1, &lines[line][index], length);
        clipboard[length] = '\0';
    }

    void paste_text(int line, int index) {
        if (clipboard == nullptr) {
            std::cout << "Clipboard is empty." << std::endl;
            return;
        }
        insert_text(line, index, clipboard);
    }

    void cut_text(int line, int index, int length) {
        copy_text(line, index, length);
        delete_text(line, index, length);
    }

    void undo() {
        if (undo_stack.empty()) {
            std::cout << "No actions to undo." << std::endl;
            return;
        }
        save_snapshot(redo_stack, redo_line_counts);
        restore_snapshot(undo_stack, undo_line_counts);
    }

    void redo() {
        if (redo_stack.empty()) {
            std::cout << "No actions to redo." << std::endl;
            return;
        }
        save_snapshot(undo_stack, undo_line_counts);
        restore_snapshot(redo_stack, redo_line_counts);
    }
    void show_menu() {
        std::cout << "Choose the command:" << std::endl;
        std::cout << "1. Append text symbols to the end" << std::endl;
        std::cout << "2. Start new line" << std::endl;
        std::cout << "3. Use files to save the information" << std::endl;
        std::cout << "4. Use files to load the information" << std::endl;
        std::cout << "5. Print the current text to console" << std::endl;
        std::cout << "6. Insert the text by line and symbol index" << std::endl;
        std::cout << "7. Search" << std::endl;
        std::cout << "8. Delete text by line, index, and length" << std::endl; 
        std::cout << "9. Clear console" << std::endl;
        std::cout << "10. Undo" << std::endl;
        std::cout << "11. Redo" << std::endl;
        std::cout << "12. Cut text" << std::endl;
        std::cout << "13. Paste text" << std::endl;
        std::cout << "14. Copy text" << std::endl;
        std::cout << "15. Insert with replacement" << std::endl;
        std::cout << "16. Show menu" << std::endl;
        std::cout << "17. Exit" << std::endl;
    }

    char* read_line() {
        size_t size = INITIAL_BUFFER_SIZE;
        size_t len = 0;
        char* buffer = (char*)malloc(size);
        if (!buffer) {
            std::cout << "Memory allocation failed" << std::endl;
            exit(EXIT_FAILURE);
        }

        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
            buffer[len++] = ch;
            if (len == size) {
                size *= 2;
                buffer = (char*)realloc(buffer, size);
                if (!buffer) {
                    std::cout << "Memory reallocation failed" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        }
        buffer[len] = '\0';
        return buffer;
    }
    void display_text() const {
        std::cout << "Current text:" << std::endl;
        bool hasLines = false;

        for (int i = 0; i < current_line; i++) {
            if (lines[i] != nullptr) {
                std::cout << lines[i] << std::endl;
                hasLines = true;
            }
        }

        if (!hasLines) {
            std::cout << "No lines" << std::endl;
        }
    }

    void run() {
        int command;
        show_menu();
        while (true) {
            std::cout << "Enter the command: ";
            std::cin >> command;
            if (command < 1 || command > 17) {
                std::cout << "Invalid command. Please enter a number between 1 and 17." << std::endl;
                continue;
            }

            switch (command) {
            case 1: {

                std::cout << "Enter text to append: ";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                char* to_append = read_line();

                if (current_line == 0) {
                    start_new_line();
                }

                append_text(to_append);
                free(to_append);
                break;
            }

            case 2: {
                start_new_line();
                std::cout << "New line started" << std::endl;
                break;
            }
            case 3: {
                std::cout << "Enter the file name for saving: ";
                char* filename = read_line();

                save_to_file(filename);
                free(filename);
                break;
            }
            case 4: {
                std::cout << "Enter the file name for loading: ";

                char* load_filename = read_line();

                load_from_file(load_filename);
                free(load_filename);
                break;
            }
            case 5: {
                display_text();
                break;
            }
            case 6: {
                std::cout << "Choose line and index:" << std::endl;
                int line, index;
                if (scanf_s("%d %d", &line, &index) != 2) {
                    std::cout << "Invalid input. Please enter two numbers." << std::endl;
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                std::cout << "Enter text to insert:" << std::endl;
                char* text = read_line();
                insert_text(line, index, text);
                free(text);
                break;
            }
            case 7: {
                std::cout << "Enter text to search:" << std::endl;
                char* text_to_search = read_line();
                std::cout << "Text found at these positions:" << std::endl;
                search_text(text_to_search);
                free(text_to_search);
                break;
            }
            case 8: { // New case for delete
                std::cout << "Choose line, index, and length to delete:" << std::endl;
                int line, index, length;
                if (scanf_s("%d %d %d", &line, &index, &length) != 3) {
                    std::cout << "Invalid input. Please enter three numbers." << std::endl;
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                delete_text(line, index, length);
                break;
            }
            case 9: {
                clear_console();
                show_menu();
                break;
            }
            case 10: // Undo
                undo();
                break;
            case 11: // Redo
                redo();
                break;
            case 12: {
                std::cout << "Choose line, index, and length to cut:" << std::endl;
                int line, index, length;
                if (scanf_s("%d %d %d", &line, &index, &length) != 3) {
                    std::cout << "Invalid input. Please enter three numbers." << std::endl;
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                cut_text(line, index, length);
                break;
            }
            case 13: {
                std::cout << "Choose line and index to paste:" << std::endl;
                int line, index;
                if (scanf_s("%d %d", &line, &index) != 2) {
                    std::cout << "Invalid input. Please enter two numbers." << std::endl;
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                paste_text(line, index);
                break;
            }
            case 14: {
                std::cout << "Choose line, index, and length to copy:" << std::endl;
                int line, index, length;
                if (scanf_s("%d %d %d", &line, &index, &length) != 3) {
                    std::cout << "Invalid input. Please enter three numbers." << std::endl;
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                copy_text(line, index, length);
                break;
            }
            case 15: {
                std::cout << "Choose line, index, and text to insert with replacement:" << std::endl;
                int line, index;
                if (scanf_s("%d %d", &line, &index) != 2) {
                    std::cout << "Invalid input. Please enter two numbers." << std::endl;
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                std::cout << "Enter text to insert with replacement:" << std::endl;
                char* text = read_line();
                insert_text_with_replacement(line, index, text);
                free(text);
                break;
            }
            case 16: {
                show_menu();
                break;
            }
            case 17:
                std::cout << "Exiting..." << std::endl;
                exit(0);
            default:
                std::cout << "The command is not implemented." << std::endl;
            }
        }
    }
};

int main() {
    TextEditor editor;
    editor.run();

    return 0;
}



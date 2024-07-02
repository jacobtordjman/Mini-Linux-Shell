///////////////                       ID : 208489484                       /////////////////
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_COMMAND_LENGTH 1025
#define MAX_ARGUMENTS 4

typedef struct AliasNode {
    char alias[MAX_COMMAND_LENGTH];
    char command[MAX_COMMAND_LENGTH];
    struct AliasNode* next;
} AliasNode;
AliasNode* alias_head = NULL;  // Head of the alias linked list


//______________________________________Functions___________________________________________

char** tokenize_input(char* input);
void clear_input_buffer(char input[]);
int count_tokens(char** data);
int check_exit_command(char** command);
int remove_quotes_from_string(char* str);
char** concatenate_arguments(char** data);
char* assemble_command_string(char** data);
int contains_quotes(char **data);
void exec_script_file(char* script_path);
int is_sh(const char *str);

//__________________________________Alias Handling_______________________________________
int create_alias(AliasNode** alias_h, const char* alias, const char* command);
const char* find_command(const char* alias);
void change_alias(char** data);
void print_aliases();
int handle_alias_command(char** data);
int remove_alias(AliasNode** alias_h, const char* alias);

//__________________________________Memory Handling______________________________________
void free_all_aliases();
void free_tokenized_data(char** data);

//_______________________________   Ex2 - Function calls ________________________________
typedef struct Job {
    pid_t pid;
    char command[MAX_COMMAND_LENGTH];
    struct Job* next;
} Job;

Job* job_head = NULL; // Head of the job linked list

int handle_logical_operators(char* input, int dup, int redirected, char* stderr_file);
int is_logical_operated(char* input);
void remove_outer_parentheses(char* str);
void remove_after_redirection(char* str);
void remove_spaces(char* str);

void add_job(pid_t pid, const char* command);
void remove_job(pid_t pid);
void list_jobs();
void free_all_jobs();
void sigchld_handler();

int handle_redirection(char** data, char** stderr_file);

int command_count = 0;
int alias_count = 0;
int script_lines_count = 0;
int command_quote_count = 0;
int quote_flag = 0;
pid_t status;

//______________________________________Main___________________________________________

// Function declarations (omitted for brevity)

int main(void) {
    char input[MAX_COMMAND_LENGTH];
    char original_input[MAX_COMMAND_LENGTH];
    char** data;
    int backup_out = dup(STDERR_FILENO);
    int redirected = 0;
    pid_t pid;

    // Setup the signal handler for SIGCHLD
    struct sigaction sa;
    sa.sa_handler = &sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while (1) {
        printf("#cmd:%d|#alias:%d|#script lines:%d> ", command_count, alias_count, script_lines_count);
        clear_input_buffer(input);
        fgets(input, MAX_COMMAND_LENGTH, stdin);
        if (strcmp(input, "\n") == 0) { // Skip empty commands
            continue;
        }

        input[strcspn(input, "\n")] = '\0'; // Remove the newline character
        strcpy(original_input, input);
        data = tokenize_input(input); // Split the input into tokens
        char* stderr_file = NULL;
        if (handle_redirection(data, &stderr_file)) {
            redirected = 1;
            if (stderr_file) {
                int fd = open(stderr_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
                if (fd == -1) {
                    perror("open");
                    exit(1);
                }
                if (dup2(fd, STDERR_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(fd);
            }
        }

        if (is_logical_operated(original_input)) {
            if (redirected) {
                remove_after_redirection(original_input);
            }
            handle_logical_operators(original_input, backup_out, redirected, stderr_file);
            free_tokenized_data(data);
            if (stderr_file) {
                free(stderr_file);
            }
            continue;
        }

        int background = 0;
        size_t len = strlen(data[count_tokens(data) - 1]);
        if (data[count_tokens(data) - 1][len - 1] == '&') {
            background = 1;
            data[count_tokens(data) - 1][len - 1] = '\0'; // Remove the '&' character
        }

        change_alias(data);
        char* temp_string = assemble_command_string(data);
        temp_string[strcspn(temp_string, "\n")] = '\0'; // Remove the newline character
        free_tokenized_data(data);
        data = tokenize_input(temp_string);
        free(temp_string);

        quote_flag = contains_quotes(data);

        if (strcmp(data[0], "echo") == 0 && quote_flag == 1) {
            char** temp = concatenate_arguments(data);
            free_tokenized_data(data);
            data = temp;
            if ((data[1][0] == '"' && data[1][strlen(data[1]) - 1] == '"') ||
                (data[1][0] == 39 && data[1][strlen(data[1]) - 1] == 39)) {
                memmove(&data[1][0], &data[1][1], strlen(data[1]) - 2);
                data[1][strlen(data[1]) - 2] = '\0';
            }
        }

        if (strcmp(data[0], "source") == 0) {
            char** temp = concatenate_arguments(data);
            free_tokenized_data(data);
            data = temp;
            if (is_sh(data[1]) != 1) {
                fprintf(stderr, "ERR\n");
                free_tokenized_data(data);
                dup2(backup_out, STDERR_FILENO);
                if (stderr_file) {
                    free(stderr_file);
                }
                continue;
            }
            exec_script_file(data[1]);
            free_tokenized_data(data);
            dup2(backup_out, STDERR_FILENO);
            if (stderr_file) {
                free(stderr_file);
            }
            continue;
        }

        if (strcmp(data[0], "alias") == 0 || strcmp(data[0], "unalias") == 0) {
            handle_alias_command(data);
            free_tokenized_data(data);
            dup2(backup_out, STDERR_FILENO);
            if (stderr_file) {
                free(stderr_file);
            }
            continue;
        }

        if (strcmp(data[0], "jobs") == 0) {
            list_jobs();
            command_count++;
            free_tokenized_data(data);
            dup2(backup_out, STDERR_FILENO);
            if (stderr_file) {
                free(stderr_file);
            }
            continue;
        }

        if (check_exit_command(data)) {
            free_tokenized_data(data);
            free_all_jobs();
            free_all_aliases();
            printf("%d\n", command_quote_count);
            dup2(backup_out, STDERR_FILENO);
            if (stderr_file) {
                free(stderr_file);
            }
            break;
        }

        if ((count_tokens(data) > MAX_ARGUMENTS + 1) || (redirected && count_tokens(data) > MAX_ARGUMENTS - 1)) {
            fprintf(stderr, "ERR\n");
            free_tokenized_data(data);
            dup2(backup_out, STDERR_FILENO);
            if (stderr_file) {
                free(stderr_file);
            }
            continue;
        }

        pid = fork(); // Create a child process
        if (pid == -1) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            // In the child process, execute the command
            if (execvp(data[0], data) == -1) {
                perror("exec");
                exit(1);
            }
        } else {
            if (!background) {
                wait(&status); // Wait for the child process to finish
            } else {
                add_job(pid, original_input); // Add the job to the job list
            }
            free_tokenized_data(data);
            if (stderr_file) {
                free(stderr_file);
            }
            if (status == 0) {
                command_count += 1;
                if (quote_flag == 1) {
                    command_quote_count += 1;
                }
            }
        }
        quote_flag = 0;
    }
    return 0;
}

char** tokenize_input(char* input) {
    int arg_count = 0;
    char** result = (char**)malloc((strlen(input) + 1) * sizeof(char*));
    if (result == NULL) {
        perror("malloc");
        exit(1);
    }

    const char* delimiters = " \t\n\r"; // Delimiters include space, tab, newline, and carriage return
    char* token = strtok(input, delimiters);
    while (token != NULL) {
        result[arg_count++] = strdup(token);
        token = strtok(NULL, delimiters);
    }

    result[arg_count] = NULL;
    return result;
}
void clear_input_buffer(char input[]) {
    for (int i = 0; input[i] != '\0'; i++) {
        input[i] = '\0';
    }
}
int count_tokens(char** data) {
    int count = 0;
    if (data == NULL){
        return count;
    }
    while (data[count] != NULL) {
        count++;
    }
    return count;
}
int check_exit_command(char** command){
    if (strcmp(command[0], "exit_shell") == 0 && count_tokens(command) == 1){
        return 1;
    }
    return 0;
}
int remove_quotes_from_string(char* str) {
    int flag = 0;
    int i, j;
    for (i = 0, j = 0; str[i] != '\0'; i++) {
        if (str[i] != '\'' && str[i] != '"') {
            str[j++] = str[i];
        }
        else{
            flag = 1;
        }
    }
    str[j] = '\0';
    return flag;
}
char** concatenate_arguments(char** data) {
    if (data[0] == NULL) {
        return NULL;
    }

    // Calculate the length of the concatenated string
    size_t total_length = 0;
    for (int i = 1; data[i] != NULL; i++) {
        total_length += strlen(data[i]) + 1; // +1 for space or null terminator
    }

    // Allocate memory for the concatenated string
    char* concatenated = (char*)malloc(total_length * sizeof(char));
    if (concatenated == NULL) {
        perror("malloc");
        exit(1);
    }
    concatenated[0] = '\0'; // Initialize concatenated string

    // Concatenate the strings
    for (int i = 1; data[i] != NULL; i++) {
        strcat(concatenated, data[i]);
        if (data[i + 1] != NULL) {
            strcat(concatenated, " ");
        }
    }

    // Allocate memory for the new data
    char** result = (char**)malloc(3 * sizeof(char*));
    if (result == NULL) {
        perror("malloc");
        exit(1);
    }

    // Set the first element to data[0]
    result[0] = strdup(data[0]);
    if (result[0] == NULL) {
        perror("malloc");
        exit(1);
    }

    // Set the second element to the concatenated string
    result[1] = concatenated;
    result[2] = NULL; // Null-terminate the data
    return result;
}
char* assemble_command_string(char** data) {
    // Calculate the total length of the resulting string
    int total_length = 0;
    for (int i = 0; data[i] != NULL; i++) {
        total_length += strlen(data[i]); //NOLINT
    }

    // Add space characters between strings
    total_length += count_tokens(data) - 1; // Add space for separators
    total_length++; // Add space for null terminator

    // Allocate memory for the resulting string
    char* result = (char*)malloc(total_length * sizeof(char));
    if (result == NULL) {
        perror("malloc");
        exit(1);
    }

    // Build the string by concatenating strings with space characters
    result[0] = '\0'; // Initialize the result string
    for (int i = 0; data[i] != NULL; i++) {
        strcat(result, data[i]); // Concatenate current string
        if (data[i + 1] != NULL) {
            strcat(result, " "); // Add space character if not the last string
        }
    }

    return result;
}
int contains_quotes(char **data) {
    int i, j;
    int quote_count = 0;

    for (i = 0; data[i] != NULL; i++) {
        for (j = 0; data[i][j] != '\0'; j++) {
            if (data[i][j] == '\'' || data[i][j] == '"') {
                quote_count++;
                if (quote_count >= 2) {
                    return 1; // Found at least two quotes
                }
            }
        }
    }
    return 0; // Less than two quotes found
}
void exec_script_file(char* script_path) {
    FILE* file = fopen(script_path, "r");
    if (file == NULL) {
        printf("ERR\n");
        return;
    }
    char input_script[MAX_COMMAND_LENGTH];
    int row_flag = 0;
    while (fgets(input_script, sizeof(input_script), file) != NULL) {
        input_script[strcspn(input_script, "\n")] = '\0'; // Remove the newline character
        if (row_flag == 0) {
            if (strcmp(input_script, "#!/bin/bash") != 0) {
                printf("ERR\n");
                return;
            }
        }
        row_flag = 1;
        if (strchr(input_script, '\n') == NULL && !feof(stdin)) {
            int c;
            int extra_input = 0;
            while ((c = getchar()) != '\n' && c != EOF) {
                extra_input = 1;
            }
            if (extra_input) {
                printf("ERR\n");
                continue;
            }
        }

        char** line_data = tokenize_input(input_script); // Split the input_script into tokens

        // Skip empty lines
        if (line_data[0] == NULL) {
            free_tokenized_data(line_data);
            script_lines_count += 1;
            continue;
        }
        // Skip comments
        if (line_data[0][0] == '#') {
            script_lines_count += 1;
            free_tokenized_data(line_data);
            continue;
        }

        if (strcmp(line_data[0], "alias" ) == 0 || strcmp(line_data[0], "unalias") == 0){
            int n = handle_alias_command(line_data);
            if (n == 1) {
                script_lines_count += 1;
            }
            free_tokenized_data(line_data);
            continue;
        }


        change_alias(line_data);
        quote_flag = contains_quotes(line_data);


        if (strcmp(line_data[0],"echo") == 0 && quote_flag == 1){
            char** temp = concatenate_arguments(line_data);
            char q[2];
            strcpy(q,"'");
            free_tokenized_data(line_data);
            line_data = temp;
            // Remove the quotes if the first and last character of the argument are quotes
            if ((line_data[1][0] == '"' && line_data[1][strlen(line_data[1])-1] == '"') ||
                (line_data[1][0] == 39 && line_data[1][strlen(line_data[1])-1] == 39)) {
                memmove(&line_data[1][0], &line_data[1][1], strlen(line_data[1]) - 2);
                line_data[1][strlen(line_data[1])-2] = '\0';
            }
        }

        if (count_tokens(line_data) > MAX_ARGUMENTS + 1) {
            printf("ERR\n");
            free_tokenized_data(line_data);
            quote_flag = 0;
            continue;
        }

        pid_t pid = fork(); // Create a child process
        if (pid == -1) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            // In the child process, execute the command
            if (execvp(line_data[0], line_data) == -1) {
                perror("exec");
                _exit(1);
            }

        } else {
            wait(&status); // Wait for the child process to finish
            if (status == 1){
                script_lines_count += 1;
                command_count += 1;
                free_tokenized_data(line_data);
                quote_flag = 0;
                continue;
            }
            if (quote_flag == 1) {
                command_quote_count++;
            }
        }
        command_count += 1;
        script_lines_count += 1;
        quote_flag = 0;
        free_tokenized_data(line_data);
    }
    fclose(file);
}
int is_sh(const char *str) {
    size_t len = strlen(str);
    if (len < 3) {
        return 0; // The string is too short to end with ".sh"
    }
    return strcmp(str + len - 3, ".sh") == 0;
}
int create_alias(AliasNode** alias_h, const char* alias, const char* command) {
    // Check if the alias already exists
    AliasNode* current = *alias_h;
    while (current != NULL) {
        if (strcmp(current->alias, alias) == 0) {
            // Update the command associated with the alias
            strcpy(current->command, command);
            return 0;
        }
        current = current->next;
    }

    // If the alias doesn't exist, add a new node
    AliasNode* new_node = (AliasNode*)malloc(sizeof(AliasNode));
    if (new_node == NULL) {
        perror("malloc");
        exit(1);
    }
    strcpy(new_node->alias, alias);
    strcpy(new_node->command, command);
    new_node->next = *alias_h;
    *alias_h = new_node;
    return 1;
}
const char* find_command(const char* alias) {
    AliasNode* current = alias_head;
    while (current != NULL) {
        if (strcmp(current->alias, alias) == 0) {
            return current->command;
        }
        current = current->next;
    }
    return NULL;
}
void change_alias(char** data) {
    // Check if the first argument matches any alias
    const char* alias = data[0];
    const char* command = find_command(alias);

    if (command != NULL) {
        // Replace the first argument with the corresponding command
        strcpy(data[0], command);
    }
}
void print_aliases() {
    AliasNode* current = alias_head;
    while (current != NULL) {
        printf("%s='%s'\n", current->alias, current->command);
        current = current->next;
    }
}
int handle_alias_command(char** data) {
    char** new_data = concatenate_arguments(data);
    if (strcmp(data[0],"alias") == 0) {
        if (count_tokens(data) == 1) {
            print_aliases();
            command_count += 1;
        } else {
            if (strstr(new_data[1], "=")) {
                char *alias = new_data[1];
                char *command = strchr(alias, '=');
                if (command != NULL) {
                    *command = '\0';
                    command = command + 1;
                    while (strchr(command, ' ') == command) {
                        *command = '\0';
                        command = command + 1;
                    }
                }
                int alias_counter = 0;
                for (int i = 0; alias[i] != '\0'; i++) { alias_counter += 1; }
                alias_counter -= 1;
                while (alias[alias_counter] == ' ') {
                    alias[alias_counter] = '\0';
                    alias_counter -= 1;
                }

                int check_quotes = remove_quotes_from_string(command);
                if (check_quotes == 1) {
                    command_quote_count += 1;
                }
                int check_add = create_alias(&alias_head, alias, command);
                if (check_add == 1) {
                    alias_count += 1;
                }
                command_count += 1;
            }
            else {
                fprintf(stderr,"ERR\n");
                free_tokenized_data(new_data);
                return 0;
            }
        }

    }
    else{
        int check_delete = remove_alias(&alias_head, new_data[1]);
        if (check_delete == 1) {
            alias_count -= 1;
            command_count += 1;
        }
    }
    free_tokenized_data(new_data);
    return 1;
}
int remove_alias(AliasNode** alias_h, const char* alias) {
    AliasNode* current = *alias_h;
    AliasNode* previous = NULL;
    while (current != NULL) {
        if (strcmp(current->alias, alias) == 0) {
            if (previous == NULL) {
                // Head of the list
                *alias_h = current->next;
            } else {
                previous->next = current->next;
            }
            free(current);
            return 1;
        }
        previous = current;
        current = current->next;
    }
    printf("ERR\n");
    return 0;
}
void free_all_aliases() {
    AliasNode* current = alias_head;
    while (current != NULL) {
        AliasNode* next = current->next;
        free(current);
        current = next;
    }
}
void free_tokenized_data(char** data) {
    if (data != NULL) {
        for (int i = 0; data[i] != NULL; i++) {
            free(data[i]);
        }
        free(data);
    }
}
//____________________________________________   Ex2   __________________________________________________
int handle_logical_operators(char* input, int dup, int redirected,char * stderr_file) {
    char* temp_input = strdup(input);
    char* commands[MAX_COMMAND_LENGTH];
    char bg_command[MAX_COMMAND_LENGTH] = "";
    int num_commands = 0;
    int backup_out = dup;

    // Remove parentheses if redirected
    if (redirected == 1) {
        remove_outer_parentheses(temp_input);
    }

    // Custom tokenizer to handle && and || without removing &
    char* start = temp_input;
    char* end = temp_input;
    while (*end != '\0') {
        if ((*end == '&' && *(end + 1) == '&')||(*end == '|' && *(end + 1) == '|')) {
            *end = '\0';
            commands[num_commands++] = start;
            start = end + 2;
            end = start;
        } else {
            end++;
        }
    }
    commands[num_commands++] = start;


    char* remaining_input = input;
    int result = 0;  // Result of the last executed command
    for (int i = 0; i < num_commands; ++i) {
        // Find the next logical operator
        while (*remaining_input != '\0' && *remaining_input != '&' && *remaining_input != '|') {
            remaining_input++;
        }
        int operator_type = 0; // 0 for no operator, 1 for &&, 2 for ||

        if (*remaining_input == '&' && *(remaining_input + 1) == '&') {
            operator_type = 1;
            *remaining_input = '\0';
            remaining_input += 2;
        } else if (*remaining_input == '|' && *(remaining_input + 1) == '|') {
            operator_type = 2;
            *remaining_input = '\0';
            remaining_input += 2;
        }

        // Check for background execution
        int background = 0;
        size_t len = strlen(commands[i]);
        if (commands[i][len - 1] == '&') {
            strcpy(bg_command,commands[i]);
            background = 1;
            commands[i][len - 1] = '\0'; // Remove the '&' character
        }

        // Execute the current command
        char* current_command = commands[i];
        char** data = tokenize_input(current_command);

//        char* stderr_file = NULL;
        if (handle_redirection(data, &stderr_file) || redirected) {
            // In the child process, handle stderr redirection
            if (stderr_file) {
                int fd = open(stderr_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
                if (fd == -1) {
                    perror("open");
                    exit(1);
                }
                if (dup2(fd, STDERR_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(fd);
            }
        }

        if (check_exit_command(data)) {
            free_tokenized_data(data);
            free_all_aliases();
            free_all_jobs();
            printf("%d\n", command_quote_count);
            free(temp_input);
            exit(0);
        }

        if (strcmp(data[0], "source") == 0) {
            char** temp = concatenate_arguments(data);
            free_tokenized_data(data);
            data = temp;
            if (is_sh(data[1]) != 1) {
                fprintf(stderr, "ERR\n");
                free_tokenized_data(data);
                dup2(backup_out, STDERR_FILENO);
                continue;
            }
            exec_script_file(data[1]);
            free_tokenized_data(data);
            dup2(backup_out, STDERR_FILENO);
            continue;
        }

        if (strcmp(data[0], "jobs") == 0) {
            list_jobs();
            command_count++;
            free_tokenized_data(data);
            dup2(backup_out, STDERR_FILENO);
            continue;
        }

        if (strcmp(data[0], "alias") == 0 || strcmp(data[0], "unalias") == 0) {
            handle_alias_command(data);
            free_tokenized_data(data);
            dup2(backup_out, STDERR_FILENO);
            continue;
        }

        change_alias(data);
        char* temp_string = assemble_command_string(data);
        temp_string[strcspn(temp_string, "\n")] = '\0';
        free_tokenized_data(data);
        data = tokenize_input(temp_string);
        free(temp_string);

        quote_flag = contains_quotes(data);

        if (strcmp(data[0], "echo") == 0 && quote_flag == 1) {
            char** temp = concatenate_arguments(data);
            free_tokenized_data(data);
            data = temp;
            if ((data[1][0] == '"' && data[1][strlen(data[1]) - 1] == '"') ||
                (data[1][0] == 39 && data[1][strlen(data[1]) - 1] == 39)) {
                memmove(&data[1][0], &data[1][1], strlen(data[1]) - 2);
                data[1][strlen(data[1]) - 2] = '\0';
            }
        }

        if ((count_tokens(data) > MAX_ARGUMENTS + 1)||(redirected && count_tokens(data) > MAX_ARGUMENTS - 1 )) {
            fprintf(stderr, "ERR\n");
            free_tokenized_data(data);
            dup2(backup_out, STDERR_FILENO);
            result = -1;
            break;
        }

        pid_t pid = fork(); // Create a child process
        if (pid == -1) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            // In the child process, execute the command
            if (execvp(data[0], data) == -1) {
                perror("exec");
                exit(1);
            }
        } else {
            if (!background) {
                wait(&status); // Wait for the child process to finish
            } else {
                add_job(pid,bg_command); // Add the job to the job list
                // printf("[%d] %d\n", command_count + 1, pid); // Print the job id and PID
            }
            free_tokenized_data(data);
            if (WIFEXITED(status)) {
                result = WEXITSTATUS(status);
            } else {
                result = -1;
            }

            if (result == 0) {
                command_count += 1;
                if (quote_flag == 1) {
                    command_quote_count += 1;
                }
            }
        }
        quote_flag = 0;
        dup2(backup_out, STDERR_FILENO);
        // Check the status and decide whether to continue or break
        if (operator_type == 1 && result != 0 && i == 0) {
            i++;
            continue;
        } else if ((operator_type == 1 && result != 0) || (operator_type == 2 && result == 0)) {
            break;
        }
    }
    free(temp_input);
    return result;
}

int is_logical_operated(char* input) {
    return (strstr(input, "&&") != NULL || strstr(input, "||") != NULL);
}

// Function to add a job to the job list
void add_job(pid_t pid, const char* command) {
    char temp_command[MAX_COMMAND_LENGTH] = "";
    strcpy(temp_command,command);
    remove_spaces(temp_command);
    Job* new_job = (Job*)malloc(sizeof(Job));
    if (new_job == NULL) {
        perror("malloc");
        exit(1);
    }
    new_job->pid = pid;
    snprintf(new_job->command, MAX_COMMAND_LENGTH, "%s", temp_command); // Copy the full command with the & suffix
    new_job->next = job_head;
    job_head = new_job;
}

// Function to remove a job from the job list
void remove_job(pid_t pid) {
    Job* current = job_head;
    Job* previous = NULL;
    while (current != NULL) {
        if (current->pid == pid) {
            if (previous == NULL) {
                job_head = current->next;
            } else {
                previous->next = current->next;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

void list_jobs() {
    Job* current = job_head;
    int job_id = 1;
    while (current != NULL) {
        // Adjust the formatting to include 15 spaces between the job number and the command
        printf("[%d]%*s%s\n", job_id, 15, "", current->command);
        current = current->next;
        job_id++;
    }
}

void free_all_jobs() {
    Job* current = job_head;
    while (current != NULL) {
        Job* next = current->next;
        free(current);
        current = next;
    }
    job_head = NULL;
}

void sigchld_handler() {
    int saved_errno = errno;
    while (1) {
        int temp_status;
        pid_t pid = waitpid(-1, &temp_status, WNOHANG);
        if (pid <= 0) {
            break;
        }
        remove_job(pid);
    }
    errno = saved_errno;
}

int handle_redirection(char** data, char** stderr_file) {
    for (int i = 0; data[i] != NULL; i++) {
        if (strcmp(data[i], "2>") == 0 && data[i+1] != NULL) {
            *stderr_file = strdup(data[i+1]); // Get the file name
            free(data[i]); // Free the "2>" token
            for (int j = i; data[j] != NULL; j++) {
                data[j] = data[j+2]; // Shift the remaining tokens
            }
            return 1; // Indicate that redirection was found
        }
    }
    return 0; // No redirection found
}
void remove_outer_parentheses(char* str) {
    remove_spaces(str);
    // Recalculate the length after trimming spaces
    size_t len = strlen(str);

    if (len > 1 && str[0] == '(' && str[len - 1] == ')') {
        // Move all characters one position to the left and remove the last character
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}
// Function to remove everything after "2>"
void remove_after_redirection(char* str) {
    char* redirection_pos = strstr(str, "2>");
    if (redirection_pos != NULL) {
        *redirection_pos = '\0'; // Terminate the string at the position of "2>"
    }
}
void remove_spaces(char* str){
    size_t len = strlen(str);
    size_t start = 0;
    size_t end = len - 1;

    // Trim leading spaces
    while (start < len && isspace((unsigned char)str[start])) {
        start++;
    }

    // Trim trailing spaces
    while (end > start && isspace((unsigned char)str[end])) {
        end--;
    }

    // Shift the trimmed string to the beginning
    if (start > 0 || end < len - 1) {
        memmove(str, str + start, end - start + 1);
    }
    str[end - start + 1] = '\0'; // Null-terminate the string
}
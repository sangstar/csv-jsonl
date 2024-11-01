#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define HAS_ARG(x) (i + 1 < argc && strcmp(argv[i], x) == 0)

size_t BYTES_PER_BUFFER=1000;
size_t READ_SIZE=1000;
size_t NUM_LINES=300;

typedef struct {
    char *slice;
    int cursor;
    int has_newline;
} sliced_buffer;

typedef struct {
    char **lines;
    int cursor;
} lines_buffer;


int is_integer(char *string) {
    int counter = 0;
    for (int i = 0; i < strlen(string); ++i) {
        counter += isnumber(string[i]);
    }
    if (counter == strlen(string)) {
        return 1;
    }
    return 0;
}

int is_float(char *string) {
    int counter = 0;
    int has_dot = 0;
    for (int i = 0; i < strlen(string); ++i) {
        if (string[i] == '.') {
            has_dot = 1;
        }
        counter += isnumber(string[i]);
    }
    if ((counter == strlen(string) - 1) && has_dot) {
        return 1;
    }
    return 0;
}

// Creates a sliced_buffer from a text buffer `buf`
// A sliced_buffer is a slice of text that guarantees
// terminating with a newline
sliced_buffer *get_to_newline(char *buf, int cursor, char *newline_seq){
    sliced_buffer *sliced = (sliced_buffer *)malloc(sizeof(sliced_buffer));
    sliced->slice = (char *)malloc(sizeof(char) * strlen(buf));
    sliced->cursor = 0;
    sliced->has_newline = 0;

    // Skip past the previous newline_seq
    if (cursor != 0) {
        cursor+=2;
    }

    for (int i = cursor; i < strlen(buf); ++i) {

        // Check if the search window for a newline_seq is within bounds
        if (buf[i] == newline_seq[0] && i + strlen(newline_seq) < strlen(buf)) {
            int comparator = 0;
            for (int j = 0; j < strlen(newline_seq); ++j) {
                comparator+= buf[i + j] == newline_seq[j];
            }
            if (comparator == strlen(newline_seq)) {
                sliced->has_newline = 1;
                break;
            }
        }
        // Ensure the indexing is positive based on the relative
        // sizes of the cursor and iterator
        if (cursor > i) {
            sliced->slice[cursor - i] = buf[i];
        } else {
            sliced->slice[i - cursor] = buf[i];
        }

    }

    // Shift the cursor by the amount moved. This actually refers to
    // the slice's cursor position on buf
    sliced->cursor = cursor + strlen(sliced->slice);
    return sliced;
}

// Writes lines of text from a `buf` buffer to a `lines_buffer`
char *add_lines_from_buffer(lines_buffer *line_buf, char *buf, char *newline_seq) {

    // A separate cursor variable is maintained that keeps track of
    // sliced->cursor values to avoid having to have `get_to_newline`
    // take `sliced` as an argument just for its `cursor` field,
    // which is unnecessary
    int cursor = 0;

    char *overflow = (char *)malloc(sizeof(char) * strlen(buf) + 1);


    while (cursor < strlen(buf)) {
        sliced_buffer *sliced = get_to_newline(buf, cursor, newline_seq);
        cursor = sliced->cursor;

        // If the slice is a complete row, save it
        if (sliced->has_newline) {
            line_buf->lines[line_buf->cursor] = malloc(sizeof(char) * strlen(buf));
            strcpy(line_buf->lines[line_buf->cursor], sliced->slice);
            line_buf->cursor+=1;
        }

        // If about to exit loop, save overflow
        if (cursor >= strlen(buf)) {
            memcpy(overflow, sliced->slice, strlen(sliced->slice));
        }
        free(sliced);
    }
    return overflow;

}

// Saves each line in a CSV to a `lines_buffer` struct
// Reads up to NUM_LINES lines
lines_buffer *file_to_lines_buffer(FILE *file, char *newline_seq) {

    lines_buffer *lines = (lines_buffer *)malloc(sizeof(lines_buffer) * NUM_LINES);
    lines->lines = (char **)malloc(sizeof(char) * NUM_LINES * BYTES_PER_BUFFER);
    lines->cursor=0;

    char *buf = (char *) malloc(sizeof(char) * 1 * BYTES_PER_BUFFER);


    for (int i = 0; i < NUM_LINES; ++i){


        // Add current buffer to lines and get overflow from previous call
        char *overflow = add_lines_from_buffer(lines, buf, newline_seq);

        // Clear buffer first, since buf has its sized changed dynamically based on overflow, so
        // reading BYTES_PER_BUFFER bytes into the buffer with a dynamic buffer size of BYTES_PER_BUFFER + overflow_size
        // will keep some text at the end from a previous step, corrupting the data integrity
        memset(buf, 0, strlen(buf));

        // Fill new text data in to buffer
        fread(buf, 1, READ_SIZE, file);

        // See if needed to flush overflow in to new buffer
        size_t overflow_size = strlen(overflow);
        size_t buf_size = strlen(buf);
        if (overflow_size > 0) {
            // When there's overflow, make a new buffer that contains [overflow, buf]
            // and make that the new buf
            size_t resized_len = buf_size + overflow_size;

            // Add 2 here to account for null terminators for the overflow and buf buffers
            char *resized_buf = (char *) malloc(sizeof(char) * resized_len + 2);
            for (int j = 0; j < overflow_size; ++j) {
                resized_buf[j] = overflow[j];
            }
            for (size_t k = overflow_size; k < resized_len; ++k) {
                resized_buf[k] = buf[k - overflow_size];
            }

            // Clear the buffer before freeing it to prevent stray text data in memory
            memset(buf, 0, buf_size);
            free(buf);
            buf = resized_buf;
        }
        free(overflow);
    }


    free(buf);
    return lines;
}

// Saves a lines_buffer to a jsonl
// Assumes the only special characters that need
// escaping are the newline \n and the CSV-escaping of the
// double quote with the double quote pair ""
void lines_to_jsonl(lines_buffer *lines, char *outfile) {

    // Get number of cols
    int num_cols = 0;
    char *col_names_line = lines->lines[0];
    int i;
    for (i = 0; i < strlen(col_names_line); ++i) {
        if (col_names_line[i] == ',') {
            num_cols++;
        }
    }

    // Check if last column doesn't end with a comma
    // This implies there's one more column than comma
    // which means we'll add an extra 1 so that the number
    // of commas equates to the number of columns
    if (col_names_line[i] != ',') {
        num_cols++;
    }


    // Get columns
    size_t max_col_name_length = 150;
    char **cols = malloc(sizeof(char *) * num_cols);

    int counter = 0;
    char *column_name_buffer = malloc(sizeof(char) * max_col_name_length);
    for (int j = 0; j < num_cols; ++j) {
        for (i = 0; i < max_col_name_length; ++i) {
            column_name_buffer[i] = col_names_line[i + counter];
            if (col_names_line[i + counter] == ',') {

                // Replace the trailing comma with a null-terminator
                column_name_buffer[i] = '\0';

                // Since we replaced the last char with the null-terminator, we don't
                // have to add +1 to make extra space
                cols[j] = malloc(sizeof(char) * strlen(column_name_buffer));
                strcpy(cols[j], column_name_buffer);

                // Need to zero out the max size, not strlen(column_name_buffer),
                // because it's null-terminated early so strlen will only count up
                // to the first /0
                memset(column_name_buffer, 0, max_col_name_length);

                // Increment counter by i + 1 to skip over the next comma
                counter+=i+1;
                break;
            }
        }
    }

    // Write to jsonl
    FILE *jsonl_file = fopen(outfile, "w");
    if (jsonl_file == NULL) {
        perror("Error opening file");
        return;
    }

    // Write line-by-line
    // Start at 1 because 0 is the column names
    int inside_outer_quotes = 0;
    int quotes_encountered = 0;
    char *col_value = malloc(sizeof(char) * max_col_name_length);
    for (int row = 1; row < NUM_LINES; ++row) {
        char *current_line = lines->lines[row];
        counter = 0;
        fprintf(jsonl_file, "{");
        for (int col = 0; col < num_cols; ++col) {
            char *col_name = cols[col];
            int col_value_idx = 0;
            for (int m = 0; m < max_col_name_length; ++m) {
                int incrementor = m + counter;
                if (!inside_outer_quotes && current_line[incrementor] == ',') {
                    counter += m + 1;
                    break;
                }

                if (current_line[incrementor] == '"') {
                    quotes_encountered++;
                    if (current_line[incrementor + 1] == '"') {
                        col_value[col_value_idx++] = '\\';
                        col_value[col_value_idx++] = '"';
                        m++;
                    } else {
                        inside_outer_quotes = quotes_encountered % 2 != 0;
                        col_value[col_value_idx++] = current_line[incrementor];
                    }
                } else if (current_line[incrementor] == '\n') {
                    col_value[col_value_idx++] = '\\';
                    col_value[col_value_idx++] = 'n';
                } else {
                    col_value[col_value_idx++] = current_line[incrementor];
                }
            }
            if (is_integer(col_value)) {
                fprintf(jsonl_file, "\"%s\": \"%d\",", col_name, atoi(col_value));
            } else if (is_float(col_value)) {
                fprintf(jsonl_file, "\"%s\": \"%f\",", col_name, atof(col_value));

            } else if (quotes_encountered >= 2) {
                fprintf(jsonl_file, "\"%s\": %s,", col_name, col_value);
            } else {
                fprintf(jsonl_file, "\"%s\": \"%s\",", col_name, col_value);
            }
            quotes_encountered = 0;
            memset(col_value, 0, max_col_name_length);
        }
        fprintf(jsonl_file, "}\n");
    }
    free(cols);
}

int main(int argc, char *argv[]) {
    char *input_filename = argv[1];
    char *output_filename = argv[2];
    if (argc < 3) {

        if (argc < 2) {
            printf("Error: Invalid arguments.\n\n");
            exit(1);
        }

        if ((strcmp(argv[1], "--help") == 1) && (strcmp(argv[1], "-h") == 1)) {
            printf("Error: Invalid arguments.\n\n");
        }
        printf("Usage: ./csv-jsonl input_csv output_jsonl\n");
        printf("Arguments:\n");
        printf("   input_csv                   The path to the csv to be converted\n");
        printf("   output_jsonl                The path where the converted jsonl will be created\n");
        printf("   -n                  \\n      (Optional) Newline escape sequence. Default \\n\n");
        printf("   --buffer-size       1000    (Optional) Bytes per read\n");
        printf("   --buffer-read-size  1000    (Optional) Read size from outer buffer\n");
        exit(1);
    }

    char *newline_seq = "\n";
    for (int i = 0; i < argc; ++i) {
        if HAS_ARG("-n") {

            // CLI args are literally parsed string,
            // so it won't automatically identify
            // special characters

            // I've only directly caught two basic
            // newline_seq cases here
            if (strcmp(argv[i + 1], "\\r\\n") == 0) {
                newline_seq = "\r\n";
            } else if (strcmp(argv[i + 1], "\\n") == 0) {
                newline_seq = "\n";
            } else {
                newline_seq = argv[i + 1];
            }
        }
        if HAS_ARG("--buffer-size") {
            BYTES_PER_BUFFER = atoi(argv[i + 1]);
        }
        if HAS_ARG("--buffer-read-size") {
            READ_SIZE = atoi(argv[i + 1]);
        }
    }

    FILE *file = fopen(input_filename, "r");
    if (file == NULL) {
        printf("Could not read input file");
        exit(1);
    }
    lines_buffer *lines = file_to_lines_buffer(file, newline_seq);
    lines_to_jsonl(lines, output_filename);
    free(lines);
    return 0;
}
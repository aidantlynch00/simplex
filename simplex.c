/**
 * Author: Aidan Lynch
 *
 * simplex: runs the simplex method on a payoff matrix to find the optimal
 * strategies and the value of the game
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>

/**
 * Frees a 2 dimensional array.
 *
 * rows: number of rows
 */
void free_2d_arr(void** arr, int rows) {
    if (arr != NULL) {
        // free row arrays
        for (int row = 0; row < rows; row++) {
            if (arr[row] != NULL) free(arr[row]);
        }

        free(arr);
    }
}

/**
 * Print the usage statement for this program.
 */
void print_usage() {
    printf("usage: simplex m n\n");
    printf("\tm: number of rows, integer greater than 0\n");
    printf("\tn: number of columns, integer greater than 0\n");
}

/**
 * Struct for storing the result of parsing command line arguments
 *
 * success: parsing of command line arguments was successful
 * m: number of rows
 * n: number of columns
 */
struct ArgResult {
    bool success;
    int m;
    int n;
};
typedef struct ArgResult ArgResult_t;

/**
 * Parses this program's command line arguments.
 *
 * argc: number of command line arguments
 * argv: array of tokens
 *
 * return: arg result structure
 */
ArgResult_t* parse_args(int argc, char** argv) {
    ArgResult_t* result = (ArgResult_t*) malloc(sizeof(ArgResult_t));

    if (argc != 3) { // invalid number of arguments
        result->success = false;
        result->m = -1;
        result->n = -1;
        return result;
    }
    else {
        char* m_check = NULL;
        char* n_check = NULL;

        // parse row and column numbers
        long m = strtol(argv[1], &m_check, 10);
        long n = strtol(argv[2], &n_check, 10);

        if (m_check == argv[1] || n_check == argv[2] || m < 0 || n < 0) {
            result->success = false;
            result->m = -1;
            result->n = -1;
            return result;
        }
        else {
            result->success = true;
	        result->m = m;
	        result->n = n;
            return result;
        }
	}
}

/**
 * Struct for storing the result the payoff matrix entry
 *
 * success: entry of payoff matrix was successful
 * payoff: payoff matrix
 * m: number of rows
 * n: number of columns
 */
struct PayoffResult {
    bool success;
    double** payoff;
    int m;
    int n;
};
typedef struct PayoffResult PayoffResult_t;

/**
 * Frees a PayoffResult struct
 *
 * result: payoff result structure
 */
void free_payoff_result(PayoffResult_t* result) { 
    free_2d_arr((void**) result->payoff, result->m);
    free(result);
}

/**
 * Prompt user for payoff matrix and process its entry.
 *
 * m: number of rows
 * n: number of columns
 *
 * return: payoff result structure
 */
PayoffResult_t* get_payoff(int m, int n) {
    // initialize result struct
    PayoffResult_t* result = (PayoffResult_t*) malloc(sizeof(PayoffResult_t));
    result->payoff = (double**) calloc(m, sizeof(double*));
    result->m = m;
    result->n = n;

    printf("Enter the %d by %d payoff matrix below. Separate rows by new lines and columns by spaces: \n", m, n);
    
    int buffer_size = 1000;
    char* row_string = (char*) calloc(buffer_size, sizeof(char));
    for (int row = 0; row < m; row++) {
        result->payoff[row] = (double*) calloc(n, sizeof(double));

        // read in next row
        getline(&row_string, &buffer_size, stdin);

        char* to_tokenize = row_string;
        for (int col = 0; col < n; col++) {
            // get next token in row
            char* number_string = strtok(to_tokenize, " ");
            if (number_string == NULL) { // no token
                result->success = false;
                goto safe_exit;
            }

            // parse number from string
            char* number_check = NULL;
            long number = strtol(number_string, &number_check, 10);

            if (number_string == number_check) { // no number
                result->success = false;
                goto safe_exit;
            }
            else {
                // put number in matrix
                result->payoff[row][col] = (double) number;
            }

            to_tokenize = NULL;
        }
    }

    result->success = true; 
safe_exit:
    free(row_string);
    return result;
}

/**
 * Struct for storing a tableau
 *
 * m: matrix
 * s_size: length of S
 * x_size: length of X
 * rows: number of rows in m
 * cols: number of columns in m
 */
struct Tableau {
    double** m;
    int s_size;
    int x_size;
    int rows;
    int cols;
    int k;
};
typedef struct Tableau Tableau_t;

/**
 * Initialize a new tableau
 *
 * s_size: length of S
 * x_size: length of X
 */
Tableau_t* create_tableau(int s_size, int x_size) {
    Tableau_t* tableau = (Tableau_t*) malloc(sizeof(Tableau_t));
    tableau->s_size = s_size;
    tableau->x_size = x_size;
    tableau->rows = s_size + 1;
    tableau->cols = x_size + s_size + 1;
    tableau->k = 0;

    tableau->m = (double**) calloc(tableau->rows, sizeof(double*));
    for (int row = 0; row < tableau->rows; row++) {
        tableau->m[row] = (double*) calloc(tableau->cols, sizeof(double));
        for (int col = 0; col < tableau->cols; col++)
            tableau->m[row][col] = 0;
    }

    return tableau;
}

/**
 * Frees a tableau struct
 *
 * tableau: struct to free
 */
void free_tableau(Tableau_t* tableau) {
    free_2d_arr((void**) tableau->m, tableau->rows);
    free(tableau);
}

/**
 * Prints the tableau matrix
 *
 * tableau: struct to print
 */
void print_tableau(Tableau_t* tableau) {
    // build divider
    int length = (7 * tableau->cols) + 3;
    char* row_divider = (char*) calloc(length, sizeof(char));
    memset(row_divider, (int) '-', length - 1);

    for (int row = 0; row < tableau->rows; row++) {
        if (row == tableau->s_size) printf("%s\n", row_divider);

        for (int col = 0; col < tableau->cols; col++) {
            if (col == tableau->x_size || col == tableau->x_size + tableau->s_size) printf("|");
            printf("%6.2f ", tableau->m[row][col]);
        }
        printf("\n");
    }

    free(row_divider);
}

/**
 * Builds the initial tableau using the given payoff matrix
 *
 * payoff: payoff matrix
 * m: number of rows
 * n: number of columns
 *
 * return: initial tableau
 */
Tableau_t* get_init_tableau(double** payoff, int m, int n) {
    Tableau_t* tableau = create_tableau(m, n); 

    // find minimum payoff value
    double min = DBL_MAX;
    for (int row = 0; row < m; row++) {
        for (int col = 0; col < n; col++) {
            if (payoff[row][col] < min) min = payoff[row][col];
        }
    }
    double k = (min < 1)? 1 - min : 0;
    tableau->k = k;

    // populate initial tableau
    for (int row = 0; row < tableau->rows; row++) {
        for (int col = 0; col < tableau->cols; col++) {
            if (col < n) tableau->m[row][col] = (row < m)? payoff[row][col] + k : -1;
            else if (col < n + m) tableau->m[row][col] = (row < m)? (double) (row == col - n) : 0;
            else tableau->m[row][col] = (double) (row < m);
        }
    }

    return tableau;
}

/**
 * Struct to store a pivot result
 *
 * success: if the pivot was successful
 * tableau: tableau resulting from pivot\
 * pivot_row: row of the pivot used
 * pivot_col: col of the pivot used
 */
struct PivotResult {
    bool success;
    Tableau_t* tableau;
    int pivot_row;
    int pivot_col;
};
typedef struct PivotResult PivotResult_t;

/**
 * Frees a pivot result
 *
 * result: result to be freed
 */
void free_pivot_result(PivotResult_t* result) {
    free_tableau(result->tableau);
    free(result);
}

/**
 * Pivots the provided tableau
 *
 * tableau: struct to pivot
 */
PivotResult_t* pivot_tableau(Tableau_t* tableau) {
    PivotResult_t* result = (PivotResult_t*) malloc(sizeof(PivotResult_t));
    result->tableau = create_tableau(tableau->s_size, tableau->x_size);
    result->tableau->k = tableau->k;

    // find pivot column
    double min_value = 0; // trying to find most negative number
    int pivot_col = -1;

    for (int col = 0; col < tableau->cols; col++) {
        double value = tableau->m[tableau->rows - 1][col];
        if (value < min_value) {
            min_value = value;
            pivot_col = col;
        }
    }

    result->pivot_col = pivot_col;
    // bounds check for pivot column
    if (pivot_col < 0) {
        result->success = false;
        return result;
    }

    // find pivot row
    min_value = DBL_MAX; // finding smallest value
    int pivot_row = -1;

    for (int row = 0; row < tableau->rows; row++) {
        double value = tableau->m[row][tableau->cols - 1] / tableau->m[row][pivot_col];
        if (value > 0 && value < min_value) {
            min_value = value;
            pivot_row = row;
        }
    }

    result->pivot_row = pivot_row;
    // bounds check for pivot row
    if (pivot_row < 0) {
        result->success = false;
        return result;
    }

    // calculate new tableau
    double pivot_value = tableau->m[pivot_row][pivot_col];
    
    // update pivot row 
    for (int col = 0; col < tableau->cols; col++)
        result->tableau->m[pivot_row][col] = tableau->m[pivot_row][col] / pivot_value; // update based on other row rule

    // update other rows
    for (int row = 0; row < tableau->rows; row++) {
        for (int col = 0; col < tableau->cols; col++) {
            if (row != pivot_row) { // update based on other row rule
                result->tableau->m[row][col] = tableau->m[row][col] - \
                    (tableau->m[row][pivot_col] * result->tableau->m[pivot_row][col]);
            }
        }
    }

    result->success = true;
    return result;
}

struct TableauStore {
    Tableau_t** store;
    int size;
    int capacity;
};
typedef struct TableauStore TableauStore_t;

TableauStore_t* new_store() {
    TableauStore_t* store = (TableauStore_t*) malloc(sizeof(TableauStore_t));
    store->size = 0;
    store->capacity = 10;
    store->store = (Tableau_t**) calloc(store->capacity, sizeof(Tableau_t*));

    return store;
}

void free_store(TableauStore_t* store) {
    if (store != NULL) {
        for (int index = 0; index < store->size; index++) {
            if (store->store[index] != NULL) free_tableau(store->store[index]);
        }
        free(store->store);
        free(store);
    }
}

void add_tableau(TableauStore_t* store, Tableau_t* tableau) {
    store->store[store->size++] = tableau;
    if (store->size == store->capacity) {
        store->capacity *= 2;
        // TODO: not memory safe, add check here
        realloc(store->store, store->capacity);
    }
}

/**
 * Runs the simplex method on the supplied payoff matrix.
 *
 * argc: number of command line arguments
 * argv: array of tokens
 *
 * return: 0 on successful execution
 */
int main(int argc, char** argv) {
	ArgResult_t* parse_result = parse_args(argc, argv);
	if (parse_result->success) { // correct command line arguments
	    PayoffResult_t* payoff_result = get_payoff(parse_result->m, parse_result->n);

        if (payoff_result->success) { // valid payoff matrix 
            int m = payoff_result->m;
            int n = payoff_result->n;

            // initialize matrix to keep track of x variable orders
            int* order = (int*) calloc(n, sizeof(int));
            memset(order, -1, n * sizeof(int));
            
            // initialize tableau store
            TableauStore_t* store = new_store();

            Tableau_t* tableau = get_init_tableau(payoff_result->payoff, m, n);
            add_tableau(store, tableau);

            int pivot_count = 0;
            PivotResult_t* pivot_result = NULL;
            while (true) {
                pivot_result = pivot_tableau(tableau);
                if (pivot_result->success) {
                    if (pivot_count < n) order[pivot_count] = pivot_result->pivot_row;
                    
                    tableau = pivot_result->tableau;
                    add_tableau(store, tableau);
                    free(pivot_result); // dont free nested tableau
                }
                else {
                    free_pivot_result(pivot_result);
                    break;
                }

                pivot_count++;
            }

            // process the final tableau and determine strategies and value
            // note: tableau is the final tableau, can use it directly w/o store
            double v = tableau->m[tableau->rows - 1][tableau->cols - 1];
            double value = (1 / v) - tableau->k;

            double* p1_strategy = (double*) calloc(m, sizeof(double));
            double* p2_strategy = (double*) calloc(n, sizeof(double));

            for (int index = 0; index < m; index++)
                p1_strategy[index] = tableau->m[tableau->rows - 1][tableau->x_size + index] / v;

            for (int index = 0; index < n; index++) {
                int x_index = order[index];
                p2_strategy[index] = (x_index >= 0)? tableau->m[x_index][tableau->cols - 1] / v : 0;
            }

            // print results
            for (int index = 0; index < store->size; index++) {
                if (index == 0) printf("Initial Tableau:\n");
                else if (index == store->size - 1) printf("Final Tableau:\n");
                else printf("Tableau %d:\n", index);

                print_tableau(store->store[index]);
                printf("\n");
            }

            char separator[3] = "";
            printf("Player 1 Optimal Strategy: ( ");
            for (int index = 0; index < m; index++) {
                printf("%s%4.2f", separator, p1_strategy[index]);
                strcpy(separator, ", ");
            }
            printf(" )\n");

            strcpy(separator, "");
            printf("Player 2 Optimal Strategy: ( ");
            for (int index = 0; index < n; index++) {
                printf("%s%4.2f", separator, p2_strategy[index]);
                strcpy(separator, ", ");
            }
            printf(" )\n");

            printf("Value: %5.2f\n", value);

            free(p1_strategy);
            free(p2_strategy);
            free_store(store);
            free(order);
        }
        else { // invalid payoff matrix
            printf("Please enter %d valid integers on each line.\n", parse_result->n);
        }

        free_payoff_result(payoff_result);
    }
    else { // wrong usage
        print_usage();
    }

    free(parse_result);
    return 0;
}

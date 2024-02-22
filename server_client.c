/**
 *  ARRAY ADDITION SERVER
 *
 *  This program is a server that accepts two integer arrays from a client and
 *  adds them together. The server then sends the result back to the client.
 *
 *  @file server.c
 *  @date 2023-12-25
 *  @authors
 *    - Emrecan Karacayir
*/

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

//                          MACROS
#define              STR_HELPER(x) #x
#define                     STR(x) STR_HELPER(x)
//                       CONSTANTS
#define                  DATA_SIZE 100
#define                PORT_NUMBER 60000
#define                   PROTOCOL 0
#define                    BACKLOG 3
#define                    BASE_10 10
//                           RULES
#define                  DELIMITER " "
#define        MIN_ALLOWED_INTEGER 0
#define        MAX_ALLOWED_INTEGER 999
//                         PROMPTS
#define             WELCOME_PROMPT "\nHello, this is Array Addition Server!\n"
#define             F_ARRAY_PROMPT "\nPlease enter the first array for addition:\n"
#define             S_ARRAY_PROMPT "\nPlease enter the second array for addition:\n"
#define           RES_ARRAY_PROMPT "\nThe result of array addition are given below:\n"
#define             GOODBYE_PROMPT "\nThank you for Array Addition Server! Good Bye!\n"
//                 SERVER MESSAGES
#define          SV_SUCCESS_SOCKET "SUCCESS: Socket created."
#define            SV_SUCCESS_BIND "SUCCESS: Binding successful."
#define          SV_SUCCESS_LISTEN "SUCCESS: Listening for connections on port " STR(PORT_NUMBER) "..."
#define          SV_SUCCESS_ACCEPT "SUCCESS: Connection accepted."
#define            SV_ERROR_SOCKET "ERROR: Could not create socket!"
#define              SV_ERROR_BIND "ERROR: Could not bind socket!"
#define            SV_ERROR_LISTEN "ERROR: Could not listen!"
#define            SV_ERROR_ACCEPT "ERROR: Could not accept connection!"
#define              SV_ERROR_READ "ERROR: Could not read from socket!"
#define   SV_ERROR_CL_DISCONNECTED "ERROR: Client disconnected!"
#define    SV_ERROR_MALLOC_FAILURE "ERROR: Memory allocation failed!"
#define   SV_ERROR_THREAD_CREATION "ERROR: Could not create thread!"
#define       SV_ERROR_THREAD_JOIN "ERROR: Could not join thread!"
//                 CLIENT MESSAGES
#define  CL_WARNING_INPUT_OVERFLOW "\nWARNING: Input overflow detected, discarding extra bytes.\n"
#define CL_WARNING_OUTPUT_OVERFLOW "\nWARNING: Output overflow detected, discarding extra bytes.\n"
#define   CL_ERROR_INVALID_CONTENT "\nERROR: The inputted integer array contains non-integer characters. You must input only integers and empty spaces to separate inputted integers!\n"
#define   CL_ERROR_LENGTH_MISMATCH "\nERROR: The number of integers are different for both arrays. You must send equal number of integers for both arrays!\n"

char      INPUT_STRING[DATA_SIZE] = "";
char     OUTPUT_STRING[DATA_SIZE] = "";
int        FIRST_ARRAY[DATA_SIZE] = { 0 };
int       SECOND_ARRAY[DATA_SIZE] = { 0 };
int        CARRY_ARRAY[DATA_SIZE] = { 0 };
int       RESULT_ARRAY[DATA_SIZE] = { 0 };
pthread_t THREAD_ARRAY[DATA_SIZE] = { 0 };

typedef struct thread_args
{
  int index;
  int* first_array;
  int* second_array;
  int* carry_array;
  int* result_array;
} thread_args;

bool  read_to_buffer(int socket, char* buffer);
bool validate_buffer(int socket, char* buffer, int* item_count_ptr);
void      fill_array(int* array, char* buffer, int item_count);
void      add_arrays(void* args);
bool  handle_carries(int* result_array, int* carry_array, int item_count);

int main()
{
  // Try to create a socket
  int server_socket = socket(AF_INET, SOCK_STREAM, PROTOCOL);

  // Check if socket was created successfully. If not, exit.
  if (server_socket == -1)
  {
    puts(SV_ERROR_SOCKET);
    close(server_socket);
    exit(EXIT_FAILURE);
  }
  puts(SV_SUCCESS_SOCKET);

  // Create a sockaddr_in struct for the proper port and IP
  struct sockaddr_in server_socket_address = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port = htons(PORT_NUMBER)
  };

  // Bind the socket to the port and IP
  if (bind(server_socket, (struct sockaddr*)&server_socket_address, sizeof(server_socket_address)) < 0)
  {
    puts(SV_ERROR_BIND);
    close(server_socket);
    exit(EXIT_FAILURE);
  }
  puts(SV_SUCCESS_BIND);

  // Listen for incoming connections
  if (listen(server_socket, BACKLOG) == -1)
  {
    puts(SV_ERROR_LISTEN);
    close(server_socket);
    exit(EXIT_FAILURE);
  }
  // Accept incoming connections
  puts(SV_SUCCESS_LISTEN);

  // Create a new socket for the accepted connection
  struct sockaddr_in client_socket_address;
  socklen_t client_socket_size = sizeof(client_socket_address);
  int client_socket = accept(server_socket, (struct sockaddr*)&client_socket_address, &client_socket_size);
  if (client_socket == -1)
  {
    puts(SV_ERROR_ACCEPT);
    close(client_socket);
    close(server_socket);
    exit(EXIT_FAILURE);
  }
  puts(SV_SUCCESS_ACCEPT);

  // Write welcome prompt
  write(client_socket, WELCOME_PROMPT, strlen(WELCOME_PROMPT));

  // Declare first array item count
  int f_array_item_count = 0;

  // Get first array
  bool f_array_input_valid = false;
  while (!f_array_input_valid)
  {
    // Write first array prompt
    write(client_socket, F_ARRAY_PROMPT, strlen(F_ARRAY_PROMPT));

    // Try to read input from socket
    if (read_to_buffer(client_socket, INPUT_STRING))
    {
      // Validate input
      f_array_input_valid = validate_buffer(client_socket, INPUT_STRING, &f_array_item_count);
    }
  }

  // Fill first array
  fill_array(FIRST_ARRAY, INPUT_STRING, f_array_item_count);

  // Declare second array item count
  int s_array_item_count = 0;

  // Get second array
  bool s_array_input_valid = false;
  while (!s_array_input_valid)
  {
    // Write second array prompt
    write(client_socket, S_ARRAY_PROMPT, strlen(S_ARRAY_PROMPT));

    // Try to read input from socket
    if (read_to_buffer(client_socket, INPUT_STRING))
    {
      // Validate input
      s_array_input_valid = validate_buffer(client_socket, INPUT_STRING, &s_array_item_count);

      // Check if the item counts match
      if (s_array_input_valid && f_array_item_count != s_array_item_count)
      {
        // Item counts do not match
        write(client_socket, CL_ERROR_LENGTH_MISMATCH, strlen(CL_ERROR_LENGTH_MISMATCH));
        s_array_input_valid = false;
      }
    }
  }

  // Fill second array
  fill_array(SECOND_ARRAY, INPUT_STRING, s_array_item_count);

  // Add arrays with threads
  for (int i = 0; i < f_array_item_count; i++)
  {
    // Create thread arguments
    thread_args* args = malloc(sizeof(thread_args));

    // Check if memory allocation was successful
    if (args == NULL)
    {
      // Memory allocation failed
      puts(SV_ERROR_MALLOC_FAILURE);
      close(client_socket);
      close(server_socket);
      exit(EXIT_FAILURE);
    }

    // Fill thread arguments
    args->index = i;
    args->first_array = FIRST_ARRAY;
    args->second_array = SECOND_ARRAY;
    args->carry_array = CARRY_ARRAY;
    args->result_array = RESULT_ARRAY;

    // Create a new thread
    if (pthread_create(&THREAD_ARRAY[i], NULL, (void*)add_arrays, (void*)args) != 0)
    {
      // Thread creation failed
      puts(SV_ERROR_THREAD_CREATION);
      close(client_socket);
      close(server_socket);
      exit(EXIT_FAILURE);
    }
  }

  // Wait for threads to finish
  for (int i = 0; i < f_array_item_count; i++)
  {
    // Join thread
    if (pthread_join(THREAD_ARRAY[i], NULL) != 0)
    {
      // Thread join failed
      puts(SV_ERROR_THREAD_JOIN);
      close(client_socket);
      close(server_socket);
      exit(EXIT_FAILURE);
    }
  }

  // Handle carries
  if (handle_carries(RESULT_ARRAY, CARRY_ARRAY, f_array_item_count))
  {
    // Carry array[0] was 1, so result array was shifted
    f_array_item_count++;
  }

  // Write result to output string
  for (int i = 0; i < f_array_item_count; i++)
  {
    char temp[4];
    sprintf(temp, "%d ", RESULT_ARRAY[i]);
    strcat(OUTPUT_STRING, temp);

    /**
     *  - Check if buffer is full and there is more to write, give warning and
     *    break from loop.
     *  - We subtract sizeof(int) and 1 from DATA_SIZE because we need to leave
     *    space for the newline character and the null terminator.
     *  - We do this in runtime because we do not know the size of the integers.
     *  - If integer size is 4, than the worst case scenario is:
     *    (# for byte, | for check, _ for empty space)
     *    -> ... # | # # # _ _ ] : This leaves 2 bytes, 1 for newline and 1 for
     *                             null terminator.
    */
    if (strlen(OUTPUT_STRING) >= (DATA_SIZE - sizeof(int) - 1) && i != f_array_item_count - 1)
    {
      // Prompt user that output is overflowing
      write(client_socket, CL_WARNING_OUTPUT_OVERFLOW, strlen(CL_WARNING_OUTPUT_OVERFLOW));
      break;
    }
  }

  // Add newline to output string
  strcat(OUTPUT_STRING, "\n\0");

  // Write result array prompt
  write(client_socket, RES_ARRAY_PROMPT, strlen(RES_ARRAY_PROMPT));

  // Write result to socket
  write(client_socket, OUTPUT_STRING, strlen(OUTPUT_STRING));

  // Write goodbye prompt
  write(client_socket, GOODBYE_PROMPT, strlen(GOODBYE_PROMPT));

  // Close sockets
  close(client_socket);
  close(server_socket);
  return 0;
}

bool read_to_buffer(int socket, char* buffer)
{
  // Clear buffer
  memset(buffer, 0, DATA_SIZE);

  // Read from socket
  ssize_t num_bytes_read = read(socket, buffer, DATA_SIZE - 1);

  // Check read status
  if (num_bytes_read == -1)
  {
    // Read failed
    puts(SV_ERROR_READ);
    return false;
  }
  else if (num_bytes_read == 0)
  {
    // Client disconnected
    puts(SV_ERROR_CL_DISCONNECTED);
    return false;
  }
  else
  {
    // Check for overflow
    if (num_bytes_read == DATA_SIZE - 1 && buffer[num_bytes_read - 1] != '\n')
    {
      // Overflow detected
      write(socket, CL_WARNING_INPUT_OVERFLOW, strlen(CL_WARNING_INPUT_OVERFLOW));

      // Drain the socket
      char c;
      while (read(socket, &c, 1) > 0 && c != '\n');
    }

    // Remove non-printable characters
    int new_index = 0;
    for (int old_index = 0; old_index < num_bytes_read; old_index++)
    {
      if ((unsigned char)buffer[old_index] >= 32 && (unsigned char)buffer[old_index] != 127)
      {
        buffer[new_index++] = buffer[old_index];
      }
    }

    // Add null terminator
    buffer[new_index] = '\0';
  }
  return true;
}

bool validate_buffer(int socket, char* buffer, int* item_count_ptr)
{
  // Make a copy of the buffer
  char* buffer_copy = strdup(buffer);

  // Split the buffer into tokens
  char* save_ptr;
  char* token = strtok_r(buffer_copy, DELIMITER, &save_ptr);
  int item_count = 0;
  while (token != NULL)
  {
    // Check if the token is a number
    char* end_ptr;
    long number = strtol(token, &end_ptr, BASE_10);
    if (*end_ptr != '\0' || number < MIN_ALLOWED_INTEGER || number > MAX_ALLOWED_INTEGER)
    {
      // The token is not a valid number
      write(socket, CL_ERROR_INVALID_CONTENT, strlen(CL_ERROR_INVALID_CONTENT));
      free(buffer_copy);
      return false;
    }
    item_count++;
    token = strtok_r(NULL, DELIMITER, &save_ptr);
  }

  // Set item count
  *item_count_ptr = item_count;

  // Free buffer copy
  free(buffer_copy);
  return true;
}

void fill_array(int* array, char* buffer, int item_count)
{
  // Split the buffer into tokens
  char* save_ptr;
  char* token = strtok_r(buffer, DELIMITER, &save_ptr);
  for (int i = 0; i < item_count; i++)
  {
    // Convert token to integer
    array[i] = (int)strtol(token, NULL, BASE_10);
    token = strtok_r(NULL, DELIMITER, &save_ptr);
  }
}

void add_arrays(void* args)
{
  // Get thread arguments
  thread_args* t_args = (thread_args*)args;
  int index = t_args->index;
  int* first_array = t_args->first_array;
  int* second_array = t_args->second_array;
  int* carry_array = t_args->carry_array;
  int* result_array = t_args->result_array;

  // Add the numbers
  int sum = first_array[index] + second_array[index];
  // Check if there is a carry
  if (sum > MAX_ALLOWED_INTEGER)
  {
    // There is a carry
    carry_array[index] = 1;
    sum -= MAX_ALLOWED_INTEGER + 1;
  }
  // Store the result
  result_array[index] = sum;

  // Free thread arguments
  free(args);

  // Exit thread
  pthread_exit(NULL);
}

bool handle_carries(int* result_array, int* carry_array, int item_count)
{
  /**
   *  1. We start from right and go left in the arrays.
   *  2. We always look carry from the previous index. (for index result_array[i],
   *     we look at index carry_array[i + 1])
   *  3. Since the last integer has nowhere to get a carry from, we start from
   *     the second last item. (i = item_count - 2)
   *  4. If the current integer has a carry, @see #2, we add 1 to the current
   *     integer. (result_array[i] += 1)
   *  5. If that addition causes a carry, we set 1 to the carry array's current
   *     index. (carry_array[i] = 1)
   *  6. If that addition does not cause a carry, we set 0 to the carry array's
   *     current index. (carry_array[i] = 0)
   *  7. We set previous carry to 0. (carry_array[i + 1] = 0)
   *  8. We repeat this process until we reach the start of the result array.
   *  9. Lastly, we check if carry_array[0] is 1. If it is, we shift the result
   *     array to the right by 1 and set the first item to 1. (result_array[0] = 1)
   * 10. If result array is shifted, we return true. Else, we return false.
  */
  for (int i = item_count - 2; i >= 0; i--)
  {
    if (carry_array[i + 1] == 1)
    {
      result_array[i] += 1;
      if (result_array[i] > MAX_ALLOWED_INTEGER)
      {
        carry_array[i] = 1;
        result_array[i] -= MAX_ALLOWED_INTEGER + 1;
      }
      carry_array[i + 1] = 0;
    }
  }
  if (carry_array[0] == 1)
  {
    for (int i = item_count - 1; i >= 0; i--)
    {
      result_array[i + 1] = result_array[i];
    }
    result_array[0] = 1;
    return true;
  }
  return false;
}

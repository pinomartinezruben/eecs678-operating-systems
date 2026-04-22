/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "quash.h"
IMPLEMENT_DEQUE(pid_deque, pid_t);
IMPLEMENT_DEQUE(job_deque, Job);

// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
  fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__)

static int pipes[2][2];
static int pipe_index = 0;
static job_deque background_jobs;
static int next_job_id = 1;
static bool deques_initialized = false;

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  // TODO: Get the current working directory. This will fix the prompt path.
  // HINT: This should be pretty simple
  // IMPLEMENT_ME();
  char* cwd = getcwd(NULL, 0);

  // Change this to true if necessary
  *should_free = true;

  return cwd;
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  // TODO: Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  // HINT: This should be pretty simple
  // IMPLEMENT_ME();

  // TODO: Remove warning silencers
  // me--(void) env_var; // Silence unused variable warning

  return getenv(env_var);
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // TODO: Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.
  if (!deques_initialized || is_empty_job_deque(&background_jobs))
    return;

  size_t num_jobs = length_job_deque(&background_jobs);
  for (size_t i = 0; i < num_jobs; i++){
    //need to define popfront
    Job job = pop_front_job_deque(&background_jobs); 
    bool all_done = true;
  

    size_t num_pids = length_pid_deque(&job.pids);
    for (size_t j = 0; j < num_pids; j++){
      pid_t pid = pop_front_pid_deque(&job.pids);
      int status;

      if (waitpid(pid, &status, WNOHANG) == 0){
        all_done = false;
        push_back_pid_deque(&job.pids, pid);
      }
    }
  
  

    if (all_done){
    // TODO: Once jobs are implemented, uncomment and fill the following line
    print_job_bg_complete(job.job_id, 0, job.cmd_string);
    destroy_pid_deque(&job.pids);
    free(job.cmd_string);
    }
    else{
      push_back_job_deque(&background_jobs,job);
    } 
  }
}


// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable


  //char* exec = cmd.args[0];
  //char** args = cmd.args;

  

  // TODO: Remove warning silencers
  // (void) exec; // Silence unused variable warning
  // (void) args; // Silence unused variable warning

  // TODO: Implement run generic
  // IMPLEMENT_ME();
  if (execvp(cmd.args[0], cmd.args) < 0){
    perror("ERROR: Failed to execute program");
  }

  
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;
  for (int i = 0; str[i] != NULL; i++){
    printf("%s%s", str[i], (str[i+1] != NULL) ? " " : "");
  }

  // TODO: Remove warning silencers
  // me--(void) str; // Silence unused variable warning

  // TODO: Implement echo
  // IMPLEMENT_ME();
  printf("\n");

  // Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable (Doni)
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

  // TODO: Remove warning silencers
  (void) env_var; // Silence unused variable warning
  (void) val;     // Silence unused variable warning

  // TODO: Implement export.
  // HINT: This should be quite simple.
  // IMPLEMENT_ME();
  if (cmd.env_var && cmd.val){
    setenv(cmd.env_var, cmd.val, 1);
  }
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  // Get the directory name
  const char* dir = cmd.dir;

  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }

  // TODO: Change directory
  if (chdir(dir) != 0){
    perror("ERROR: Failed to change directory");
    return;
  }

  // TODO: Update the PWD environment variable to be the new current working
  // directory and optionally update OLD_PWD environment variable to be the old
  // working directory.
  // IMPLEMENT_ME();
  char* new_path = getcwd(NULL, 0);
  setenv("PWD", new_path, 1);
  free(new_path);

}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  // TODO: Remove warning silencers
  (void) signal; // Silence unused variable warning
  (void) job_id; // Silence unused variable warning

  // TODO: Kill all processes associated with a background job
  // IMPLEMENT_ME();
  size_t num_jobs = length_job_deque(&background_jobs);
  for (size_t i = 0; i < num_jobs; i++) {
    Job job = pop_front_job_deque(&background_jobs);
    
    if (job.job_id == job_id) {
      // Found the job! Signal all PIDs in its deque
      size_t num_pids = length_pid_deque(&job.pids);
      for (size_t j = 0; j < num_pids; j++) {
        pid_t pid = pop_front_pid_deque(&job.pids);
        kill(pid, signal);
        push_back_pid_deque(&job.pids, pid); // Put it back so check_jobs can clean it up
      }
    }
    push_back_job_deque(&background_jobs, job);
  }
}


// Prints the current working directory to stdout
void run_pwd() {
  // TODO: Print the current working directory
  // IMPLEMENT_ME();
  bool should_free = false;
  char* cwd = get_current_directory(&should_free);

  printf("%s\n", cwd);

  if (should_free) {
    free(cwd);
  }

  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // TODO: Print background jobs
  // IMPLEMENT_ME();
  if (!deques_initialized || is_empty_job_deque(&background_jobs))
   return;

  size_t num_jobs = length_job_deque(&background_jobs);
  for (size_t i = 0; i < num_jobs; i++){
    Job job = pop_front_job_deque(&background_jobs);
    print_job(job.job_id, peek_front_pid_deque(&job.pids), job.cmd_string);
    push_back_job_deque(&background_jobs, job);
  }

  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;

  case ECHO:
    run_echo(cmd.echo);
    break;

  case PWD:
    run_pwd();
    break;

  case JOBS:
    run_jobs();
    break;

  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;

  case CD:
    run_cd(cmd.cd);
    break;

  case KILL:
    run_kill(cmd.kill);
    break;

  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
pid_t create_process(CommandHolder holder) {
  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  // TODO: Remove warning silencers
  // (void) p_in;  // Silence unused variable warning
  // (void) p_out; // Silence unused variable warning
  // (void) r_in;  // Silence unused variable warning
  // (void) r_out; // Silence unused variable warning
  // (void) r_app; // Silence unused variable warning

  // TODO: Setup pipes, redirects, and new process
  // IMPLEMENT_ME();

  //parent_run_command(holder.cmd); // This should be done in the parent branch of
                                  // a fork
  if (p_out) {
    if (pipe(pipes[pipe_index]) == -1){
      perror("pipe");
    }
  }

  pid_t pid = fork();

  //child_run_command(holder.cmd); // This should be done in the child branch of a fork
  if (pid == 0) { // CHILD PROCESS
    // --- Handle Pipes ---
    if (r_in) {
      // Read from the PREVIOUS pipe (the one we didn't just create)
      int in_fd = open(holder.redirect_in, O_RDONLY);
      if (in_fd < 0) {
        perror("open");
        exit(1);
      }
      dup2(in_fd, STDIN_FILENO);
      close(in_fd);
    }
    else if (p_in) {
      // Write to the CURRENT pipe
      dup2(pipes[1 - pipe_index][0], STDIN_FILENO);
      close(pipes[1 - pipe_index][0]);
    }

    // --- Handle Redirections ---
    if (r_out) {
      int flags = O_WRONLY | O_CREAT | (r_app ? O_APPEND : O_TRUNC);
      int out_fd = open(holder.redirect_out, flags, 0644);
      if (out_fd < 0){
        perror("open");
        exit(1);
      }
      dup2(out_fd, STDOUT_FILENO);
      close(out_fd);
    }else if (p_out){
      dup2(pipes[pipe_index][1], STDOUT_FILENO);
      close(pipes[pipe_index][1]);
    }
    
    if (p_out) close(pipes[pipe_index][0]);

    // Run the command
    child_run_command(holder.cmd);
    exit(0); 
  } 
  else { // PARENT PROCESS (Quash)
    // Close the write end of the current pipe; the next child will read from it
    if (p_out) close(pipes[pipe_index][1]);
    // Close the read end of the previous pipe; it's no longer needed
    if (p_in)  close(pipes[1 - pipe_index][0]);

    // Update index for the next process in the pipeline
    pipe_index = 1 - pipe_index;

    // TODO: Store the PID in a deque so we can wait for it in run_script
    parent_run_command(holder.cmd);

    return pid;
  }
}

// Run a list of commands
void run_script(CommandHolder* holders) {
  if (holders == NULL)
    return;

  if (!deques_initialized){
    background_jobs = new_job_deque(10);
    deques_initialized = true;
  }

  check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }

  // prepare a new job structure
  Job current_job;
  current_job.pids = new_pid_deque(1);
  current_job.cmd_string = strdup(get_command_string());
  pipe_index = 0;

  CommandType type;

  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i){
    pid_t child_pid = create_process(holders[i]);
    push_back_pid_deque(&current_job.pids, child_pid);
  }
    // create_process(holders[i]);

  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    // TODO: Wait for all processes under the job to complete
    // IMPLEMENT_ME();
    while (!is_empty_pid_deque(&current_job.pids)){
      pid_t pid = pop_front_pid_deque(&current_job.pids);
      int status;
      waitpid(pid, &status, 0); // this would wait for every process in the pipeline
    }
    destroy_pid_deque(&current_job.pids);
    free(current_job.cmd_string);
  }
  else {
    // A background job.
    // TODO: Push the new job to the job queue
    // IMPLEMENT_ME();
    current_job.job_id = next_job_id++;
    push_back_job_deque(&background_jobs, current_job);

    // TODO: Once jobs are implemented, uncomment and fill the following line
    print_job_bg_start(current_job.job_id, peek_front_pid_deque(&current_job.pids), current_job.cmd_string);
  }
}

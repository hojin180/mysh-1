#ifndef MYSH_COMMANDS_H_
#define MYSH_COMMANDS_H_

struct single_command
{
  int argc;
  char** argv;
};

int exec_com(struct single_command (*com));

int do_exec(char** argv, int argc);

int getFullPATH(char** argv);

void* socket_thread(void* argv);

void* print_bg_terminate(void* argv);

int evaluate_command(int n_commands, struct single_command (*commands)[512]);

void free_commands(int n_commands, struct single_command (*commands)[512]);

#endif // MYSH_COMMANDS_H_

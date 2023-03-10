#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define L_GREEN "\e[1;32m"
#define L_BLUE "\e[1;34m"
#define L_RED "\e[1;31m"
#define WHITE "\e[0m"

#define TRUE 1
#define FALSE 0
#define DEBUG

char lastdir[100];
char command[BUFSIZ];
char argv[100][100];
char **argvtmp1;
char **argvtmp2;
char argv_redirect[100];
int argc;
int BUILTIN_COMMAND = 0;
int PIPE_COMMAND = 0;
int REDIRECT_COMMAND = 0;

void set_prompt(char *prompt);

int analysis_command();
void builtin_command();
void do_command();

void help();
void initial();
void init_lastdir();
void history_setup();
void history_finish();
void display_history_list();

int main()
{
	char prompt[BUFSIZ];
	char *line;

	init_lastdir();
	history_setup();
	while (1)
	{
		set_prompt(prompt);
		if (!(line = readline(prompt)))
			break;
		if (*line)
			add_history(line);

		strcpy(command, line);
		
		if (!(analysis_command()))
		{
			
			if (BUILTIN_COMMAND)
			{
				builtin_command();
			} 
			else
			{
				do_command();
			}	   
		}		  
		initial();
	}			  
	history_finish();

	return 0;
}


void set_prompt(char *prompt)
{
	char hostname[100];
	char cwd[100];
	char super = '#';
	
	char delims[] = "/";
	struct passwd *pwp;

	if (gethostname(hostname, sizeof(hostname)) == -1)
	{
		
		strcpy(hostname, "unknown");
	} 
	pwp = getpwuid(getuid());
	if (!(getcwd(cwd, sizeof(cwd))))
	{
		
		strcpy(cwd, "unknown");
	} 
	char cwdcopy[100];
	strcpy(cwdcopy, cwd);
	char *first = strtok(cwdcopy, delims);
	char *second = strtok(NULL, delims);
	
	if (!(strcmp(first, "home")) && !(strcmp(second, pwp->pw_name)))
	{
		int offset = strlen(first) + strlen(second) + 2;
		char newcwd[100];
		char *p = cwd;
		char *q = newcwd;

		p += offset;
		while (*(q++) = *(p++))
			;
		char tmp[100];
		strcpy(tmp, "~");
		strcat(tmp, newcwd);
		strcpy(cwd, tmp);
	}

	if (getuid() == 0) 
		super = '#';
	else
		super = '%';
	sprintf(prompt, "\001\e[1;32m\002%s@%s\001\e[0m\002:\001\e[1;31m\002%s\001\e[0m\002%c", pwp->pw_name, hostname, cwd, super);
}


int analysis_command()
{
	int i = 1;
	char *p;
	
	char delims[] = " ";
	argc = 1;

	strcpy(argv[0], strtok(command, delims));
	while (p = strtok(NULL, delims))
	{
		strcpy(argv[i++], p);
		argc++;
	} 

	if (!(strcmp(argv[0], "quit")) || !(strcmp(argv[0], "help")) || !(strcmp(argv[0], "cd")) || !(strcmp(argv[0], "history")))
	{
		BUILTIN_COMMAND = 1;
	}
	int j;
	
	int pipe_location;
	for (j = 0; j < argc; j++)
	{
		if (strcmp(argv[j], "|") == 0)
		{
			PIPE_COMMAND = 1;
			pipe_location = j;
			break;
		}
	} 


	int redirect_location;
	for (j = 0; j < argc; j++)
	{
		if (strcmp(argv[j], ">") == 0)
		{
			REDIRECT_COMMAND = 1;
			redirect_location = j;
			break;
		}
	} 

	if (PIPE_COMMAND)
	{
		
		argvtmp1 = malloc(sizeof(char *) * pipe_location + 1);
		int i;
		for (i = 0; i < pipe_location + 1; i++)
		{
			argvtmp1[i] = malloc(sizeof(char) * 100);
			if (i <= pipe_location)
				strcpy(argvtmp1[i], argv[i]);
		} 
		argvtmp1[pipe_location] = NULL;

		
		argvtmp2 = malloc(sizeof(char *) * (argc - pipe_location));
		int j;
		for (j = 0; j < argc - pipe_location; j++)
		{
			argvtmp2[j] = malloc(sizeof(char) * 100);
			if (j <= pipe_location)
				strcpy(argvtmp2[j], argv[pipe_location + 1 + j]);
		} 
		argvtmp2[argc - pipe_location - 1] = NULL;

	} 
	else if (REDIRECT_COMMAND)
	{
		strcpy(argv_redirect, argv[redirect_location + 1]);
		argvtmp1 = malloc(sizeof(char *) * redirect_location + 1);
		int i;
		for (i = 0; i < redirect_location + 1; i++)
		{
			argvtmp1[i] = malloc(sizeof(char) * 100);
			if (i < redirect_location)
				strcpy(argvtmp1[i], argv[i]);
		} 
		argvtmp1[redirect_location] = NULL;

	} 

	else
	{
		argvtmp1 = malloc(sizeof(char *) * argc + 1);
		int i;
		for (i = 0; i < argc + 1; i++)
		{
			argvtmp1[i] = malloc(sizeof(char) * 100);
			if (i < argc)
				strcpy(argvtmp1[i], argv[i]);
		} 
		argvtmp1[argc] = NULL;
	}

#ifdef DEBUG
	
	if (BUILTIN_COMMAND)
	{
		printf("\tthis is a builtin command: %s\n", argv[0]);
	}
	else if (PIPE_COMMAND)
	{
		printf("\tthis is a pipe command:\n");
		printf("\t==command 1:\n");
		int k;
		for (k = 0; k < pipe_location + 1; k++)
		{
			printf("\t%d: %s\n", k, argvtmp1[k]);
		} 
		printf("\t==command 2:\n");
		for (k = 0; k < argc - pipe_location; k++)
		{
			printf("\t%d: %s\n", k, argvtmp2[k]);
		} 
	}
	else if (REDIRECT_COMMAND)
	{
		printf("\tthis is a redirect command:\n");
		printf("\t==command:\n");
		int k;
		for (k = 0; k < pipe_location + 1; k++)
		{
			printf("\t%d: %s\n", k, argvtmp1[k]);
		} 
		printf("redirect target: %s\n", argv_redirect);
	}
	else
	{
		printf("\n\tthe command is:%s with %d parameter(s):\n", argv[0], argc);
		printf("0(command): %s\n", argv[0]);
		int k;
		for (k = 1; k < argc; k++)
		{
			printf("%d: %s\n", k, argv[k]);
		} 
	}

#endif

	return 0;
}

void builtin_command()
{
	struct passwd *pwp;
	
	if (strcmp(argv[0], "quit") == 0)
	{
		history_finish();
		exit(EXIT_SUCCESS);
	}
	else if (strcmp(argv[0], "help") == 0)
	{
		help();
	}
	else if (!strcmp(argv[0], "history"))
	{
		display_history_list();
	}

	else if (strcmp(argv[0], "cd") == 0)
	{
		char cd_path[100];
		if ((strlen(argv[1])) == 0)
		{
			pwp = getpwuid(getuid());
			sprintf(cd_path, "/home/%s", pwp->pw_name);
			strcpy(argv[1], cd_path);
			argc++;
		}
		else if ((strcmp(argv[1], "~") == 0))
		{
			pwp = getpwuid(getuid());
			sprintf(cd_path, "/home/%s", pwp->pw_name);
			strcpy(argv[1], cd_path);
		}

		
#ifdef DEBUG
		printf("cdpath = %s \n", argv[1]);
#endif
		if ((chdir(argv[1])) < 0)
		{
			printf("cd failed in builtin_command()\n");
		}
	} 
}

void do_command()
{
	

	if (PIPE_COMMAND)
	{
		int fd[2], res;
		int status;

		res = pipe(fd);

		if (res == -1)
			printf("pipe failed in do_command()\n");
		pid_t pid1 = fork();
		if (pid1 == -1)
		{
			printf("fork failed in do_command()\n");
		} 
		else if (pid1 == 0)
		{
			dup2(fd[1], 1); 
			close(fd[0]);	
			if (execvp(argvtmp1[0], argvtmp1) < 0)
			{
#ifdef DEBUG
				printf("execvp failed in do_command() !\n");
#endif
				printf("%s:command not found\n", argvtmp1[0]);
			} 
		}	
		else
		{
			waitpid(pid1, &status, 0);
			pid_t pid2 = fork();
			if (pid2 == -1)
			{
				printf("fork failed in do_command()\n");
			}
			else if (pid2 == 0)
			{
				close(fd[1]);	
				dup2(fd[0], 0); 
				if (execvp(argvtmp2[0], argvtmp2) < 0)
				{
#ifdef DEBUG
					printf("execvp failed in do_command() !\n");
#endif
					printf("%s:command not found\n", argvtmp2[0]);
				} 
			}	  
			else
			{
				close(fd[0]);
				close(fd[1]);
				waitpid(pid2, &status, 0);
			} 
		}	
	}		  

	else if (REDIRECT_COMMAND)
	{

		pid_t pid = fork();
		if (pid == -1)
		{
			printf("fork failed in do_command()\n");
		} 
		else if (pid == 0)
		{
			char text[] = "";
			int f;

			if ((f = creat(argv_redirect, S_IRUSR | S_IWUSR)) < 0)
				perror("creat() error");
			else
			{
				write(f, text, strlen(text));
				close(f);
				unlink(argv_redirect);
			}

			int redirect_flag = 0;
			FILE *fstream;
			fstream = fopen(argv_redirect, "w+");
			if (fstream == NULL)
			{
				printf("failed to open file");
			}
			freopen(argv_redirect, "w", stdout);
			if (execvp(argvtmp1[0], argvtmp1) < 0)
			{
				redirect_flag = 1; 
			}					   
			printf("ran successfully");
			fclose(stdout);
			fclose(fstream);
			if (redirect_flag)
			{
#ifdef DEBUG
				printf("execvp redirect command failed in do_command() !\n");
#endif
				printf("%s:command not found\n", argvtmp1[0]);
			} 

		} 
		else
		{
			int pidReturn = wait(NULL);
		} 
		}
	else
	{
		pid_t pid = fork();
		if (pid == -1)
		{
			printf("fork failed in do_command()\n");
		} 
		else if (pid == 0)
		{
			if (execvp(argvtmp1[0], argvtmp1) < 0)
			{
#ifdef DEBUG
				printf("execvp failed in do_command() !\n");
#endif
				printf("%s:command not found\n", argvtmp1[0]);
			} 
			
		}
		else
		{
			int pidReturn = wait(NULL);
		} 
	}	  

	free(argvtmp1);
	free(argvtmp2);
}

void help()
{
	char message[50] = "Hi,welcome to myShell!";
	printf(
		"< %s >\n"
		"\t\t\\\n"
		"\t\t \\   \\_\\_    _/_/\n"
		"\t\t  \\      \\__/\n"
		"\t\t   \\     (oo)\\_______\n"
		"\t\t    \\    (__)\\       )\\/\\\n"
		"\t\t             ||----w |\n"
		"\t\t             ||     ||\n",
		message);
}

void initial()
{
	int i = 0;
	for (i = 0; i < argc; i++)
	{
		strcpy(argv[i], "\0");
	}
	argc = 0;
	BUILTIN_COMMAND = 0;
	PIPE_COMMAND = 0;
	REDIRECT_COMMAND = 0;
}

void init_lastdir()
{
	getcwd(lastdir, sizeof(lastdir));
}

void history_setup()
{
	using_history();
	stifle_history(50);
	read_history("/tmp/msh_history");
}

void history_finish()
{
	append_history(history_length, "/tmp/msh_history");
	history_truncate_file("/tmp/msh_history", history_max_entries);
}

void display_history_list()
{
	HIST_ENTRY **h = history_list();
	if (h)
	{
		int i = 0;
		while (h[i])
		{
			printf("%d: %s\n", i, h[i]->line);
			i++;
		}
	}
}

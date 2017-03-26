#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 80
#define HINDEX 10

int cycle = 0;

struct termios settings;

/* Switch to canonical mode */
void reset_term_mode(void)
{
	tcsetattr (STDIN_FILENO, TCSANOW, &settings);
}

/* Switch to noncanonical mode */
void set_input_mode(void) 
{
    tcgetattr(STDIN_FILENO, &settings);
    atexit(reset_term_mode);

    struct termios term;

    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON|ECHO);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

/* Parse user input */
void parse(char *input, char **argv)
{
	while(*input != '\0')
	{ 
		while(*input == ' ' || *input == '\t' || *input == '\n')
			*input++ = '\0';
		*argv++ = input;    
        while(*input != '\0' && *input != ' ' && *input != '\t' && *input != '\n') 
			input++;        
	}
	*argv = '\0';           
}

/* Excute user input through execvp */
void execute(char **argv)
{
	pid_t pid;
	int status;
    
	/* Error handling */
	if((pid = fork()) < 0)
	{
		printf("Could not create child\n");
        	exit(1);
    	}
    
	/* Child executes command */
	else if(pid == 0) 
	{
		if(execvp(*argv, argv) < 0)
		{
			printf("Command not recognized\n");
			exit(1);
		}
	}

	/* Parent waits */
	else 
	{
		while (wait(&status) != pid)
			;
	}
}

/* Retrieve previously entered commands */
void history(char *hist[], int current, char *input, char **argv)
{
	int n = 0;

	/* Most recent command */
	if(strcmp(++input, "!") == 0)
	{
		/* If 10 or fewer commands have been entered */
		if(cycle == 0)
		{
			if(current - 1 < 0)
				printf("No commands entered\n");
			else
			{
				printf("%s\n", hist[(current-1)]);
				parse(hist[(current-1)], argv);
				execute(argv);
			}
		}

		/* If more than 10 commands have been entered */
		else
		{
			if(current - 1 < 0)
			{
				printf("%s\n", hist[9]);
				parse(hist[9], argv);
				execute(argv);
			}
			else
			{
				printf("%s\n", hist[(current-1) % HINDEX]);
				parse(hist[(current-1)], argv);
				execute(argv);
			}
		}
	}
	
	/* nth command */
	else
	{
		n = atoi(input);
		/* If 10 or fewer commands have been entered */
		if(cycle == 0)
		{
			if(n < 1 || n > 10 || hist[n-1] == NULL)
				printf("Nothing found\n");
			else
			{
				printf("%s\n", hist[n-1]);
				parse(hist[n-1], argv);
				execute(argv);
			}
		}

		/* If 10 or more commands have been entered */
		else
		{
			printf("%s\n", hist[(current+n-1) % HINDEX]);
			parse(hist[(current+n-1)], argv);
			execute(argv);
		}
	}
}     

/* Arrow key navigation of history */
void arrow(char *hist[], char **argv)
{	
	int track;
	int threshold = 9;

	/* Find most recent command */
	while(hist[threshold] == NULL)
	{
		threshold--;
		if(threshold == 0)
			break;
	}
	
	track = threshold + 1;

	/* Switch to noncanonical mode */
	set_input_mode();
	char check;

	while(1)
	{
		/* Scan for arrow key input */
		scanf("%c", &check);
		if(check == '\033')
		{
			scanf("%c", &check);
			scanf("%c", &check);

			/* Pressed up */
			if(check == 'A')
			{
				/* As long as you haven't reached the oldest command stored */
				if(track != 0)
				{
					/* Move to an older command */
					if(track != threshold + 1)
					{
						
						for(int i=0; i < strlen(hist[track]); i++)
							printf("\b");
					}
					track--;
					printf("%s", hist[track]);
				}

				/* Do nothing if you already reached the oldest command */
				else
				{
					;
				}
				
			}

			/* Pressed down */
			else if(check == 'B')
			{
				/* Do nothing if you already reached the most recent command */
				if(track >= threshold)
				{
					;
				}

				/* Move to a more recent command */
				else
				{
					
					for(int i=0; i < strlen(hist[track]); i++)
						printf("\b");						
					track++;
					printf("%s", hist[track]);
				}
				
			}
		}

		/* Pressed enter */
		else if(check == '\n')
		{
			/* Switch to canonical mode and execute command */
			printf("\n");
			reset_term_mode();
			if(hist[track] != NULL)
			{
				parse(hist[track], argv);
				execute(argv);
			}
			break;
		}

		else
			;
	}
}


void main(void)
{
	char input[MAX_LINE];
	char *argv[MAX_LINE];
	char *hist[HINDEX];

	int current = 0;

	/* Array of command history */
	for(int i = 0; i < HINDEX; i++)
		hist[i] = NULL;

	while (1)
	{
		printf("osh> ");
		
		/* Get user input */
		fgets(input, MAX_LINE, stdin);
		if(input[strlen(input) - 1] == '\n')
			input[strlen(input) - 1] = '\0';

		/* Record input unless asking for history */
		if(strncmp(input, "!", 1) != 0 && strncmp(input, "\033", 1) != 0 && strcmp(input, "arrow") != 0)
		{
			free(hist[current]);
			hist[current] = strdup(input);
			if(current == 9)
				cycle = 1;
			current = (current + 1) % HINDEX;
		}
		
		/* Fetch history */
		if(strncmp(input, "!", 1) == 0)
			history(hist, current, input, argv);

		/* Noncanonical mode */
		else if(strcmp(input, "arrow") == 0)
			arrow(hist, argv);

		/* Batch processing */
		else if(strcmp(input, "test.sh") == 0)
		{
			FILE *file = fopen(input, "r");

			if (file != NULL)
			{
				char line [MAX_LINE];
				fgets(line, sizeof  line, file);
		
				while(fgets(line, sizeof line, file) != NULL)
				{							
					parse(line, argv);
					execute(argv);	
				}
			
				fclose(file);
			}
			else
				perror(input);
		}

		/* Quit */
		else if(strcmp(input, "quit") == 0)
			break;
			
		else
		{
			parse(input, argv);
			execute(argv);
		}          
	}
}

                


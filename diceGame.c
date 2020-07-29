#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>

void action(){}; // to avoid quitting when receiving a SIGUSR1
void player(char *name, int playerId, int fd);
void checkWinner(int fd, char *name);

int main(int argc, char *argv[])
{
    int fd;
    pid_t pid1, pid2, pid3;
    printf("DiceGame: a 3-players game with a referee\n\n");

    // Creating the binary file before forking
    if ((fd = open("sharedFile.bin", O_CREAT | O_WRONLY | O_TRUNC, 0777)) == -1)
    {
        perror("File problem");
        exit(1);
    }
    else
    {
        // Writing three zero-integer values to the file
        int *threeZeros;
        threeZeros = malloc(3 * sizeof(int));
        threeZeros[0] = 0;
        threeZeros[1] = 0;
        threeZeros[2] = 0;
        write(fd, threeZeros, 3 * sizeof(int));
        close(fd);
        free(threeZeros);
    }

    // Creating the players and calling the common function "player"
    if ((pid1 = fork()) == 0)
        player("TATA", 1, fd);
    if ((pid2 = fork()) == 0)
        player("TITI", 2, fd);
    if ((pid3 = fork()) == 0)
        player("TOTO", 3, fd);
    sleep(1);
    signal(SIGUSR1, action);
    while (1)
    {
        // Make the players play in order: TATA, TITI then TOTO
        fd = open("sharedFile.bin", O_RDONLY);
        checkWinner(fd, "TATA");
        printf("Referee: TATA plays\n\n");
        kill(pid1, SIGUSR1);
        pause();
        checkWinner(fd, "TITI");
        printf("Referee: TITI plays\n\n");
        kill(pid2, SIGUSR1);
        pause();
        checkWinner(fd, "TOTO");
        printf("Referee: TOTO plays\n\n");
        kill(pid3, SIGUSR1);
        pause();
    }
}

void player(char *name, int playerId, int fd)
{
    fd = open("sharedFile.bin", O_RDONLY);
    int points = 0, dice, oldScore[1];
    long int ss = 0;
    while (1)
    {
        signal(SIGUSR1, action);
        pause();
        // Reading the old score
        if (playerId == 1) // TATA
        {
            lseek(fd, 0, SEEK_SET);
            read(fd, oldScore, sizeof(int));
            printf("TATA: my old score is: %d\n", oldScore[0]);
        }
        else if (playerId == 2) // TITI
        {
            lseek(fd, sizeof(int), SEEK_SET);
            read(fd, oldScore, sizeof(int));
            printf("TITI: my old score is: %d\n", oldScore[0]);
        }
        else // TOTO
        {
            lseek(fd, 2 * sizeof(int), SEEK_SET);
            read(fd, oldScore, sizeof(int));
            printf("TOTO: my old score is: %d\n", oldScore[0]);
        }
        close(fd);
        // Playing the dice and printing its own name and the dice score
        printf("%s: I'm playing my dice\n", name);
        dice = (int)time(&ss) % 10 + 1;
        printf("%s: I got %d points\n\n", name, dice);
        // Updating the old score
        int old = oldScore[0];
        oldScore[0] = old + dice;
        // Writing the new score at the same file offset
        fd = open("sharedFile.bin", O_WRONLY);
        if (playerId == 1) // TATA
        {
            lseek(fd, 0, SEEK_SET);
            write(fd, oldScore, sizeof(int));
        }
        else if (playerId == 2) // TITI
        {
            lseek(fd, sizeof(int), SEEK_SET);
            write(fd, oldScore, sizeof(int));
        }
        else // TOTO
        {
            lseek(fd, 2 * sizeof(int), SEEK_SET);
            write(fd, oldScore, sizeof(int));
        }
        close(fd);
        // Sleeping for 2 seconds and signaling the referee before pausing
        sleep(2);
        kill(getppid(), SIGUSR1);
    }
}

void checkWinner(int fd, char *name)
{
    int currentScore[1];
    // reading the new totals from sharedFile.bin
    read(fd, currentScore, sizeof(int));
    // printing player's name and total points so far
    if (strcmp(name, "TATA") == 0)
        printf("Referee: TATA's current score: ");
    else if (strcmp(name, "TITI") == 0)
        printf("Referee: TITI's current score: ");
    else
        printf("Referee: TOTO's current score: ");
    printf("%d\n", currentScore[0]);
    sleep(2);
    // checking if there's a winner and terminating all processes in case there is
    if (currentScore[0] >= 50)
    {
        printf("Referee: %s won the game\n", name);
        kill(0, SIGTERM);
    }
}

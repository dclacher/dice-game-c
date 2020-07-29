#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

/* Write a C program, call it diceGame, to simulate a dice game between three players (processes) supervised by a referee (process). 
The three players are named TATA, TITI and TOTO. 
Unlike example 11 from chapter 6, these 4 processes should use a binary file, to store total scores. 
In particular, the main function, run by the referee, should have the following steps */

void action(){}; // to avoid quitting when receiving a SIGUSR1
void player(char *name, int playerId, int fd);
void child(char *);

int main(int argc, char *argv[]) // Referee
{
    int fd; // file descriptor for the binary file sharedFile.bin
    pid_t pid1, pid2, pid3;
    printf("DiceGame: a 3-players game with a referee\n");

    /* Read the winning score from the user. (BUT WHAT TO DO WITH THE SCORE HERE?)
    BESIDES, THIS SHOULD BE INSIDE THE LOOP, NOT IN THIS PART THAT IS EXECUTED ONLY ONCE!
    if (((fd = open("sharedFile.bin", O_RDONLY)) == -1))
    {
        perror("file problem ");
        exit(1);
    }*/

    // Create a read/write binary file, call it sharedFile.bin, before any fork, and write three zero-integers using write. 
    // The file sharedFilke.bin is like a 3-integer array, where 1st integer is the total of TATA, 2nd of TITI and 3rd of TOTO.

    // Creating the binary file
    if ((fd = open("sharedFile.bin", O_CREAT | O_WRONLY | O_TRUNC, 0777)) == -1)
    {
        perror("File problem");
        exit(1);
    } else
    {
        int *threeZeros;
        threeZeros = malloc(3 * sizeof(int));
        threeZeros[0] = 0;
        threeZeros[1] = 0;
        threeZeros[2] = 0;
        write(fd, threeZeros, 3 * sizeof(int));
        close(fd);
        // Test of binary file (read and print content)
        fd = open("sharedFile.bin", O_RDONLY);
        int *buffer;
        buffer = malloc(3 * sizeof(int));
        read(fd, buffer, 3 * sizeof(int));
        printf("buffer[0]: %d\n", buffer[0]);
        printf("buffer[1]: %d\n", buffer[1]);
        printf("buffer[2]: %d\n", buffer[2]);
        //close(fd);
        free(threeZeros);
        free(buffer);
    }

    // Create the three players (processes) and each player should run the same function called 
    // player(char *name, int playerId, int fd) (fd is the file descriptor)

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
        // Make the players play in order: TATA, TITI then TOTO.

        // When a player has finished, the referee reads the new totals from sharedFile.bin, prints the player’s name, 
        // its total points so far and sleeps for 2 seconds. If total exceeds winning score, 
        // the referee prints the name of winning player with its total score and kills all players and itself using kill().
        // WHERE EXACTLY TO PUT THIS PART? AFTER EACH PAUSE BELOW? BETTER CREATE A FUNCTION FOR THIS PIECE OF CODE TO AVOID REPETITION.

        printf("\nReferee: TATA plays\n\n");
        fd = open("sharedFile.bin", O_RDONLY);
        int buf;
        read(fd, &buf, sizeof(int));
        printf("Referee: TATA current score: %d\n", buf);
        kill(pid1, SIGUSR1);
        pause();
        printf("\n\nReferee: TITI plays\n\n");
        kill(pid2, SIGUSR1);
        pause();
        printf("\n\nReferee: TOTO plays\n\n");
        kill(pid3, SIGUSR1);
        pause();
    }
}

void player(char *name, int playerId, int fd)
{
    close(fd);
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
            printf("player: TATA old score: %d\n", oldScore[0]);
        } else if (playerId == 2) // TITI
        {
            lseek(fd, sizeof(int), SEEK_SET);
            read(fd, oldScore, sizeof(int));
            printf("player: TITI old score: %d\n", oldScore[0]);
        } else // TOTO
        {
            lseek(fd, 2*sizeof(int), SEEK_SET); // NOT SURE ABOUT THE 2*
            read(fd, oldScore, sizeof(int));
            printf("player: TOTO old score: %d\n", oldScore[0]);
        }
        close(fd);
        // Playing the dice and printing its own name and the dice score
        printf("%s: playing my dice\n", name);
        dice = (int)time(&ss) % 10 + 1;
        printf("%s: got %d points\n", name, dice);
        // Updating the old score
        int old = oldScore[0];
        oldScore[0] = old + dice;
        // Writing the new score at the same file offset
        fd = open("sharedFile.bin", O_WRONLY);
        if (playerId == 1) // TATA
        {
            lseek(fd, 0, SEEK_SET);
            write(fd, oldScore, sizeof(int));
        } else if (playerId == 2) // TITI
        {
            lseek(fd, sizeof(int), SEEK_SET);
            write(fd, oldScore, sizeof(int));
        } else // TOTO
        {
            lseek(fd, 2*sizeof(int), SEEK_SET); // NOT SURE ABOUT THE 2*
            write(fd, oldScore, sizeof(int));
        }
        close(fd);
        // Sleeping for 2 seconds and signaling the referee before pausing
        sleep(2);
        kill(getppid(), SIGUSR1);
    }
}

void child(char *s)
{
    int points = 0, dice;
    long int ss = 0;
    while (1)
    {
        signal(SIGUSR1, action); // block myself
        pause();
        printf("%s: playing my dice\n", s);
        dice = (int)time(&ss) % 10 + 1;
        printf("%s: got %d points\n", s, dice);
        points += dice;
        printf("%s: Total so far %d\n\n", s, points);
        sleep(3);
        if (points >= 50)
        {
            printf("%s: game over I won\n", s);
            kill(0, SIGTERM);
        }
        kill(getppid(), SIGUSR1);
    }
}
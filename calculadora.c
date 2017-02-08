#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

int main (int argc, char* argv[]){

    int origen, destino, pipe1[2], pipe2[2], pid;    
    int escritos, leidos, status, comentario, i;
    int numlineas = 1;	int numoperaciones = 1;

    char *argvexec[] = {"/usr/bin/bc", NULL};
    char resultado[10], operacion[25], buffer;

    if (argc != 3){
        printf ("Numero de argumentos incorrecto.\n");
        printf ("Usage: ./calculadora filein.txt fileout.txt \n");
        exit (-1);
    }

    origen = open(argv[1], O_RDONLY);
    destino = open(argv[2], O_WRONLY|O_CREAT);

    pipe(pipe1);
    pipe(pipe2);
    pid = fork();
    if (pid == 0){ /* HIJO */
        close(pipe1[1]);
        close(pipe2[0]);

        close(0);
        dup(pipe1[0]);
        close(pipe1[0]);

        close(1);
        dup(pipe2[1]);
        close(pipe2[1]);

        execve(argvexec[0], argvexec, NULL);
        exit(13);

    }else{ /* PADRE */
        close(pipe1[0]);
        close(pipe2[1]);        
        i = 0;
        comentario = 1;        
        while ((leidos = read(origen, &buffer, 1)) > 0){
            if (leidos < 0){
                perror("Error al leer del fichero origen: ");
                exit(-1);

            }else if((buffer == '/') && (comentario == 1)){
                while(buffer != '\n' && leidos > 0){
                    leidos = read(origen, &buffer, 1);
                    if (leidos < 0){
                        perror("Error al leer del fichero origen: ");
                        exit(-1);  
                    }
                }
                numlineas++;

            }else{
                if(buffer == '\n'){
                    if (i != 0){
                        operacion[i] = buffer;
                        escritos = write(destino, " = ", 3);           
                        escritos = write (pipe1[1], &operacion, i+1);
                        leidos = read (pipe2[0], &resultado, 10);
                        escritos = write (destino, &resultado, leidos);
                        numoperaciones++;

                    }else{
                        escritos = write(destino, &buffer, 1);
                    }              
                    numlineas++;
                    comentario = 1;
                    i = 0;

                }else{
                    operacion[i] = buffer;
                    escritos = write(destino, &buffer, 1);
                    i++;
                    comentario = 0;
                }
            }
        }
        write(pipe1[1], "quit\n", 5); /*cierro bc*/
        close(pipe1[1]); close(pipe2[0]);        
        waitpid(pid, &status, 0);
        
        close(origen);
        close(destino);        
        printf("El fichero %s tiene %d lineas, de las cuales, %d son operaciones. El valor de retorno del hijo es %d.\n", argv[1], numlineas, numoperaciones, WEXITSTATUS(status));
        exit(0);
    }
}

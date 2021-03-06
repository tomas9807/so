#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/time.h>
char **get_parametros(int argc, char **argv)
{
    int const start_point_parametros = 1;
    if (argc == 2)
        return NULL; //no parameters supplied

    size_t length_parametros = (argc - start_point_parametros) + 1; //+1 cause  will need a space for NULL

    char **parametros = (char **)malloc(length_parametros * sizeof(char *));
    perror("allocating memory");
    for (size_t i = 0; i < (length_parametros - 1 /*-1 for null at the end */); i++)
        parametros[i] = argv[i + start_point_parametros];

    return parametros;
}

// void print_parametros(char **params){
//     for(int i=0;i<3;i++)
//     printf("param[%i] : %s",i,params[i]);
//     fflush(stdout);
// }

int main(int argc, char *argv[])
{
    struct timeval tv;
    struct timeval start_tv;

    double elapsed = 0.0;

    if (argc > 1)
    {

        char const *prog = argv[1];
        char **parametros = get_parametros(argc, argv);
        int status;
        clock_t proccess_time;

        pid_t pdi = fork(); //crear un proceso hijo para que sea depues reemplazado por el programa a correr
        if (pdi < 0)
        {
            puts("error fork");
        }
        else if (pdi == 0)
        { //es hijo

            execv(prog, parametros);
        }
        else
        { //es padre

            gettimeofday(&start_tv, NULL);

            if (wait(&status) == -1)
                perror("wait error");
            else if (!WIFEXITED(status))
                perror("proceso hijo  tuvo un error y no se completo exitosamente :");

            else
            {
                gettimeofday(&tv, NULL);
                elapsed = (tv.tv_sec - start_tv.tv_sec) +
                          (tv.tv_usec - start_tv.tv_usec) / 1000000.0;

                printf("tiempo de ejecucion del proceso: %fs \n\n", elapsed);
            }
            free(parametros);
        }
    }
}
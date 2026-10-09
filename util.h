#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

#define VAZIO 0   // buffer vazio 

//Medir tempo (Windows e Linux é bem diferente) 
#ifdef _WIN32
  #include <windows.h>
  static void dormir_ms(int ms) { if (ms > 0) Sleep(ms); }
  static long long agora_ms(void) { return (long long)GetTickCount64(); }
#else
  #include <unistd.h>
  static void dormir_ms(int ms) { if (ms > 0) usleep(ms * 1000); }
  static long long agora_ms(void) {
      struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
      return (long long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
  }
#endif

//Parametros da linha de comando 
static int arg(int argc, char **argv, int i, int padrao) {
    return (i < argc) ? atoi(argv[i]) : padrao;
}

// Atrasos aleatorios 
static void semear(int id, int tipo) {      // chamada uma vez no inicio de cada thread
    srand((unsigned)time(NULL) + id * 100 + tipo);
}
static int atraso(int minimo, int maximo) { // sorteia um valor entre minimo e maximo (ms)
    if (maximo <= minimo) return minimo;
    return minimo + rand() % (maximo - minimo + 1);
}

//Logs
static pthread_mutex_t log_mtx = PTHREAD_MUTEX_INITIALIZER; 
static long long t0;                                        

static void iniciar_log(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   // mostra acentos 
#endif
    setvbuf(stdout, NULL, _IONBF, 0);
    t0 = agora_ms();
}

static void log_msg(const char *formato, ...) {   // usado como printf
    
    va_list args;
    pthread_mutex_lock(&log_mtx);
    printf("[%6lld ms] ", agora_ms() - t0);
    va_start(args, formato);
    vprintf(formato, args);
    va_end(args);
    printf("\n");
    pthread_mutex_unlock(&log_mtx);
}

//Formatação do buffer, estado do buffer a cada operacao P ou C
static void mostrar_buffer(const volatile int *buffer, int tam, int ocupados) {
    
    pthread_mutex_lock(&log_mtx);
    printf("[%6lld ms]    BUFFER: [", agora_ms() - t0);
    for (int i = 0; i < tam; i++) {
        if (i > 0) printf(" |");
        if (buffer[i] == VAZIO) printf("  ---");
        else                    printf("%5d", buffer[i]);
    }
    printf(" ]  ocupados=%d\n", ocupados);
    pthread_mutex_unlock(&log_mtx);
}
#endif
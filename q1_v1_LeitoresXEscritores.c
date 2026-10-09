// Processos Leitores × Escritores
// Versão 1: Leitores e escritores sem preferência de ordem de acesso

#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SALDO_INICIAL 1000
#define RODADAS 5

volatile int saldo = SALDO_INICIAL, extrato = 0;
sem_t sem_escritores;
pthread_mutex_t io = PTHREAD_MUTEX_INITIALIZER;
int atraso_l, atraso_e, tempo_escrita, sujas = 0, *valores;
struct timespec t0;

void log_(const char *tipo, int id, const char *fmt, ...) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    long ms = (t.tv_sec - t0.tv_sec) * 1000 + (t.tv_nsec - t0.tv_nsec) / 1000000;
    va_list a; va_start(a, fmt);
    pthread_mutex_lock(&io);
    printf("[%5ldms] %-8s %d | ", ms, tipo, id);
    vprintf(fmt, a); printf("\n");
    pthread_mutex_unlock(&io);
    va_end(a);
}

void *leitor(void *arg) {
    int id = (long)arg;
    log_("LEITOR", id, "criado");
    for (int r = 0; r < RODADAS; r++) {
        usleep(atraso_l * 1000);
        log_("LEITOR", id, "entrou na região crítica (sem bloqueio) - consultando saldo");
        int s = saldo, e = extrato;
        if (s == SALDO_INICIAL + e)
            log_("LEITOR", id, "saldo=%d extrato=%d (consistente)", s, e);
        else {
            __sync_fetch_and_add(&sujas, 1);
            log_("LEITOR", id, "*** LEITURA SUJA *** saldo=%d extrato=%d (saldo esperado=%d)",
                 s, e, SALDO_INICIAL + e);
        }
        log_("LEITOR", id, "saiu da região crítica");
    }
    log_("LEITOR", id, "finalizado");
    return NULL;
}

void *escritor(void *arg) {
    int id = (long)arg, v = valores[id];
    const char *op = v >= 0 ? "depósito" : "saque";
    log_("ESCRITOR", id, "criado (%s de %d por rodada)", op, abs(v));
    for (int r = 0; r < RODADAS; r++) {
        usleep(atraso_e * 1000);
        if (sem_trywait(&sem_escritores) != 0) {
            log_("ESCRITOR", id, "bloqueado: outro escritor ativo");
            sem_wait(&sem_escritores);
        }
        log_("ESCRITOR", id, "entrou na região crítica");
        if (saldo + v < 0)
            log_("ESCRITOR", id, "saque de %d recusado: saldo insuficiente (saldo=%d)", abs(v), saldo);
        else {
            extrato += v;
            log_("ESCRITOR", id, "%s de %d: extrato=%d registrado, saldo=%d ainda desatualizado",
                 op, abs(v), extrato, saldo);
            usleep(tempo_escrita * 1000);
            saldo += v;
            log_("ESCRITOR", id, "saldo atualizado: saldo=%d extrato=%d", saldo, extrato);
        }
        log_("ESCRITOR", id, "saiu da região crítica");
        sem_post(&sem_escritores);
    }
    log_("ESCRITOR", id, "finalizado");
    return NULL;
}

int main(void) {
    int nl, ne;
    printf("Leitores, escritores: "); scanf("%d %d", &nl, &ne);
    printf("Atraso leitor (ms), atraso escritor (ms), tempo de escrita (ms): ");
    scanf("%d %d %d", &atraso_l, &atraso_e, &tempo_escrita);
    valores = malloc(ne * sizeof(int));
    printf("Valor de cada escritor (%d valores; positivo=depósito, negativo=saque): ", ne);
    for (int i = 0; i < ne; i++) scanf("%d", &valores[i]);

    sem_init(&sem_escritores, 0, 1);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    pthread_t *th = malloc((nl + ne) * sizeof(pthread_t));
    for (long i = 0; i < nl; i++) pthread_create(&th[i], NULL, leitor, (void *)i);
    for (long i = 0; i < ne; i++) pthread_create(&th[nl + i], NULL, escritor, (void *)i);
    for (int i = 0; i < nl + ne; i++) pthread_join(th[i], NULL);

    printf("\nFinal: saldo=%d extrato=%d | leituras sujas: %d\n", saldo, extrato, sujas);
    return 0;
}
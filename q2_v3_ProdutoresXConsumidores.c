// Produtores × Consumidores
// Versão sem controle de concorrência (Modo vida loka)

#include "util.h"

int n_prod, n_cons, tam, itens, pmin, pmax, cmin, cmax, janela;
volatile int *buffer;
volatile int in = 0, out = 0, ocupados = 0;
volatile int sobrescritas = 0, leituras_vazias = 0;

static void *produtor(void *p) {
    int id = (int)(size_t)p;
    semear(id, 1);

    for (int k = 1; k <= itens; k++) {
        int valor = id * 1000 + k;
        log_msg("Produtor %d produzindo item %d", id, valor);
        dormir_ms(atraso(pmin, pmax));

        int pos = in;                  
        dormir_ms(janela);                
        if (buffer[pos] != VAZIO) {
            sobrescritas++;
            log_msg("Produtor %d: posicao %d ja tem o item %d, sera sobrescrito (perdido)",id, pos, buffer[pos]);
        }

        buffer[pos] = valor;
        in = (pos + 1) % tam;
        int tmp = ocupados;             
        dormir_ms(janela);
        ocupados = tmp + 1;

        log_msg("Produtor %d colocou item %d na posicao %d", id, valor, pos);
        mostrar_buffer(buffer, tam, ocupados);
    }
    return NULL;
}

static void *consumidor(void *p) {
    int id = (int)(size_t)p;
    semear(id, 2);
    int total = n_prod * itens;
    int quota = total / n_cons + (id <= total % n_cons ? 1 : 0);   // itens que este consumidor tenta ler

    for (int n = 0; n < quota; n++) {
        
        int pos = out;                  
        dormir_ms(janela);                 
        int valor = buffer[pos];
        if (valor == VAZIO) {
            leituras_vazias++;
            log_msg("Consumidor %d: posicao %d esta vazia, nao esperou um item (errado)", id, pos);
        } else {
            buffer[pos] = VAZIO;
        }
        out = (pos + 1) % tam;
        int tmp = ocupados;          
        dormir_ms(janela);
        ocupados = tmp - 1;

        if (valor != VAZIO) log_msg("Consumidor %d retirou item %d da posicao %d", id, valor, pos);
        mostrar_buffer(buffer, tam, ocupados);
        if (valor != VAZIO) log_msg("Consumidor %d consumindo item %d", id, valor);
        dormir_ms(atraso(cmin, cmax));
    }
    return NULL;
}

int main(int argc, char **argv) {
    n_prod = arg(argc, argv, 1, 3);   n_cons = arg(argc, argv, 2, 2);
    tam    = arg(argc, argv, 3, 5);   itens  = arg(argc, argv, 4, 6);
    pmin   = arg(argc, argv, 5, 100); pmax   = arg(argc, argv, 6, 600);
    cmin   = arg(argc, argv, 7, 300); cmax   = arg(argc, argv, 8, 900);
    janela = arg(argc, argv, 9, 20);

    if (n_prod < 1 || n_prod > 999 || n_cons < 1 || tam < 1 || itens < 1 || itens > 999 || pmin > pmax || cmin > cmax) {
        printf("Parametros invalidos\n");
        return 1;
    }

    iniciar_log();
    buffer = calloc(tam, sizeof(int));

    printf("Versão 3: %d produtores x %d consumidores | buffer = %d | %d itens por produtor\n\n", n_prod, n_cons, tam, itens);

    pthread_t *tp = malloc(sizeof(pthread_t) * n_prod);
    pthread_t *tc = malloc(sizeof(pthread_t) * n_cons);
    for (int i = 0; i < n_cons; i++) pthread_create(&tc[i], NULL, consumidor, (void *)(size_t)(i + 1));
    for (int i = 0; i < n_prod; i++) pthread_create(&tp[i], NULL, produtor, (void *)(size_t)(i + 1));
    for (int i = 0; i < n_prod; i++) pthread_join(tp[i], NULL);
    for (int i = 0; i < n_cons; i++) pthread_join(tc[i], NULL);
    
    int restantes = 0;
    for (int i = 0; i < tam; i++) // cc itens que sobraram no buffer
        if (buffer[i] != VAZIO)
            restantes++;

    printf("Resultados (deveria ser 0 erros e contador = 0)\n");
    printf("Itens sobrescritos (perdidos): %d\n", sobrescritas);
    printf("Leituras de posicao vazia: %d\n", leituras_vazias);
    printf("Itens restantes no buffer: %d\n", restantes);
    printf("Contador ocupado final: %d (deveria ser 0)\n", ocupados);

    free((void *)buffer);
    free(tp); 
    free(tc);
    return 0;
}

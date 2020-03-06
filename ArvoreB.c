#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Configurações do programa
#define TSIZE 50
#define INDICE "indice.bin"

//Definindo tipo de variaveis auxiliares
typedef struct no_arvoreB arvoreB, *p_arvoreB;
typedef int Dado;
typedef enum{ERROR=0, OK=1} status;
typedef unsigned long int offset;

//Definindo estrutura do nó da arvore
struct no_arvoreB{
    short int n;
    char folha;
    Dado chaves[(2*TSIZE-1)];
    offset link[(2*TSIZE-1)];
    char deletado[(2*TSIZE-1)];
    offset filhos[2*TSIZE];
};

/*#################### FUNÇÕES ####################*/

void contador_no_arvore(){
    int j;
    FILE *fp;
    fp = fopen(INDICE, "rb+");
    fseek(fp, 0, SEEK_SET);
    fread(&j, sizeof(int), 1, fp);
    j++;
    fseek(fp, 0, SEEK_SET);
    fwrite(&j, sizeof(int), 1, fp);
    fclose(fp);
}

status aloca_no_arvoreB(p_arvoreB *arvore){
    arvoreB *no = (arvoreB*) malloc(sizeof(arvoreB));
    if(no == NULL) return ERROR;
    int i;
    no->n = 0;
    no->folha = 'S';
    for(i=0; i<(2*TSIZE-1); i++) no->chaves[i] = 0;
    for(i=0; i<(2*TSIZE-1); i++) no->link[i] = 0;
    for(i=0; i<(2*TSIZE-1); i++) no->deletado[i] = 'N';
    for(i=0; i<(2*TSIZE); i++) no->filhos[i] = 0;
    *arvore = no;
    return OK;
}

status criar_arvore_B(){
    p_arvoreB raiz;
    int cont = 0;
    FILE *fp;
    if(aloca_no_arvoreB(&raiz) == ERROR) return ERROR;
    fp = fopen(INDICE, "wb");
    if(fp == NULL) return ERROR;
    fwrite(&cont, sizeof(int), 1, fp);
    fwrite(raiz, sizeof(arvoreB), 1, fp);
    fclose(fp);
    contador_no_arvore();
    free(raiz);
    return OK;
}

status disk_read(p_arvoreB *no, offset link){
    FILE *fp;
    fp = fopen(INDICE, "rb");
    if(fp == NULL) return ERROR;
    p_arvoreB lido = (arvoreB*) malloc(sizeof(arvoreB));
    if(lido == NULL) return ERROR;
    fseek(fp, link, SEEK_SET);
    fread(lido, sizeof(arvoreB), 1, fp);
    fclose(fp);
    *no = lido;
    return OK;
}

status disk_write(p_arvoreB no, offset link, offset *retorno){
    FILE *fp;
    fp = fopen(INDICE, "rb+");
    if(fp == NULL) return ERROR;
    if(link != 0){
        fseek(fp, link, SEEK_SET);
    }else{
        fseek(fp, 0, SEEK_END);
        *retorno = ftell(fp);
    }
    fwrite(no, sizeof(arvoreB), 1, fp);
    fclose(fp);
    if(link == 0) contador_no_arvore();
    return OK;
}

status divide_arvoreB_filho(p_arvoreB x, offset e_x, int i){
    p_arvoreB y, z;
    offset e_z, e_y = x->filhos[i];
    int j;
    if(aloca_no_arvoreB(&z) == ERROR) return ERROR;
    if(disk_read(&y, e_y) == ERROR) return ERROR;
    z->folha = y->folha;
    z->n = TSIZE-1;
    for(j=0; j<(TSIZE-1); j++){
        z->chaves[j] = y->chaves[j+TSIZE];
        z->link[j] = y->link[j+TSIZE];
        z->deletado[j] = y->deletado[j+TSIZE];
    }
    if(y->folha == 'N'){
        for(j=0; j<TSIZE; j++) z->filhos[j] = y->filhos[j+TSIZE];
    }
    y->n = TSIZE-1;
    for(j=x->n; j>i; j--) x->filhos[j+1] = x->filhos[j];
    if(disk_write(z, 0, &e_z) == ERROR) return ERROR;
    x->filhos[i+1] = e_z;
    for(j=x->n-1; j>=i; j--){
        x->chaves[j+1] = x->chaves[j];
        x->link[j+1] = x->link[j];
        x->deletado[j+1] = x->deletado[j];
    }
    x->chaves[i] = y->chaves[TSIZE-1];
    x->link[i] = y->link[TSIZE-1];
    x->deletado[i] = y->deletado[TSIZE-1];
    x->n++;
    if(disk_write(y, e_y, &e_y) == ERROR) return ERROR;
    if(disk_write(x, e_x, &e_x) == ERROR) return ERROR;
    free(z);
    free(y);
    return OK;
}

status insere_arvoreB_incompleto(p_arvoreB x, offset e_x, Dado chave, offset e_c){
    int i;
    p_arvoreB y;
    offset e_y;
    i = x->n - 1;
    if(x->folha == 'S'){
        while(i >= 0 && chave < x->chaves[i]){
            x->chaves[i+1] = x->chaves[i];
            x->link[i+1] = x->link[i];
            x->deletado[i+1] = x->deletado[i];
            i--;
        }
        x->chaves[i+1] = chave;
        x->link[i+1] = e_c;
        x->n++;
        if(disk_write(x, e_x, &e_x) == ERROR) return ERROR;
        free(x);
    }else{
        while(i >= 0 && chave < x->chaves[i]) i--;
        i++;
        if(disk_read(&y, x->filhos[i]) == ERROR) return ERROR;
        if(y->n == (2*TSIZE-1)){
            if(divide_arvoreB_filho(x, e_x, i) == ERROR) return ERROR;
            if(chave > x->chaves[i]) i++;
            free(y);
            if(disk_read(&y, x->filhos[i]) == ERROR) return ERROR;
        }
        e_y = x->filhos[i];
        free(x);
        if(insere_arvoreB_incompleto(y, e_y, chave, e_c) == ERROR) return ERROR;
    }
}

status insere_arvoreB(Dado chave, offset e_c){
    p_arvoreB r, s;
    offset e_s, e_r = sizeof(int);
    if(disk_read(&r, e_r) == ERROR) return ERROR;
    if(r->n == (2*TSIZE-1)){
        e_s = e_r;
        if(aloca_no_arvoreB(&s) == ERROR) return ERROR;
        s->folha = 'N';
        s->n = 0;
        if(r->filhos[1] != 0){
            r->folha = 'N';
        }else{
            r->folha = 'S';
        }
        if(disk_write(r, 0, &e_r) == ERROR) return ERROR;
        free(r);
        s->filhos[0] = e_r;
        if(divide_arvoreB_filho(s, e_s, 0) == ERROR) return ERROR;
        free(s);
        if(disk_read(&s, e_s) == ERROR) return ERROR;
        if(insere_arvoreB_incompleto(s, e_s, chave, e_c) == ERROR) return ERROR;
    }else{
        if(insere_arvoreB_incompleto(r, e_r, chave, e_c) == ERROR) return ERROR;
    }
    return OK;
}

offset busca_arvoreB(offset e_x, int chave){
    int i=0;
    p_arvoreB x;
    printf("> %d\n", e_x);
    if(disk_read(&x, e_x) == ERROR) return;
    while(i < x->n && chave > x->chaves[i]) i++;
    if(i < x->n && chave == x->chaves[i]){
        printf("\nChave %d foi encontrada no endereco %d (link: %d)\n", chave, e_x, x->link[i]);
        return x->link[i];
    }else if(x->folha == 'S'){
        printf("\nChave nao existe.\n");
        return -1;
    }else{
        return busca_arvoreB(x->filhos[i], chave);
    }
}

void deleta_arvoreB(offset e_x, int chave){
    int i=0;
    p_arvoreB x;
    printf("> %d\n", e_x);
    if(disk_read(&x, e_x) == ERROR) return;
    while(i < x->n && chave > x->chaves[i]) i++;
    if(i < x->n && chave == x->chaves[i]){
        x->deletado[i] = 'S';
        if(disk_write(x, e_x, &e_x) == ERROR) return;
        printf("\nChave %d foi deletada.\n", x->chaves[i]);
    }else if(x->folha == 'S'){
        printf("\nChave nao existe.\n");
    }else{
        return deleta_arvoreB(x->filhos[i], chave);
    }
}

void imprime_tudo_arvoreB(offset e_x){
    p_arvoreB x;
    int i;
    if(disk_read(&x, e_x) == ERROR) return;
    if(x->folha == 'S'){
        for(i=0; i<x->n; i++){
            printf("=> %d (l: %d - ex: %c)\n", x->chaves[i], x->link[i], x->deletado[i]);
        }
    }else{
        for(i=0; i<x->n; i++){
            imprime_tudo_arvoreB(x->filhos[i]);
            printf("=> %d (l: %d - ex: %c)\n", x->chaves[i], x->link[i], x->deletado[i]);
        }
        imprime_tudo_arvoreB(x->filhos[i]);
    }
    free(x);
}

void imprime_nos_arvoreB(){
    FILE *fp;
    int i, j, n;
    arvoreB no;
    fp = fopen(INDICE, "rb");
    fread(&n, sizeof(int), 1, fp);
    for(i=0; i<n; i++){
        fread(&no, sizeof(arvoreB), 1, fp);
        printf("\n===== Offset %d (%d chaves, folha %c) =====\n", (ftell(fp)-sizeof(arvoreB)), no.n, no.folha);
        for(j=0; j<no.n; j++){
            printf("%d < %d > %d\n", no.filhos[j], no.chaves[j], no.filhos[j+1]);
        }
    }
    fclose(fp);
}

void menu(){
    int op;
    printf("\n===== MENU =====\n1-Inserir\n2-Imprimir nos\n3-Criar Arvore\n4-Inserir ate\n5-Buscar\n6-Imprimir Tudo\n7-Deletar\n8-Sair\n");
    scanf("%d", &op);
    if(op == 1){
        int chave;
        printf("\nDigite a chave que deseja inserir:\n");
        scanf("%d", &chave);
        if(insere_arvoreB(chave, chave) == OK){
            printf("\n%d inserido com sucesso.\n", chave);
        }else{
            printf("\nErro ao inserir %d\n", chave);
        }
    }
    if(op == 2){
        imprime_nos_arvoreB();
    }
    if(op == 3){
        if(criar_arvore_B() == OK){
            printf("\nArvore criada com sucesso.\n");
        }
    }
    if(op == 4){
        int i, c, f;
        printf("\nDigite o comeco:\n");
        scanf("%d", &c);
        printf("\nDigite ate a chave que deseja inserir:\n");
        scanf("%d", &f);
        for(i=c; i<=f; i++){
            if(insere_arvoreB(i, i) == OK){
                printf("%d\n", i);
            }else{
                printf("\nErro ao inserir %d\n", i);
            }
        }
    }
    if(op == 5){
        int n;
        printf("\nDigite ate a chave que deseja buscar:\n");
        scanf("%d", &n);
        busca_arvoreB(sizeof(int), n);
    }
    if(op == 6){
        printf("\n============= Imprimindo =============\n");
        imprime_tudo_arvoreB(sizeof(int));
        printf("\n============= Fim =============\n");
    }
    if(op == 7){
        int n;
        printf("\nDigite ate a chave que deseja deletar:\n");
        scanf("%d", &n);
        deleta_arvoreB(sizeof(int), n);
    }
    if(op != 8) menu();
}

int main(){
    printf("\nTamanho da Struct: %d\n", sizeof(arvoreB));
    menu();
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>

typedef struct Dados
{
    uint8_t* dados;
    int sequenciaTam;
} Dados;

typedef struct DadosArquivo
{
    int qtdDados;
    Dados* dados;
} DadosArquivo;

// Estrutura para o nó da árvore de Huffman
typedef struct NoHuffman
{
    uint8_t byte; // Valor do byte
    unsigned int frequencia; // Frequência do byte
    struct NoHuffman* esquerda; // Nó esquerdo
    struct NoHuffman* direita; // Nó direito
} NoHuffman;

// Estrutura para armazenar o resultado da compressão
typedef struct ResultadoComp {
    int bitsTotal;
    float percentual;
    uint8_t* buffer;
    int bufferTam;
    char algo[4]; // "RLE" ou "HUF"
} ResultadoComp;

// --- Estrutura de Heap (Min-Heap Array) ---
typedef struct Heap {
    NoHuffman** array;
    int tamanho;
    int capacidade;
} Heap;

// Função auxiliar para converter um caractere hexadecimal para seu valor (0-15)
int hexCharParaInt(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1; // Erro ou caractere não hexadecimal
}

// Função para ler os dados do arquivo de entrada
DadosArquivo lerArquivo(FILE* arquivo)
{
    DadosArquivo dadosArquivo;
    // Lendo quantidade de sequências
    if (fscanf(arquivo, "%d", &dadosArquivo.qtdDados) != 1)
    {
        dadosArquivo.qtdDados = 0;
        dadosArquivo.dados = NULL;
        return dadosArquivo;
    }
    
    // Alocando memória para as sequências
    dadosArquivo.dados = malloc(dadosArquivo.qtdDados * sizeof(Dados));
    
    // Lendo cada sequência
    for (int i = 0; i < dadosArquivo.qtdDados; i++)
    {
        // 1. Lendo tamanho da sequência
        fscanf(arquivo, "%d", &dadosArquivo.dados[i].sequenciaTam);
        int tamanho = dadosArquivo.dados[i].sequenciaTam;

        // 2. Alocando memória para os dados da sequência
        dadosArquivo.dados[i].dados = malloc(tamanho * sizeof(uint8_t));

        // 3. Lendo e convertendo os bytes de forma eficiente (Substituindo o fscanf lento)
        for (int j = 0; j < tamanho; j++) {
            int val_alto = -1;
            int val_baixo = -1;
            char c;

            // Busca pelo dígito hexadecimal alto
            do {
                c = fgetc(arquivo);
                val_alto = hexCharParaInt(c);
            } while (val_alto == -1 && !feof(arquivo));

            // Busca pelo dígito hexadecimal baixo
            do {
                c = fgetc(arquivo);
                val_baixo = hexCharParaInt(c);
            } while (val_baixo == -1 && !feof(arquivo));
            
            // Verifica se as leituras foram bem-sucedidas
            if (val_alto != -1 && val_baixo != -1) {
                // Combina os dois nibbles para formar o byte (e.g., A e 3 = 0xA3)
                dadosArquivo.dados[i].dados[j] = (uint8_t)((val_alto << 4) | val_baixo);
            } else {
                // Tratar erro de formato ou fim de arquivo inesperado
                // Por simplicidade, podemos sair ou logar um erro
                break; 
            }
        }
    }

    return dadosArquivo;
}

// ---------------------- RLE --------------------------

// Função para comprimir dados usando RLE (Run-Length Encoding)
ResultadoComp compressaoRLE(Dados* dados)
{
    ResultadoComp res;
    strcpy(res.algo, "RLE");
    res.buffer = NULL;
    res.bufferTam = 0;

    uint8_t* sequencia = dados->dados;
    int tamanho = dados->sequenciaTam;

    if (tamanho == 0) {
        res.bitsTotal = 0;
        res.percentual = 0.0f;
        return res;
    }

    /* --- ALOCAÇÃO INICIAL E ESCRITA EM PASSAGEM ÚNICA --- */
    // Alocação inicial otimista: 1/4 do tamanho original (típico para repetição)
    int capacidade = (tamanho / 4) + 1; 
    res.buffer = malloc(capacidade * sizeof(uint8_t));
    int pos = 0;
    int i = 0;
    
    // Calcula bitsAntes apenas uma vez
    const int bitsAntes = tamanho * 8; 
    
    while (i < tamanho) {
        // Variáveis de escopo local para clareza, eliminando run_start
        const uint8_t byte_atual = sequencia[i];
        int run_length = 1;
        
        // 1. Contar sequência de bytes repetidos
        while (i + run_length < tamanho && byte_atual == sequencia[i + run_length]) {
            run_length++;
        }
        
        int remaining = run_length;
        
        // 2. Codificar em chunks de 255
        while (remaining > 0) {
            // Garante que o buffer tem espaço para 2 bytes (Contagem + Dado)
            if (pos + 2 > capacidade) {
                capacidade *= 2; // Duplica a capacidade
                uint8_t* temp = realloc(res.buffer, capacidade * sizeof(uint8_t));
                if (temp == NULL) {
                    // Tratar erro de alocação: liberar o que já foi alocado e retornar
                    free(res.buffer);
                    res.buffer = NULL;
                    return res; 
                }
                res.buffer = temp;
            }

            // Contagem máxima é 255 para uint8_t
            int chunk = remaining > 255 ? 255 : remaining; 
            
            // Escreve a Contagem
            res.buffer[pos++] = (uint8_t)chunk;
            // Escreve o Byte
            res.buffer[pos++] = byte_atual;
            
            remaining -= chunk;
        }
        
        i += run_length; // Avança o ponteiro de leitura
    }

    /* --- FINALIZAÇÃO E CÁLCULO DE RESULTADOS --- */
    
    // Ajusta o buffer para o tamanho exato final (pos)
    res.bufferTam = pos;
    uint8_t* temp = realloc(res.buffer, pos * sizeof(uint8_t));
    if (temp) {
        res.buffer = temp;
    }
    
    const int bitsDepois = res.bufferTam * 8;
    res.bitsTotal = bitsDepois;
    
    // Usa 'bitsAntes' calculado e constante
    res.percentual = (bitsAntes == 0) ? 0.0f : 100.0f * (float)bitsDepois / (float)bitsAntes;

    return res;
}

/* ---------------------- Huffman -------------------------- */

Heap* criarHeap(int capacidade)
{
    Heap* h = malloc(sizeof(Heap));
    h->tamanho = 0;
    h->capacidade = capacidade;
    h->array = malloc(capacidade * sizeof(NoHuffman*));
    return h;
}

NoHuffman* criarNo(uint8_t byte, int freq)
{
    NoHuffman *n = malloc(sizeof(NoHuffman));
    n->byte = byte;
    n->frequencia = freq;
    n->esquerda = NULL;
    n->direita = NULL;
    return n;
}

void trocarNo(NoHuffman** a, NoHuffman** b)
{
    NoHuffman* t = *a;
    *a = *b;
    *b = t;
}

// Min-Heapify padrão
void minHeapify(Heap* h, int idx)
{
    int menor = idx;
    int esq = 2 * idx + 1;
    int dir = 2 * idx + 2;

    if (esq < h->tamanho && h->array[esq]->frequencia < h->array[menor]->frequencia)
        menor = esq;

    if (dir < h->tamanho && h->array[dir]->frequencia < h->array[menor]->frequencia)
        menor = dir;

    if (menor != idx) {
        trocarNo(&h->array[menor], &h->array[idx]);
        minHeapify(h, menor);
    }
}

// Insere nó no heap
void inserirHeap(Heap *h, NoHuffman *novo)
{
    int i = h->tamanho++;
    h->array[i] = novo;

    // Heapify-up (sift-up)
    while (i > 0) {
        int pai = (i - 1) / 2;

        if (h->array[pai]->frequencia <= h->array[i]->frequencia)
            break;

        trocarNo(&h->array[pai], &h->array[i]);
        i = pai;
    }
}

// Constrói o heap mínimo a partir do array atual
void buildHeap(Heap *h)
{
    for (int i = (h->tamanho / 2) - 1; i >= 0; i--) {
        minHeapify(h, i);
    }
}

// Extrai o nó com a menor frequência
NoHuffman* extrairMin(Heap *h)
{
    if (h->tamanho == 0) return NULL;
    NoHuffman* raiz = h->array[0];
    h->array[0] = h->array[h->tamanho - 1];
    h->tamanho--;
    minHeapify(h, 0);
    return raiz;
}

// Gera tabela de códigos recursivamente
void gerarCodigos(NoHuffman* raiz, int arr[], int top, char codigos[256][256], int tamanhos[256])
{
    if (raiz->esquerda) {
        arr[top] = 0;
        gerarCodigos(raiz->esquerda, arr, top + 1, codigos, tamanhos);
    }
    if (raiz->direita) {
        arr[top] = 1;
        gerarCodigos(raiz->direita, arr, top + 1, codigos, tamanhos);
    }
    // É nó folha
    if (!raiz->esquerda && !raiz->direita) {
        for (int i = 0; i < top; i++) {
            codigos[raiz->byte][i] = arr[i] + '0';
        }
        codigos[raiz->byte][top] = '\0';
        tamanhos[raiz->byte] = top;
    }
}

void liberarArvore(NoHuffman* raiz)
{
    if (!raiz) return;
    liberarArvore(raiz->esquerda);
    liberarArvore(raiz->direita);
    free(raiz);
}

ResultadoComp compressaoHuffman(Dados *dados)
{
    ResultadoComp res;
    strcpy(res.algo, "HUF");
    res.buffer = NULL;
    res.bufferTam = 0;

    int tam = dados->sequenciaTam;
    int bitsAntes = tam * 8;
    
    // Frequência
    int freq[256] = {0};
    for (int j = 0; j < tam; j++)
        freq[dados->dados[j]]++;

    // Contar quantos símbolos únicos existem
    int simbolosUnicos = 0;
    for (int b = 0; b < 256; b++) {
        if (freq[b] > 0) simbolosUnicos++;
    }
    
    // Criar heap com capacidade exata
    Heap* h = criarHeap(simbolosUnicos);
    
    // ADICIONAR símbolos diretamente no array (ainda não é heap)
    for (int b = 0; b < 256; b++) {
        if (freq[b] > 0) {
            h->array[h->tamanho] = criarNo((uint8_t)b, freq[b]);
            h->tamanho++;
        }
    }
    
    // AGORA construir heap em tempo linear O(n)
    buildHeap(h);

    // Construção da árvore
    while (h->tamanho > 1) {
        NoHuffman *esquerda = extrairMin(h);
        NoHuffman *direita = extrairMin(h);

        NoHuffman *pai = criarNo(0, esquerda->frequencia + direita->frequencia);
        pai->esquerda = esquerda;
        pai->direita = direita;

        inserirHeap(h, pai);
    }
    
    NoHuffman* raiz = extrairMin(h);

    // Gerar Códigos e Empacotar Bits
    char mapaCodigos[256][256];
    int mapaTamanhos[256] = {0};
    int arrAux[256];
    
    if (raiz && !raiz->esquerda && !raiz->direita) {
        mapaCodigos[raiz->byte][0] = '0';
        mapaCodigos[raiz->byte][1] = '\0';
        mapaTamanhos[raiz->byte] = 1;
    } else if (raiz) {
        gerarCodigos(raiz, arrAux, 0, mapaCodigos, mapaTamanhos);
    }

    // Calcular bits totais e gerar buffer
    int bitsTotais = 0;
    for(int i=0; i<tam; i++) bitsTotais += mapaTamanhos[dados->dados[i]];
    
    res.bufferTam = (bitsTotais + 7) / 8;
    if (res.bufferTam == 0 && bitsTotais > 0) res.bufferTam = 1;
    res.buffer = calloc(res.bufferTam, sizeof(uint8_t));

    // Escrever bits no buffer
    int bitPos = 0;
    for (int i = 0; i < tam; i++) {
        uint8_t byte = dados->dados[i];
        char* codigo = mapaCodigos[byte];
        for (int k = 0; codigo[k] != '\0'; k++) {
            if (codigo[k] == '1') {
                res.buffer[bitPos / 8] |= (1 << (7 - (bitPos % 8)));
            }
            bitPos++;
        }
    }

    int bitsDepois = res.bufferTam * 8;

    res.bitsTotal = bitsDepois;
    res.percentual = 100.0f * (float)bitsDepois / (float)bitsAntes;
    
    liberarArvore(raiz);  // Primeiro libera árvore
    free(h->array);       // Depois array do heap
    free(h);              // Finalmente estrutura heap
    
    return res;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
        return 1;

    FILE* input = fopen(argv[1], "r");
    FILE* output = fopen(argv[2], "w");
    if (!input || !output) {
        perror("Erro ao abrir input ou output");
        return 1;
    }

    DadosArquivo dadosArquivo = lerArquivo(input);
    int qtdDados = dadosArquivo.qtdDados;

    char** saidas = malloc(qtdDados * sizeof(char*));
    
    #ifdef _OPENMP
    #pragma omp parallel for
    #endif
    for (int i = 0; i < qtdDados; i++)
    {
        ResultadoComp rle = compressaoRLE(&dadosArquivo.dados[i]);
        ResultadoComp huf = compressaoHuffman(&dadosArquivo.dados[i]);

        // Calcular tamanho necessário para o buffer de saída
        int tamanhoNecessario = (rle.bufferTam + huf.bufferTam) * 2 + 256;
        
        char* bufferSaida = malloc(tamanhoNecessario);

        int offset = 0;

        // Caso de empate: imprime os dois
        if (huf.bitsTotal == rle.bitsTotal)
        {
            offset += snprintf(bufferSaida + offset, tamanhoNecessario - offset, "%d->HUF(%.2f%%)=", i, huf.percentual);
            for (int j = 0; j < huf.bufferTam; j++)
                offset += snprintf(bufferSaida + offset, tamanhoNecessario - offset, "%02X", huf.buffer[j]);

            // Adiciona quebra de linha e o segundo resultado
            offset += snprintf(bufferSaida + offset, tamanhoNecessario - offset, "\n%d->RLE(%.2f%%)=", i, rle.percentual);
            for (int j = 0; j < rle.bufferTam; j++)
                offset += snprintf(bufferSaida + offset, tamanhoNecessario - offset, "%02X", rle.buffer[j]);
        } else
        {
            ResultadoComp* v = (huf.bitsTotal < rle.bitsTotal) ? &huf : &rle;

            if (strcmp(v->algo, "HUF") == 0)
            {
                offset += snprintf(bufferSaida + offset, tamanhoNecessario - offset,
                                "%d->HUF(%.2f%%)=", i, v->percentual);
            }
            else
            {
                offset += snprintf(bufferSaida + offset, tamanhoNecessario - offset,
                                "%d->RLE(%.2f%%)=", i, v->percentual);
            }

            for (int j = 0; j < v->bufferTam; j++)
                offset += snprintf(bufferSaida + offset, tamanhoNecessario - offset, "%02X", v->buffer[j]);
        }

        saidas[i] = bufferSaida;

        free(rle.buffer);
        free(huf.buffer);
    }

    // Escreve todas as saídas no arquivo de uma vez
    for (int i = 0; i < qtdDados; i++)
    {
        fprintf(output, "%s\n", saidas[i]);
        free(saidas[i]);
    }
    
    // Liberar memória alocada para os dados
    for (int i = 0; i < qtdDados; i++)
        free(dadosArquivo.dados[i].dados);
    free(dadosArquivo.dados);
    free(saidas);
    // Fechar arquivos
    fclose(input);
    fclose(output);

    return 0;
}
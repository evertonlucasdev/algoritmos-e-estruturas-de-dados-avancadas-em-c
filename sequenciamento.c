#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Estruturas de dados
typedef struct Gene {
    char nomeGene[1001];
} Gene;

typedef struct Doenca {
    char nomeDoenca[10];
    Gene* genes;
    int qtdGenes;
} Doenca;

typedef struct Arquivo {
    char dna[50000];
    Doenca* doencas; // Array dinâmico de doenças
    int qtdDoencas;
    int tamanhoSubGenes;
} Arquivo;

typedef struct Resultado {
    char nomeDoenca[10];
    int probabilidade;
} Resultado;

// Estrutura para tabela hash simples
typedef struct HashEntry {
    unsigned long long hash;
    int* posicoes;  // Array dinâmico de posições
    int contador;   // Quantidade de posições
    int capacidade; // Capacidade atual do array
} HashEntry;

typedef struct HashTable {
    HashEntry* entradas;
    int tamanho;    // Tamanho da tabela
    int contador;   // Entradas ocupadas
} HashTable;

// Constantes para hash rolling
#define BASE 257            // Base para hash (primo maior que alfabeto)
#define MOD 1000000007      // Módulo primo grande
#define TABLE_SIZE 100003   // Tamanho da tabela hash (primo)

// Função para criar a tabela hash
HashTable* criarHashTable(int tamanho)
{
    // Aloca memória para a tabela hash
    HashTable* hashtable = malloc(sizeof(HashTable));
    // Inicializa os campos
    hashtable->tamanho = tamanho;
    hashtable->contador = 0;
    hashtable->entradas = calloc(tamanho, sizeof(HashEntry));

    return hashtable;
}

// Procedimento para destruir a tabela hash
void destruirHashTable(HashTable* ht)
{
    // Verifica se a tabela é nula
    if (!ht) return;
    // Libera cada entrada
    for (int i = 0; i < ht->tamanho; i++)
        if (ht->entradas[i].posicoes)
            free(ht->entradas[i].posicoes);

    // Libera o array de entradas e a tabela
    free(ht->entradas);
    free(ht);
}

// Função hash simples
int hashFunc(unsigned long long key, int tamanho)
{
    return (int)(key % tamanho);
}

// Procedimento para adicionar posição a uma entrada
void adicionarPosicao(HashEntry* entry, int posicao)
{
    // Verifica se precisa redimensionar
    if (entry->contador >= entry->capacidade)
    {
        // Redimensiona o array
        int novaCapacidade = entry->capacidade == 0 ? 4 : entry->capacidade * 2;
        // Aloca novo array
        int* novoArray = realloc(entry->posicoes, novaCapacidade * sizeof(int));
        if (!novoArray) return;
        // Atualiza a entrada
        entry->posicoes = novoArray;
        // Atualiza capacidade
        entry->capacidade = novaCapacidade;
    }

    // Adiciona a posição
    entry->posicoes[entry->contador++] = posicao;
}

// Procedimento para inserir hash na tabela
void inserirHash(HashTable* ht, unsigned long long hash, int posicao)
{
    // Calcula índice inicial
    int idx = hashFunc(hash, ht->tamanho);

    // Procura entrada existente
    while (ht->entradas[idx].contador > 0 && ht->entradas[idx].hash != hash)
        idx = (idx + 1) % ht->tamanho; // Tratamento de colisão por sondagem linear

    // Verifica se é uma nova entrada, caso seja, inicializa
    if (ht->entradas[idx].contador == 0)
    {
        ht->entradas[idx].hash = hash; // Define o hash
        ht->entradas[idx].posicoes = NULL; // Inicializa o array de posições
        ht->entradas[idx].contador = 0; // Inicializa contador
        ht->entradas[idx].capacidade = 0; // Inicializa capacidade
        ht->contador++; // Incrementa contador de entradas ocupadas
    }

    // Adiciona a posição à entrada
    adicionarPosicao(&ht->entradas[idx], posicao);
}

// Função para buscar hash na tabela
int* buscarHash(HashTable* ht, unsigned long long hash, int* numPosicoes)
{
    // Calcula índice inicial
    int idx = hashFunc(hash, ht->tamanho);
    // Procura entrada
    int startIdx = idx;

    // Percorre até encontrar ou esgotar
    while (ht->entradas[idx].contador > 0)
    {
        // Verifica se o hash corresponde, se sim, retorna as posições
        if (ht->entradas[idx].hash == hash)
        {
            // Define o número de posições
            *numPosicoes = ht->entradas[idx].contador;
            // Retorna o array de posições
            return ht->entradas[idx].posicoes;
        }
        // Avança para o próximo índice
        idx = (idx + 1) % ht->tamanho;
        // Verifica se voltou ao início
        if (idx == startIdx) break;
    }

    // Não encontrado
    *numPosicoes = 0;

    return NULL;
}

// Função para calcular hash de uma string
unsigned long long calcularHash(const char* str, int len)
{
    // Calcula hash simples
    unsigned long long hash = 0;

    // Percorre a string caractere por caractere e calcula o hash
    for (int i = 0; i < len; i++)
        hash = (hash * BASE + (unsigned char)str[i]) % MOD;

    return hash;
}

// Função para calcular BASE^(len) % MOD
unsigned long long calcularPotencia(int len)
{
    // Calcula potência usando multiplicação simples
    unsigned long long resultado = 1;

    // Percorre para calcular a potência
    for (int i = 0; i < len; i++)
        resultado = (resultado * BASE) % MOD;

    return resultado;
}

// Função para preprocessar o DNA e criar a tabela hash
HashTable* preprocessarDNA(const char* dna, int L)
{
    // Verifica tamanho do DNA
    int dnaLen = strlen(dna);
    // Se o DNA for menor que L, retorna NULL
    if (dnaLen < L) return NULL;
    // Cria a tabela hash
    HashTable* hashtable = criarHashTable(TABLE_SIZE);
    // Calcula hash da primeira substring
    unsigned long long hash = calcularHash(dna, L);
    // Insere na tabela hash
    inserirHash(hashtable, hash, 0);
    // Calcula potência BASE^(L-1) % MOD
    unsigned long long potencia = calcularPotencia(L - 1);

    // Calcula hashes rolling para as demais substrings
    for (int i = L; i < dnaLen; i++)
    {
        // Remove caractere mais antigo, adiciona novo
        unsigned long long oldChar = (unsigned char)dna[i - L];
        unsigned long long newChar = (unsigned char)dna[i];
        
        // hash = (hash - oldChar * BASE^(L-1)) * BASE + newChar
        // Cuidado com módulo negativo
        hash = (hash + MOD - (oldChar * potencia) % MOD) % MOD;
        hash = (hash * BASE + newChar) % MOD;
        // Insere na tabela hash
        inserirHash(hashtable, hash, i - L + 1);
    }

    return hashtable;
}

// Função para verificar match exato
int verificarMatchExato(const char* dna, const char* gene, int dnaPos, int genePos, int L)
{
    // Compara caractere por caractere
    for (int i = 0; i < L; i++)
        // Verifica se há diferença entre os caracteres
        if (dna[dnaPos + i] != gene[genePos + i])
            return 0;

    return 1;
}

// Função para estender o match além de L
int estenderMatch(const char* dna, int dnaLen, const char* gene, int geneLen, int dnaPos, int genePos, int L)
{
    // Inicializa o comprimento do match
    int matchLen = L;

    // Estende para a direita
    while (genePos + matchLen < geneLen && 
           dnaPos + matchLen < dnaLen &&
           gene[genePos + matchLen] == dna[dnaPos + matchLen])
        matchLen++;
    
    return matchLen;
}

// Função principal com Rabin-Karp
int calcularCompatibilidade(const char* dna, const char* gene, int L, HashTable* hashtable)
{
    // Armazena o tamaanho do DNA e do gene
    int dnaLen = strlen(dna);
    int geneLen = strlen(gene);
    if (L > geneLen || L > dnaLen || L <= 0) return 0;

    // Total de caracteres que deram match
    int totalMatch = 0;
    // Posição atual no gene
    int genePos = 0;

    // Percorre o gene enquanto houver espaço para substrings de tamanho L
    while (genePos <= geneLen - L)
    {
        // Calcula o hash da substring do gene
        unsigned long long geneHash = calcularHash(&gene[genePos], L);
        // Numero de posições encontradas no DNA
        int numPosicoes;
        // Array de posições no DNA onde o hash foi encontrado
        int* posicoes = buscarHash(hashtable, geneHash, &numPosicoes);
        // Flag para indicar se encontrou match
        int melhorMatchDestePonto = 0;

        // Procurando o MAIOR match entre todas as ocorrências
        if (posicoes && numPosicoes > 0)
        {
            // Verifica cada posição para match exato
            for (int i = 0; i < numPosicoes; i++)
            {
                // Posição no DNA
                int dnaPos = posicoes[i];

                // Verifica match exato
                if (verificarMatchExato(dna, gene, dnaPos, genePos, L))
                {
                    // Estende o match além de L
                    int matchAtual = estenderMatch(dna, dnaLen, gene, geneLen, dnaPos, genePos, L);

                    // Atualiza o melhor match deste ponto
                    if (matchAtual > melhorMatchDestePonto)
                        melhorMatchDestePonto = matchAtual;
                }
            }
        }

        // Se encontrou algum match, avança a posição do gene
        if (melhorMatchDestePonto > 0)
        {
            // Atualiza total de caracteres que deram match
            totalMatch += melhorMatchDestePonto;
            // Avança a posição do gene
            genePos += melhorMatchDestePonto;
        } else // Se não encontrou match
            // Avança apenas uma posição no gene
            genePos++;
    }

    if (geneLen == 0) return 0; // Evita divisão por zero
    
    // Calcula o percentual de compatibilidade
    return (totalMatch * 100 + (geneLen / 2)) / geneLen;
}

// Função para ler dados do arquivo
Arquivo lerArquivo(FILE* arquivo)
{
    // Declaração da estrutura de dados
    Arquivo dadosArquivo;
    // Quantidade de doenças
    int qtdDoencas = 0;
    // Lê os 3 primeiros dados do arquivo
    fscanf(arquivo, "%d", &dadosArquivo.tamanhoSubGenes);
    fscanf(arquivo, "%s", dadosArquivo.dna);
    fscanf(arquivo, "%d", &qtdDoencas);
    // Atribui a quantidade de doenças
    dadosArquivo.qtdDoencas = qtdDoencas;
    // Aloca memória para as doenças
    dadosArquivo.doencas = malloc(qtdDoencas * sizeof(Doenca));

    // Lê os dados de cada doença
    for (int i = 0; i < qtdDoencas; ++i)
    {
        // Lê nome da doença e quantidade de genes
        fscanf(arquivo, "%s %d", dadosArquivo.doencas[i].nomeDoenca, &dadosArquivo.doencas[i].qtdGenes);
        // Armazena quantidade de genes
        int qtdGenes = dadosArquivo.doencas[i].qtdGenes;
        // Aloca memória para os genes
        dadosArquivo.doencas[i].genes = malloc(qtdGenes * sizeof(Gene));
        // Lê os genes
        for (int j = 0; j < qtdGenes; ++j)
            fscanf(arquivo, "%s", dadosArquivo.doencas[i].genes[j].nomeGene);
    }
    
    return dadosArquivo;
}

// Função para diagnosticar doença
int diagnosticarDoenca(const char* DNA, Gene* genes, int numGenes, int L, HashTable* ht)
{
    // Verifica parâmetros inválidos
    if (!DNA || !genes || numGenes <= 0 || L <= 0) return 0;
    // Contador de genes detectados
    int genesDetectados = 0;

    // Percorre todos os genes
    for (int i = 0; i < numGenes; i++)
    {
        // Calcula a compatibilidade do gene atual
        int compatibilidade = calcularCompatibilidade(DNA, genes[i].nomeGene, L, ht);
        // Verifica se a compatibilidade é maior ou igual a 90%
        if (compatibilidade >= 90)
            genesDetectados++;
    }

    // Calcula a probabilidade como porcentagem de genes detectados
    double resultado = ((double)genesDetectados / (double)numGenes) * 100.0;

    return (int)(resultado + 0.5);
}

// Procedimento para ordenar resultados por probabilidade (insertion sort)
void ordenarResultados(Resultado* resultados, int qtdResultados)
{
    // Percorre os resultados e ordena
    for (int i = 1; i < qtdResultados; i++)
    {
        // Armazena o resultado atual
        Resultado key = resultados[i];
        // Posição anterior
        int j = i - 1;

        // Move os resultados maiores para a direita
        while (j >= 0 && resultados[j].probabilidade < key.probabilidade)
        {
            // Move o resultado para a direita
            resultados[j + 1] = resultados[j];
            j--;
        }

        // Insere o resultado na posição correta
        resultados[j + 1] = key;
    }
}

// Procedimento para escrever resultados no arquivo
void escreverResultados(FILE* output, Resultado* resultados, int qtdResultados)
{
    // Escreve cada resultado no arquivo
    for (int i = 0; i < qtdResultados; i++)
    {
        fprintf(output, "%s->%d%%", resultados[i].nomeDoenca, resultados[i].probabilidade);
        fprintf(output, "\n");
    }
}

// Procedimento para liberar memória alocada
void liberarMemoria(Arquivo* dados)
{
    // Percorre todas as doenças e libera os genes
    for (int i = 0; i < dados->qtdDoencas; i++)
        free(dados->doencas[i].genes);
    // Libera o array de doenças
    free(dados->doencas);
}

int main(int argc, char *argv[])
{
    // Verifica a validade dos argumentos de linha de comando
    if (argc != 3)
    {
        printf("Uso: %s <input> <output>\n", argv[0]);
        return 1;
    }
    
    // Abre arquivos de input e output
    FILE* input = fopen(argv[1], "r");
    if (!input)
    {
        perror("Erro ao abrir input");
        return 1;
    }
    
    FILE* output = fopen(argv[2], "w");
    if (!output)
    {
        perror("Erro ao abrir output");
        fclose(input);
        return 1;
    }

    // Lê dados do arquivo de input
    Arquivo dadosArquivo = lerArquivo(input);
    // Array para armazenar resultados
    Resultado* resultados = malloc(dadosArquivo.qtdDoencas * sizeof(Resultado));
    // Preprocessa o DNA e cria a tabela hash
    HashTable* hashtable = preprocessarDNA(dadosArquivo.dna, dadosArquivo.tamanhoSubGenes);

    // Percorre todas as doenças para diagnóstico
    for (int i = 0; i < dadosArquivo.qtdDoencas; i++)
    {
        // Doença atual
        Doenca doencaAtual = dadosArquivo.doencas[i];
        // Faz o diagnóstico da doença
        int resultado = diagnosticarDoenca(
            dadosArquivo.dna,
            doencaAtual.genes,
            doencaAtual.qtdGenes,
            dadosArquivo.tamanhoSubGenes,
            hashtable
        );

        // Copia o nome da doença para o resultado
        strcpy(resultados[i].nomeDoenca, doencaAtual.nomeDoenca);
        // Armazena a probabilidade
        resultados[i].probabilidade = resultado;
    }

    // Destrói a tabela hash
    destruirHashTable(hashtable);
    // Ordena e escreve os resultados
    ordenarResultados(resultados, dadosArquivo.qtdDoencas);
    escreverResultados(output, resultados, dadosArquivo.qtdDoencas);
    
    // Libera memória
    free(resultados);
    liberarMemoria(&dadosArquivo);

    // Fecha os arquivos
    fclose(input);
    fclose(output);
    
    return 0;
}
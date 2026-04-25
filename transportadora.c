#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Estruturas
typedef struct {
    char codigo[14];
    float valor;
    uint32_t peso, volume;
    bool usado;
} Item;

typedef struct {
    Item *itens;
    uint32_t qtd;
    float valorMaximo;
} ItemArray;

typedef struct {
    char placa[16];
    uint32_t peso, volume;
} Veiculo;

typedef struct {
    Veiculo *veiculos;
    uint32_t qtd;
} VeiculosArray;

// Fórmula: Camada * (AreaDaCamada) + Linha * (LarguraDaLinha) + Coluna
#define IDX(i, p, v, strideP, strideV) ((size_t)(i) * (strideP) * (strideV) + (size_t)(p) * (strideV) + (size_t)(v))

// Procedimento para preencher Tabela 3D linearizada
void preencherTabelaLinear(float *matriz, Item *itens, uint32_t qtdItens, uint32_t capPeso, uint32_t capVolume)
{
    // Cálculo de strides para acesso linear
    uint32_t strideP = capPeso + 1;
    uint32_t strideV = capVolume + 1;

    // Itera sobre os itens
    for (uint32_t i = 1; i <= qtdItens; ++i)
    {
        // Atributos do item atual
        const uint32_t idxItem = i - 1;
        const uint32_t pesoItem = itens[idxItem].peso;
        const uint32_t volumeItem = itens[idxItem].volume;
        const float valorItem = itens[idxItem].valor;

        // Itera sobre capacidades de Peso
        for (uint32_t p = 0; p <= capPeso; ++p)
        {
            // Verifica se é possível incluir o item atual pelo peso
            int pesoRestante = (int)p - (int)pesoItem;
            bool podeIncluirPeso = (pesoRestante >= 0);

            // Itera sobre capacidades de Volume
            for (uint32_t v = 0; v <= capVolume; ++v)
            {
                // Calcula índices lineares para acesso direto à RAM
                size_t idxAtual = IDX(i, p, v, strideP, strideV);
                size_t idxPrev  = IDX(i - 1, p, v, strideP, strideV);
                // Valor sem incluir o item atual
                float melhor = matriz[idxPrev];

                // Verifica se é possível incluir o item atual pelo volume
                if (podeIncluirPeso && (int)v >= (int)volumeItem)
                {
                    // Acessa o estado anterior subtraindo o peso/volume do item atual
                    size_t idxPrevComItem = IDX(i - 1, pesoRestante, v - volumeItem, strideP, strideV);
                    float comItem = matriz[idxPrevComItem] + valorItem;
                    
                    // Atualiza o melhor valor se incluir o item for vantajoso
                    if (comItem > melhor) melhor = comItem;
                }

                // Armazena o melhor valor encontrado na posição atual
                matriz[idxAtual] = melhor;
            }
        }
    }
}

// Backtracking Otimizado com Alocação Única
float backtracking(ItemArray itens, uint32_t qtdItens, uint32_t capPeso, uint32_t capVolume)
{
    // Aloca matriz 3D linearizada em um único bloco
    size_t totalElementos = (size_t)(qtdItens + 1) * (capPeso + 1) * (capVolume + 1);
    // calloc inicializa com 0, necessário para a lógica da DP
    float *matrizLinear = (float *) calloc(totalElementos, sizeof(float));
    if (!matrizLinear) {
        fprintf(stderr, "Erro fatal: Memoria insuficiente para alocar matriz de DP.\n");
        exit(1);
    }

    // Preenche a tabela usando aritmética de ponteiros
    preencherTabelaLinear(matrizLinear, itens.itens, qtdItens, capPeso, capVolume);
    // Cálculo de strides para acesso linear
    uint32_t strideP = capPeso + 1;
    uint32_t strideV = capVolume + 1;
    // O valor máximo está na última posição da matriz lógica
    float valorMaximo = matrizLinear[IDX(qtdItens, capPeso, capVolume, strideP, strideV)];

    // Se houver itens, faz o caminho reverso para descobrir quais foram escolhidos
    if (qtdItens > 0)
    {
        // Variáveis auxiliares para rastreamento
        int auxPeso = (int)capPeso;
        int auxVolume = (int)capVolume;
        uint32_t temp = qtdItens;

        // Caminha de trás para frente na matriz
        while (temp > 0)
        {
            // Se capacidades negativas, algo errado ou fim do caminho
            if (auxPeso < 0 || auxVolume < 0) break;

            // Valores atual e anterior na matriz
            float valAtual = matrizLinear[IDX(temp, auxPeso, auxVolume, strideP, strideV)];
            float valPrev  = matrizLinear[IDX(temp - 1, auxPeso, auxVolume, strideP, strideV)];

            // Se o valor mudou em relação à iteração anterior (temp-1), significa que incluímos o item
            if (valAtual != valPrev)
            {
                // Marca o item como usado e ajusta capacidades restantes
                itens.itens[temp-1].usado = true;
                auxPeso -= (int)itens.itens[temp-1].peso;
                auxVolume -= (int)itens.itens[temp-1].volume;
                temp--;
            } else // Item não foi incluído, sobe para a linha anterior mantendo peso/volume
            {
                temp--;
            }
        }
    }

    // Libera o bloco único de memória
    free(matrizLinear);

    // Retorna o valor máximo calculado
    return valorMaximo;
}

// Procedimento para calcular somatórios de peso e volume dos itens marcados como usados
void somatorio(Item *itens, uint32_t qtdItens, uint32_t *somatorioPeso, uint32_t *somatorioVolume)
{
    // Inicializa somatórios
    *somatorioPeso = 0;
    *somatorioVolume = 0;

    // Itera sobre os itens para calcular somatórios
    for (uint32_t i = 0; i < qtdItens; ++i)
    {
        // Verifica se o item foi marcado como usado
        if (itens[i].usado)
        {
            *somatorioPeso += itens[i].peso;
            *somatorioVolume += itens[i].volume;
        }
    }
}

// Função auxiliar para porcentagem
uint32_t calcularPorcentagens(uint32_t valorParcial, uint32_t valorTotal)
{
    if (valorTotal == 0) return 0;
    return (uint32_t) round(((float)valorParcial / (float)valorTotal) * 100.0f);
}

// Função para escrever a linha de saída formatada
void escreverOutput(FILE* output, char placa[], float valorMaximo, uint32_t somaPeso, uint32_t porcentagemPeso, uint32_t somaVolume, uint32_t porcentagemVolume, ItemArray itens)
{
    fprintf(output, "[%s]R$%.2f,%dKG(%d%%),%dL(%d%%)->", placa, valorMaximo, somaPeso, porcentagemPeso, somaVolume, porcentagemVolume);

    bool primeiro = true;
    for (uint32_t i = 0; i < itens.qtd; ++i)
    {
        if (itens.itens[i].usado)
        {
            if (!primeiro) fprintf(output, ",");
            fprintf(output, "%s", itens.itens[i].codigo);
            primeiro = false;
        }
    }
    fprintf(output, "\n");
}

// Função para remover itens usados do array e retorna nova quantidade
uint32_t removerItensUsados(Item *itens, uint32_t qtd)
{
    // Índice de escrita para itens não usados
    uint32_t write = 0;

    // Itera sobre os itens
    for (uint32_t i = 0; i < qtd; ++i)
    {
        // Se o item não foi usado, mantém no array
        if (!itens[i].usado)
        {
            // Move o item para a posição de escrita se necessário
            if (write != i) itens[write] = itens[i];
            // Incrementa o índice de escrita
            write++;
        }
    }

    // Retorna a nova quantidade de itens não usados
    return write;
}

// Procedimento principal de processamento dos dados
void processarDados(VeiculosArray veiculos, ItemArray itens, FILE* output)
{
    // Itera sobre cada veículo
    for (uint32_t i = 0; i < veiculos.qtd; i++)
    {
        // Reseta flags para a nova iteração para o veículo atual
        for (uint32_t j = 0; j < itens.qtd; ++j)
            itens.itens[j].usado = false;

        // Capacidades do veículo atual
        uint32_t capP = veiculos.veiculos[i].peso;
        uint32_t capV = veiculos.veiculos[i].volume;
        // Calcula valor máximo possível com backtracking
        float valorMaximo = backtracking(itens, itens.qtd, capP, capV);
        // Calcula somatórios de peso e volume dos itens escolhidos
        uint32_t somaPeso = 0, somaVolume = 0;
        somatorio(itens.itens, itens.qtd, &somaPeso, &somaVolume);
        // Calcula porcentagens de peso e volume utilizados
        uint32_t porcentagemPeso = calcularPorcentagens(somaPeso, capP);
        uint32_t porcentagemVolume = calcularPorcentagens(somaVolume, capV);
        // Cria array temporário para escrita
        ItemArray tmpArr = { itens.itens, itens.qtd, 0.0f };
        // Escreve a linha de saída para o veículo atual
        escreverOutput(output, veiculos.veiculos[i].placa, valorMaximo, somaPeso, porcentagemPeso, somaVolume, porcentagemVolume, tmpArr);
        // Remove itens carregados para não serem considerados no próximo veículo
        uint32_t novaQtd = removerItensUsados(itens.itens, itens.qtd);
        // Atualiza a quantidade de itens restantes
        itens.qtd = novaQtd;
    }

    // Imprime itens pendentes, se houver
    if (itens.qtd > 0)
    {
        float somaValor = 0.0f;
        uint32_t somaPeso = 0, somaVolume = 0;
        for (uint32_t i = 0; i < itens.qtd; ++i)
        {
            somaValor += itens.itens[i].valor;
            somaPeso += itens.itens[i].peso;
            somaVolume += itens.itens[i].volume;
        }

        fprintf(output, "PENDENTE:R$%.2f,%dKG,%dL->", somaValor, somaPeso, somaVolume);
        for (uint32_t i = 0; i < itens.qtd; ++i)
        {
            fprintf(output, "%s", itens.itens[i].codigo);
            if (i + 1 < itens.qtd) fprintf(output, ",");
        }
        fprintf(output, "\n");
    }
}

// Função para leitura de arquivo - Veículos
VeiculosArray lerDadosVeiculo(FILE* arquivo)
{
    uint32_t n;
    VeiculosArray veiculos = {NULL, 0};

    if (fscanf(arquivo, "%u", &n) != 1) return veiculos;

    Veiculo *dadosVeiculo = malloc(sizeof(Veiculo) * n);
    if (!dadosVeiculo) return veiculos;

    for (uint32_t i = 0; i < n; ++i)
        fscanf(arquivo, "%15s %u %u", dadosVeiculo[i].placa, &dadosVeiculo[i].peso, &dadosVeiculo[i].volume);

    veiculos.veiculos = dadosVeiculo;
    veiculos.qtd = n;
    return veiculos;
}

// Função para leitura de arquivo - Itens
ItemArray lerDadosItem(FILE* arquivo)
{
    ItemArray itens = {NULL, 0, 0.0f};
    uint32_t n;

    if (fscanf(arquivo, "%u", &n) != 1) return itens;

    Item *dadosItem = malloc(sizeof(Item) * n);
    if (!dadosItem) return itens;

    for (uint32_t i = 0; i < n; ++i)
    {
        fscanf(arquivo, "%13s %f %u %u", dadosItem[i].codigo, &dadosItem[i].valor, &dadosItem[i].peso, &dadosItem[i].volume);
        dadosItem[i].usado = false;
    }

    itens.itens = dadosItem;
    itens.qtd = n;
    return itens;
}

int main(int argc, char *argv[])
{
    // Validação de Argumentos
    if (argc != 3)
        return 1;

    // Abre arquivos de entrada e saída
    FILE* input = fopen(argv[1], "r");
    FILE* output = fopen(argv[2], "w");
    if (!input || !output)
    {
        perror("Erro ao abrir arquivo de entrada");
        return 1;
    }

    // Leitura de Dados de Entrada
    VeiculosArray dadosVeiculos = lerDadosVeiculo(input);
    ItemArray dadosItens = lerDadosItem(input);
    // Processamento dos Dados
    processarDados(dadosVeiculos, dadosItens, output);
    // Fecha arquivos
    fclose(input);
    fclose(output);
    // Libera memória alocada
    free(dadosVeiculos.veiculos);
    free(dadosItens.itens);

    return 0;
}
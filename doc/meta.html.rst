.. rst3: filename: ./doc/meta.html

#################################
MetaG, compilador de compiladores
#################################

A documentação é lacônica; projeto em desenvolvimento.
Ainda há muito para aprimorar. Tudo escrito aqui pode estar errado.

Introdução
++++++++++++



O que é MetaG?
***************

MetaG é um compilador de compiladores baseado em um trabalho anterior chamado Meta-II.
Existem alguns tutoriais na internet demonstrando como Meta-II funcionava.

MetaG recebe uma gramática escrita em uma linguagem especial e produz código em C que implementa um parser/compilador para esta gramática.
Este manual descreve como usar a linguagem de MetaG de forma prática.

MetaG pode ser útil para criar preprocessadores para uma linguagem pré-existente, converter um formato de configuração em código executável, ou implementar uma linguagem. Na esperança que seja útil, este documento foi criado.

Como usar MetaG?
****************

MetaG é distribuído como um pequeno conjunto de arquivos.
Há um cabeçalho C (support.h) com as funções mais fundamentais do compilador, e a gramática (meta.txt) que define o compilador MetaG.
Também é distribuída a implementação de MetaG em C (meta.c).

Para usar MetaG, você precisa realizar o processo de bootstrap. É simples, basta compilar a implementação em C::

    gcc meta.c -o meta

Compilar suas gramáticas para código C é trivial::

    ./meta gram.meta gram.c

Como modificar MetaG?
*********************

MetaG é um metacompilador porque é escrito na mesma linguagem que ele implementa.
Você pode modificar o compilador (meta.txt) e recompilar, criando um novo compilador baseado no anterior::

    ./meta meta.txt meta2.c
    gcc meta2.c -o meta2

Se não acontecerem erros durante a compilação e testes da nova versão, você pode substituir a anterior pela nova.

Pode parecer arbitrário que MetaG compile à si mesmo, mas isso é importante.
Metacompilar prova a expressividade e estabilidade da linguagem implementada pelo compilador.
Metacompilar, portanto, é o teste mais básico para MetaG.

O teste da metacompilação é que o metacompilador possa produzir uma cópia idêntica de si mesmo.
Eu digo que um metacompilador que passa neste teste é "metacompilante".

Com isso em mente, o ciclo de atualização ideal de MetaG funciona da seguinte maneira::

    # Bootstrap da próxima iteração
    ./meta meta.txt meta2.c
    gcc meta2.c -o meta2
    # Teste da metacompilação
    ./meta2 meta.txt meta3.c
    gcc meta3.c -o meta3
    ./meta3 meta.txt meta4.c
    # Se a próxima iteração for "metacompilante",
    #   o comando à seguir deve mostrar que os dois arquivos são idênticos:
    diff meta3.c meta4.c

Acompanha um Makefile que implementa a metacompilação::

    # Faz bootstrap e metacompila:
    make metacompile
    # Admite a nova versão:
    make evolve

Linguagem MetaG
+++++++++++++++

A linguagem de MetaG é livremente inspirada em EBNF, com semântica PEG.
MetaG constrói parsers recursivo-descendentes com predição ajustável e backtracking.

MetaG foi pensado para aplicações práticas que precisem detalhar tokens byte por byte.
Possui instruções para controle do espaço branco e definição de tokens customizados.

Acima de tudo, a linguagem de MetaG processa texto.

Para facilitar a leitura, tenha em mente:

- "Metacompilador" se refere ao MetaG
- "Compilador" se refere ao compilador construído à partir da gramática que o usuário criar.

Convenções léxicas
*********************

A gramática MetaG no alto nível é composta de regras de reconhecimento de padrões.

Para instruir o metacompilador de forma detalhada existem diretivas de diferentes tipos.
As diretivas são os elementos mais básicos da linguagem, com as quais são construídas todas as subexpressões.

MetaG não faz exigência de identação, mas você pode usar livremente para facilitar a leitura.

Primeiros passos
^^^^^^^^^^^^^^^^

Toda construção de alto nível de MetaG termina com um ponto-e-vírgula.
As subexpressões são separadas por espaço branco.

Toda gramática MetaG começa com uma diretiva '.syntax', que dá o nome da gramática::

    .syntax my_grammar;

E termina com uma diretiva de finalização, que define o que o compilador irá fazer primeiro. Por exemplo::

    .end first_rule;

Opcionalmente a gramática pode possuir uma diretiva de inicialização, que adiciona código customizado no início do compilador.
A diretiva ".initialize" precisa ser imediatamente a primeira após ".syntax" e vir antes de todas as outras regras. ::

    .initialize "/* Hello */";

As regras de reconhecimento possuem um nome e um corpo, separados por um sinal de igual.
O corpo é composto de uma ou mais diretivas.

Regras de reconhecimento são como funções que não recebem argumentos.
Elas podem chamar umas às outras e à si mesmas recursivamente, se necessário. ::

    nome_da_regra = <'/*corpo_da_regra*/'>;

Você não pode definir regras com corpo vazio, mas pode definir regras que ignoram a entrada::

    regra_nula = continue;

Aqui uma gramática que reconhece a sequência de caracteres "abc" e exibe na saída o texto "OK" em caso de sucesso::

    .syntax test;
    abc = 'abc' {'OK'};
    .test abc;

A diretiva de finalização ".test" faz o compilador esperar texto na entrada padrão do console (stdin), configura o estado inicial do compilador e chama a função definida para processar o texto recebido (neste caso é a regra "abc").

Por padrão, MetaG ignora todo o texto posterior ao que for reconhecido pela regra chamada. Então se você escrever "abcdef" na entrada do console, o compilador irá emitir "OK" e ignorar "def".

Ordem de regras de reconhecimento
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Devido ao fato do código gerado ter uma correlação muito próxima com a linguagem de MetaG e o código emitido ser em C, você idealmente deve declarar suas regras algum momento antes de usa-las em outras, da mesma forma que deve definir suas funções em um momento anterior ao de usa-las em C.

Caso precise usar uma regra antes do ponto onde ela é definida, use ".initialize" para declarar os protótipos das funções das regras. Isso será explicado mais à frente.

Ou seja::

    % "regra2" foi definida antes do primeiro local onde é usada:
    regra2 = {'OK'};
    regra1 = regra2;

    % O compilador C irá reclamar:
    regra1 = regra2;
    regra2 = {'OK'};

    % O compilador C não irá reclamar:
    .initialize regra2;
    regra1 = regra2;
    regra2 = {'OK'};

Comentários
^^^^^^^^^^^^

Comentários começam com o caractere "%" e continuam até a quebra da linha.
O conteúdo dos comentários é tratado como espaço em branco pelo metacompilador e não causa efeito na saída. ::

    % Um comentário

Comentários podem aparecer apenas entre a diretiva inicial ".syntax" e de finalização, entre as regras de reconhecimento e possivelmente antes de ".initialize".
Comentários não podem aparecer dentro das regras de reconhecimento ou dentro do corpo das outras diretivas.

Se sentir necessidade de comentar partes de uma regra, quebre suas regras em regras menores e use comentários antes/acima delas.

Exemplo::

    % inválido
    .syntax test;
    % válido
    .initialize .var(p[3]); % válido
    % válido
    abc = 'abc' {'OK'}; % válido
    def = 'def' % inválido;
    .test abc;

Literais
^^^^^^^^

Strings literais são delimitadas pelos caracteres de aspas simples ou duplas::

    "uma string"
    'outra string'

Strings podem conter caracteres especiais no estilo C, como::

    "\\ \n \r"

Constantes numéricas
^^^^^^^^^^^^^^^^^^^^^

Algumas diretivas aceitam argumentos numéricos, como::

    123
    \\xff

Identificadores
^^^^^^^^^^^^^^^

Os identificadores, como os nomes das regras de reconhecimento, podem conter os seguintes caracteres:

- Underline "_" ou letras latinas ("A" até "Z", "a" até "z") como primeiro caractere
- Os mesmos caracteres anteriores, mais algarismos ("0" até "9") do segundo caractere em diante


.. rst3: filename: ./doc/meta.html

#################################
MetaG, compilador de compiladores
#################################

A documentação é lacônica; este é um projeto em desenvolvimento.
Ainda há muito para aprimorar.

Introdução
++++++++++++



O que é MetaG?
***************

MetaG é um compilador de compiladores baseado em um trabalho anterior chamado Meta-II.
Existem alguns tutoriais na internet demonstrando como Meta-II funcionava.

MetaG recebe uma gramática escrita em uma linguagem especial e produz código em C que implementa um parser/compilador para esta gramática.
Este manual descreve como usar a linguagem de MetaG de forma prática.

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

Estou fornecendo um Makefile que implementa a metacompilação.

E a linguagem de MetaG?
***********************

A linguagem de MetaG é livremente inspirada em EBNF, e será descrita em detalhes no restante do documento.


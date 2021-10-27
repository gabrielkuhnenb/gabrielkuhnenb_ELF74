    PUBLIC EightBitHistogram
    SECTION .text: CODE
  
    THUMB

;--------------------------------------------------
; [r0,r3] para parametros da funcao
; [r4,r11] para variaveis
; e necessario mapear valores de [0,255] para [0,511]
; para fazer isso, basta multiplicar por 2
;--------------------------------------------------

EightBitHistogram
; r4 tamanho da imagem
; r5 posicao inicial do vetor histograma
; r6 iterador
; r7 valor do pixel
; r8 valor da posicao x do histograma

    mul r4, r0, r1 ; armazena o tamanho da imagem em r4
    mov r5, r3 ; inicio do histograma

ImageSize
; verifica o tamanho da imagem. se for superior a 64k, retorna 0
    cmp r4, #65536
    itt gt
        movgt r0, #0
        bxgt lr    

    mov r6, #0 ; prepara iterador
    mov r7, #0 ; valor inicial das posicoes do histograma
    push {lr}
    bl ClearVector ; preenche histograma com #0 em todas as posicoes
    pop {lr}
    
    mov r6, #0 ; prepara iterador
    push {lr}
    bl FillVector ; preenche histograma
    pop {lr}
    
    mov r0, r4 ; r0 recebe o tamanho da imagem
    bx lr

;--------------------------------------------------
ClearVector
    str r7, [r3], #2 ; escreve #0 em todas as posicoes do vetor
                     ; incrementa o valor de r3 em 2 bytes
                     ; pois se trata de um vetor de uint16_t
    add r6, #1
    cmp r6, #256     ; verifica se preencheu todo o vetor
    bne ClearVector
    bx lr

;--------------------------------------------------
FillVector
    mov r3, r5 ; r3 recebe a posicao inicial do vetor
    ldrb r7, [r2], #1 ; r7 recebe o valor do pixel
    add r7, r7 ; como o vetor e uint16_t, e necessario multiplicar por 2
               ; para escrever na posicao correta do vetor
    
    ldr r8, [r3, r7] ; r8 recebe a posicao do vetor referente ao valor do pixel
    add r8, #1 ; e entao seu valor e incrementado
    
    add r3, r7 ; o ponteiro para o vetor e corrigido
    str r8, [r3] ; e o valor daquela posicao e atualizado
    
    add r6, #1 
    cmp r6, r4 ; verifica se chegou ao final da imagem
    bne FillVector 
    bx lr

    END

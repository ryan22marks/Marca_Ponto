# Marca_Ponto


- EXPLICAÇÕES DA MEMORIA EEPROM:  memoria não volátil do ESP32 (não se apaga com o tempo). Veja abaixo a distribuição das posições de memória
      - endereço 0 guarda quantos usuarios estao cadastrados
      - endereço 1 guarda o numero magico do mestre (flag de estado do mestre)
      - endereços de 2 pra cima guardam os ID's dos usuarios

- EXPLICAÇÕES DO RFID: modulo leitor de cartoes com chip. Veja abaixo a pinagem do RFID para o ESP32
    - SDA  --->  GPIO14
    - SCK  --->  GPIO18
    - MOSI --->  GPIO23
    - MISO --->  GPIO19
    - GND  --->  GND DO ESP32
    - RST  --->  GPIO27
    - 3V3  --->  3V3 DO ESP32


- EXPLICAÇÕES DO TECLADO MATRICIAL: teclado alfanumerico usado para iserção de dados. Composto por 4 linhas e 4 colunas
    -------------------------------------------------------
    LINHAS:
    cor do fio -  pino referente
    - LARANJA  =  GPIO15
    - AMARELO  =  GPIO02
    - VERDE    =  GPIO04
    - AZUL     =  GPIO05
    --------------------------------------------------------
    COLUNAS: SÃO PRETAS NAS PONTAS FINAIS
    cor do fio -  pino referente
    - ROXO     =  GPIO13
    - CINZA    =  GPIO12
    - BRANCO   =  GPIO26
    - PRETO    =  GPIO25
    

      
 - EXPLICAÇÕES DO DISPLA LCD: DISPLAY 16x2 COM COMUNICAÇÃO I2C. ESSE DISPLAY OPERA EM 5Vdc
    - SDA(FIO BRANCO)  =  GPIO21
    - SCL(FIO AZUL)  =  GPIO22


 - EXPLICAÇÕES DO NTC RELOGIO DE TEMPO ONLINE:
    Relogio de tempo que coleta a data e a hora de um servidor online, ha uma função que faz
    com que esse sistema de horas funcione mesmo sem conexao com a rede.

 - EXPLICAÇÕES DA PLATAFORMA GOOGLE SHEETS:
    Essa plataforma recebe os dados de login dos usuarios, quando um usuario registra ponto, 
    o ESP32 envia ao Google Sheetso ID do cartao e assim o ponto fica salvo na nuvem. O envio
    dos dados é feito por requisição ao servidor do Google, ou seja, nos conectamos ao servidor
    e depois enviamos um link de registro de dados. Dentro do Google Sheets há uma Aplicação
    WEB que recebe os dados e insere eles na planilha, portanto A COMUNICAÇÃO OCORRE ENTRE O
    ESP32 E A APLICAÇÃO WEB DENTRO DO GOOGLE SHEETS.
    

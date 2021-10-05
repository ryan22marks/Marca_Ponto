


/*==============================================================================================================================================================================================
-----------------------------------------------------------------------------        DEFINIÇÕES DO PROGRAMA       ---------------------------------------------------------------------------------
================================================================================================================================================================================================ */
#include <EEPROM.h> // Vamos ler e escrever UIDs do PICC de / para EEPROM
#include <SPI.h> // Módulo RC522 usa protocolo SPI
#include <MFRC522.h> // Biblioteca para dispositivos Mifare RC522
#include <Keypad.h>  //Biblioteca para manipular e usar o teclado matricial
#include <LiquidCrystal_I2C.h>  //Biblioteca para o display LCD I2C
#include <Wire.h>  //Biblioteca auxiliar para usar o display LCD I2C
#include <WiFi.h> //Biblioteca de conexao em rede WiFi
#include <NTPClient.h> //Biblioteca NTPClient modificada (exibe data e hora)
#include <WiFiUdp.h> //Socket UDP
#include <GoogleSheets.h>// Biblioteca que envia dados para o Google Planilhas



//-----------------------------------------------------------------------------------------------------------------------------------
//definindo as variaveis do leitor RFID
#define EEPROM_SIZE 16//define o tamanho dos dados que irao ser salvos na 
boolean match = false;          // variavel que libera o cartao inicia como falsa
boolean programMode = false;  // o modo de programaçao inicia como falso (DESLIGADO)
boolean replaceMaster = false;
uint8_t successRead;    // Variavel que guarda o valor quando uma leitura e bem sucedida
String convertido; //armazena o ID convertido em string (char)
int convertInt[4];
byte storedCard[4];   // armazena um ID lido na EEPROM
byte readCard[4];   // Armazena o ID escaneado lido do Módulo RFID (CARTAO DE DESBLOQUEIO)
byte masterCard[4];   // Armazena a ID do cartão mestre lido na EEPROM
// Instanciando o objeto da biblioteca MFRC522.
constexpr uint8_t RST_PIN = 27;     // Define o pino de RESET
constexpr uint8_t SS_PIN = 14;     // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);//Instancia o objeto para facilitar o uso da biblioteca


//-----------------------------------------------------------------------------------------------------------------------------------
//definindo as variaveis do teclado matricial
const byte LINHAS = 4;//numero de linhas do teclado matricial
const byte COLUNAS = 4;//numero de colunas do teclado matricial
byte pinosLinhas[LINHAS] = {15,2,4,5};//pinos onde estao conectadas as linhas do teclado matricial
byte pinosColunas[COLUNAS] = {13,12,26,25};//pinos onde estao conectadas as colunas do teclado matricial
char teclado[LINHAS][COLUNAS] = {{'1','2','3','A'},
                                 {'4','5','6','B'},
                                 {'7','8','9','C'},
                                 {'*','0','#','D'}};
Keypad map_teclado = Keypad( makeKeymap(teclado), pinosLinhas, pinosColunas, LINHAS, COLUNAS );//instancia objeto que facilitara o uso da biblioteca




//-----------------------------------------------------------------------------------------------------------------------------------
//definindo as variaveis do display LCD I2C
#define endereco  0x27 // Endereços comuns: 0x27, 0x3F
#define colunas   16   //numero de colunas do display
#define linhas    2    //numero de linhas do display
LiquidCrystal_I2C lcd(endereco, colunas, linhas);//instancia objeto que facilitara o uso da biblioteca do display LCD






//-----------------------------------------------------------------------------------------------------------------------------------
//definindo as variaveis do relogio Web NTC
String WIFI_Name = "NOME DA REDE WIFI"; // Nome da rede em que o ESP32 ira se conectar
String WIFI_Pass = "SENHA DA REDE WIFI"; // Senha da rede em que o ESP32 ira se conectar
int timeZone = -3;//Fuso Horário, no caso horário de Brasília (pesquisar na internet quando ele estiver desatualizado)
struct Date{//Struct com os dados do dia e hora
    int day;//elemento da lista que armazena o dia
    int month;//elemento da lista que armazena o mes
    int year;//elemento da lista que armazena o ano
    int hours;//elemento da lista que armazena as horas
    int minutes;//elemento da lista que armazena os minutos
    int seconds;//elemento da lista que armazena os segundos
};
WiFiUDP udp;//Socket UDP que a lib utiliza para recuperar dados sobre o horário
NTPClient ntpClient(//Objeto responsável por recuperar dados sobre horário
    udp,                    //socket udp
    "0.br.pool.ntp.org",    //URL do servwer NTP
    timeZone*3600,          //Deslocamento do horário em relacão ao GMT 0
    60000);                 //Intervalo entre verificações online







//-----------------------------------------------------------------------------------------------------------------------------------
//definindo as variaveis da Plataforma do Google Sheets
String GOOGLE_SCRIPT_ID = "ID DA APLICAÇÃO WEB - GOOGLE SCRIPTS"; // ID da aplicação WEB(banco de dados no google shets)
const String unitName = "NOME DO USUÁRIO"; // nome do usuario que faz o update dos dado
GoogleSheets enviar(GOOGLE_SCRIPT_ID);





//-----------------------------------------------------------------------------------------------------------------------------------
//definindo as variaveis da senha do mestre
#define MAX_SH 10//tamanho maximo da senha 
char mestre[MAX_SH] = {'2','2','0','9','0','0','*','*','*','*'};//senha do mestre (DEFINIDA POR ELE)

//***********************************************************************************************************************************************************************************************
//***********************************************************************************************************************************************************************************************
//***********************************************************************************************************************************************************************************************










/*==============================================================================================================================================================================================
-----------------------------------------------------------------------------        DEFINIÇÕES DOS PROTOTIPOS       ----------------------------------------------------------------------------
================================================================================================================================================================================================ */

//funções referentes ao RFID
void ShowReaderDetails();//função que mostra os detalhes do modulo RFID
uint8_t getID();//função que coleta o ID do cartao inserido
void readID( uint8_t number );//função  que le o ID de um cartao salvo na memoria EEPROM
void writeID( byte a[] );//função que escreve na memoria EEPROM  o ID de um cartão inserido
void deleteID( byte a[] );//função que apaga da memoria EEPROM o ID de um cartao inserido (o cartao ja deve estar cadastrado)
boolean checkTwo ( byte a[], byte b[] );//função que checa se dois ID's são iguais
uint8_t findIDSLOT( byte find[] );//função que verifica se ainda ha espaço na memoria EEPROM
boolean findID( byte find[] );//função que procura um ID na memoria EEPROM
boolean isMaster( byte test[] );//função que verifica se o cartao inserido é o mestre
void clearMemory( );//apaga toda a memoria

//funções referentes aos modos de operação da maquina
void normalMode();//função de operação no modo normal (rotina de leitura de cartoes)
void masterMode();//função de configuração do mestre

//funçoes referentes ao teclado 
void getSenha(char senha[MAX_SH]);
boolean verificaSh ( char a[], char b[] );

//funções referentes ao relogio de tempo NTP
void initNTP();//configura o relogio de tempo ntc
void connectWiFi();//conecta o ESP32 a rede WiFi
void setupNTP();//prepara o relogio de tempo ntc
void wifiConnectionTask(void* param);//task de reconexao de rede
void exibeData();//função que exibe a data e hora do relogio NTP
Date getDate();//coleta a data do servidor Web

//***********************************************************************************************************************************************************************************************
//***********************************************************************************************************************************************************************************************
//***********************************************************************************************************************************************************************************************







/*==============================================================================================================================================================================================
---------------------------------------------------------------------------------------            SETUP       ---------------------------------------------------------------------------------
================================================================================================================================================================================================ */
void setup() {
  //---------------------------------------------------------------------------------------------------------------------------------
  //Configurações Iniciais do display LCD
  lcd.init(); // INICIA A COMUNICAÇÃO COM O DISPLAY
  lcd.backlight(); // LIGA A ILUMINAÇÃO DO DISPLAY
  lcd.clear(); // LIMPA O DISPLAY
  lcd.setCursor(2,0);//coloca o cursor na posicao 2 da linha 0
  lcd.print("Marca Ponto");
  lcd.setCursor(3,1);//coloca o cursor na posicao 3 da linha 1
  lcd.print("Eletronico");
  delay(2000);//espera 5 segundos
  //Configurações Iniciais
  Serial.begin (115200); // Inicia a comunicaçao serial com o monitor
  //Configurações Iniciais do leitor RFID
  SPI.begin (); // Inicializa o protocolo  SPI que sera usado pelo Hardware MFRC522
  mfrc522.PCD_Init (); // Inicializar Hardware MFRC522
  initNTP();//Inicializa o relogio de tempo NTP
  Serial.println(F("\t\t\t\t\tControle de Acesso RFID\n\n"));   // Envia mensagem para o monitor Serial ( DEBUG)
  ShowReaderDetails();  // Mostrar detalhes do PCD - Detalhes do leitor de cartão MFRC522
  Serial.println("\nO dispositivo esta iniciado, voce pode se conectar a ele!");//envia mensagem informando que o bluetooth esta pronto para se conectar
  //Configurações inicias da memoria EEPROM
  EEPROM.begin(EEPROM_SIZE);//inicia a memoria EEPROM com o tamanho dos dados definidos anteriormente
  //-----------------------------------------------------------------------------------------------------------------------------------
  //Verifica se ha um cartao mestre, se nao houver entao define um
  // Você pode manter outros registros EEPROM apenas escrever diferente de 143 no endereço EEPROM 1. O endereço EEPROM 1 deve conter o número mágico que é '143'
  if (EEPROM.read(1) != 143) {//se o numero lido for diferente do numero magico
    Serial.println (F ("Nenhum cartão mestre definido"));
    Serial.println (F("Insira a senha mestre para desbloquear"));

    lcd.clear();//limpa o display LCD
    lcd.print("Nenhum cartao");
    lcd.setCursor(0,1);
    lcd.print("Mestre definido");
    delay(3000);//espera 3 segundos
    lcd.clear();//limpa o display lcd
    lcd.print("Insira a senha");
    lcd.setCursor(0,3);
    lcd.print("do mestre: ");
    
    char senha[MAX_SH];//variavel que guarda a senha inserida pelo usuario
    while(1){
      getSenha(senha);//coleta a senha que o usuario inseriu
      if(verificaSh(senha, mestre)){//se a senha for igual a do mestre, entao
        lcd.clear();//limpa o display
        lcd.print("Senha Correta!");//exibe que a senha esta  correta
        delay(3000);//espera 3 segundos
        break;//sai da leitura infinita
      }else{//se a senha digitada nao for igual, entao
        Serial.println(F("Senha incorreta!!!"));//informa que a senha é incorreta
        lcd.clear();//limpa o display
        lcd.print("Senha Incorreta!");//informa que a senha é incorreta
        delay(3000);//espera 3 segundos
      }
    }
    //depois que a senha correta foi inserida
    Serial.println (F ("Insira um cartao para definir como mestre"));//pede para inserir o mestre
    lcd.clear();//limpa o dispay
    lcd.print("Insira um cartao");//pede para inserir o cartao mestre
    lcd.setCursor(0,1);
    lcd.print("para ser mestre");
    do{//laço de repetição de leitura
      successRead = getID (); //armazena se a leitura foi bem sucedida (1) ou nao (0)
    }while (! successRead); // Repete a leitura enquanto ela nao for bem sucedida
    for ( uint8_t j = 0; j < 4; j++ ) {// Repete 4 vezes
      EEPROM.write( 2 + j, readCard[j] );  // Grava o UID do PICC escaneado na EEPROM, começa a partir do endereço 3
      EEPROM.commit();//Salva o dado na EEPROM.
    }
    EEPROM.write(1, 143);// Escreva para EEPROM que o Master Card foi definido
    EEPROM.commit();//Salva o dado na EEPROM.
    Serial.println("Cartao Mestre definido");//informa que o cartao mestre foi definido
    lcd.clear();//limpa o display LCD
    lcd.print("Mestre definido");//informa que o cartao mestre foi definido
    delay(3000);//espera 3 segundos
    lcd.clear();//limpa o display
    //informa que as configurações foram concluidas
    lcd.print("Configuracao");
    lcd.setCursor(0,1);
    lcd.print("concluida");
    delay(3500);//aguarda 3,5 segundos
  }
  Serial.println(F("-------------------"));
  Serial.print(F("\nUID do Cartao mestre : "));
  for ( uint8_t i = 0; i < 4; i++ ) {          // Lê o ID do cartão mestre da EEPROM 
    masterCard[i] = EEPROM.read(2 + i);    //   Escreve no vetor de cartoes de acesso
    Serial.print(masterCard[i], HEX);//imprime no monitor serial o ID do cartao lido    
  }
  Serial.println(F("-------------------"));
  Serial.println(F("Aguardando cartoes para serem lidos"));
}
//***********************************************************************************************************************************************************************************************
//***********************************************************************************************************************************************************************************************
//***********************************************************************************************************************************************************************************************
















/*=================================================================================================================================================================================================
-------------------------------------------------------------------------------------            LOOP       ---------------------------------------------------------------------------------------
================================================================================================================================================================================================ */
void loop () {
  normalMode();//modo de operação normal da maquina
}
//***********************************************************************************************************************************************************************************************
//***********************************************************************************************************************************************************************************************
//***********************************************************************************************************************************************************************************************











//=================================================================================================================================================================================================
//------------------------------------------------------------------            FUNÇÕES AUXILIARES DO TECLADO MATRICIAL       --------------------------------------------------------------------
//=================================================================================================================================================================================================
//SO SERAO ATIVADAS SE O TECLADO JA ESTIVERE FUNCIONANDO COM OS PINOS OK
void getSenha(char senha[MAX_SH]){
    Serial.println("Insira sua senha: ");
    lcd.clear();//limpa o display
    lcd.print("Insira a senha: ");//pede para o usuario inserir a senha
    int cont=0;//variavel que vai percorrer o display e vai guardar o tamanho da senha
    while(1){//laço de leitura infinita da senha
      if(cont==MAX_SH) break;//se o tamanho da senha for igual ao limite, sai da leitura
      char key = map_teclado.getKey();//coleta a tecla apertada no teclado
      if(key != NO_KEY){//se a tecla nao foi vazia entao
          if(key == '#') break;//se a tecla foi a de finalizacao '#', entao sai do laço
          if(key == '*'){//se a tecla foi a de remover um caracter, emtao
            cont--;//volta um elemento
            if(cont<0) cont = 0;//se o elemento ja for o zero, ele nao volta mais
            senha[cont] = '*';//coloca * no elemento que se deseja apagar
            lcd.setCursor(cont,1);//coloca o cursor na posicao do elemento que se deseja apagar
            lcd.print(" ");//apaga o elemento desejado do display
          }else{//se a tecka inserida nao for '#' e nem '*', entao
            senha[cont] = key;//armazena a tecla inserida no vetor senha
            lcd.setCursor(cont,1);//coloca o cursor na posicao do elemento inserido
            lcd.print("*");//exibe o caracter * 
            cont++;//anda com o cursor e incrementa o tamanho da senha
          }
      }
    }
    //preenche com '*' os elementos da senha que nao foram inseridos
    for(int i=0;i<MAX_SH;i++){
      if(i >= cont) senha[i] = '*';//se i for maior que o tamanho da senha entao i recebe '*'
    }
}



boolean verificaSh ( char a[], char b[] ) {//verifica se duas senhas sao iguais
  boolean match;//variavel que armazena o resultado da comparacao
  if ( a[0] != 0 )      // se a primeira senha nao for vazia
    match = true;       // a variavel de estado sera verdadeira
  for ( uint8_t k = 0; k < MAX_SH; k++ ) {//laço que percorre as senhas
    if ( a[k] != b[k] ) //se um elemento de A for diferente de um elemento de B entao
      match = false;    //a variavel de estado sera falsa
  }
  if ( match ) {        // se a variavel de estado for verdadeira
    return true;        // retorna verdadeiro
  }
  else  {               // se a variavel de estado for falsa
    return false;       // retorna falso
  }
}




//=================================================================================================================================================================================================
//------------------------------------------------------------------            FUNÇÕES AUXILIARES DO LEITOR RFID       -----------------------------------------------------------------------------
//=================================================================================================================================================================================================
void normalMode(){
  Serial.println(F("\n\n\t\t\tMODO DE OPERAÇÃO. INSIRA UM CARTAO"));
  Serial.printf("\nValor de registro mestre: %d\n", EEPROM.read(1));
  do{//laço de leitura infinita
    successRead = getID (); //armazena se a leitura foi bem sucedida (1) ou nao (0)
    exibeData();//exibe a data e hora
  }while (! successRead); // Repete a leitura enquanto ela nao for bem sucedida
  if ((isMaster(readCard))){ //se o cartao lido for o mestre
    masterMode();//entra no modo mestre
    return;//retorna para o inicio da função depois de ter executado o modo mestre
  }
  if (findID(readCard)) { //se o cartao lido for cadastrado
    Serial.println(F("Bem vindo!!!!"));//informa que o usuario foi registrado
    lcd.clear();//limpa o display
    lcd.print("Bem vindo!!!");  //informa que o usuario foi registrado
    for (uint8_t j = 0; j < 4; j++) {// Repete 4 vezes
      convertInt[j] = readCard[j];//converte byte em inteiro
      convertido += String(convertInt[j], HEX);//converte inteiro em char (string)
    }
    convertido.toUpperCase();
    Serial.println("\n\tConvertido");
    String data = enviar.sendData("id=" + unitName + "&uid=" + convertido,NULL);//envia o registro para o google sheets
    enviar.HandleDataFromGoogle(data);//faz a preparação do dado
    convertido = "";//limpa a string que armazena o dado convertido
    delay(1000);//espera 2,5 segundos
  }
  if(!findID(readCard)){ //se o cartao lido não for cadastrado
    Serial.println(F("Nao cadastrado"));//informa que o usuario nao esta cadastrado
    lcd.clear();//limpa o display
    lcd.print("Nao cadastrado!!");//informa que o usuario nao esta cadastrado
    delay(2500);//espera 2,5 segundos
  }
}
void masterMode(){
  char tecla;
  Serial.println(F("\n\n\t\t\tMODO DE PROGRAMAÇÃO - MESTRE"));
  lcd.clear();
  lcd.print("Entrando no");
  lcd.setCursor(0,1);
  lcd.print("Modo mestre");
  delay(3000);
  //----------------------------------------          CARTAO MESTRE LIDO NA LOOP
  Serial.println(F("Ola Mestre, entrando no modo Programa"));
  uint8_t count = EEPROM.read(0);   // Read the first Byte of EEPROM that
  Serial.print(F("Foram encontrados "));     // stores the number of ID's in EEPROM
  Serial.print(count);
  Serial.print(F(" cartoes(s) registrados"));
  Serial.println("");
  lcd.clear();
  lcd.print("Ha");
  lcd.setCursor(3,0);
  lcd.print(count);
  lcd.setCursor(6,0);
  lcd.print("usuarios");
  lcd.setCursor(3,1);
  lcd.print("cadastrados");
  delay(3000);
  Serial.println(F("Escolha A para salvar um cartao ou B para remover um cartao"));
  lcd.clear();
  lcd.print("[A] Cadastrar");
  lcd.setCursor(0,1);
  lcd.print("[B] Remover");
  while(1){
    tecla = map_teclado.getKey();
    if(tecla != NO_KEY){
      if((tecla == 'A')||(tecla == 'B')||(tecla == 'D')) break;
      Serial.println(tecla);
    }
  }
  
  if(tecla == 'A') {                    // If scanned card is not known add it
    Serial.println(F("Encoste o cartao."));
    lcd.clear();
    lcd.print("Encoste o cartao");
    do {//Faz a leitura do cartao RFID
      successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
    }while (!successRead);   //the program will not go further while you are not getting a successful read
    Serial.println(F("Cadastrando..."));
    writeID(readCard);
    lcd.clear();
    lcd.print("Usuario");
    lcd.setCursor(0,1);
    lcd.print("Cadastrado!");
    delay(3500);
    Serial.println(F("-----------------------------"));
  }
  //--------------------------------------            REMOVE CARTAO LIDO NA LOOP
  if( tecla == 'B' ) { // If scanned card is known delete it
    Serial.println(F("Encoste o cartao."));
    lcd.clear();
    lcd.print("Encoste o cartao");
    do {//Faz a leitura do cartao RFID
      successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
    }while (!successRead);   //the program will not go further while you are not getting a successful read
    if(findID(readCard)){
      Serial.println(F("Removendo..."));
      deleteID(readCard);
      lcd.clear();
      lcd.print("Usuario");
      lcd.setCursor(0,1);
      lcd.print("Removido!");
      delay(3500);//espera 3,5 segundos
      Serial.println("-----------------------------");
    }else{
      Serial.println(F("Cartao nao encontrado!!"));
      lcd.clear();
      lcd.print("Nao encontrado");
      delay(3000);//espera 3,5 segundos
    }
  }if(tecla == 'D'){
      clearMemory( );
  }
}




///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("UID do cartao lido:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("Versao do Software: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (desconhecido) provavelmente um clone chines?"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("AVISO: Falha de comunicação, o MFRC522 está conectado corretamente?"));
    Serial.println(F("SISTEMA INTERROMPIDO. Verifique as conexoes."));
    lcd.clear();
    lcd.print("Falha no sistema");
    lcd.setCursor(0,1);
    lcd.print("Veja as conexoes");
    while (true); // nao avança no codigo
  }
}


//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( uint8_t number ) {
  uint8_t start = (number * 4 ) + 2;    // Figure out starting position
  for ( uint8_t i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}


///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Se o cartao ainda nao estiver cadastrado na memoria EEPROM
    uint8_t num = EEPROM.read(0);     // Coleta quantos cartoes estao cadastrados na EEPROM
    uint8_t start = ( num * 4 ) + 6;  // calcula o ponto de partida para gravar o novo ID
    num++;                // incrementa a variavel que guarda quantos cartoes cadastrados ha (pois esta sendo cadastrado mais um cartao)
    EEPROM.write( 0, num );     //escreve na memoria EEPROM o novo numero de cartoes cadastrados
    EEPROM.commit();
    for ( uint8_t j = 0; j < 4; j++ ) {   // loop que percorre o cartao ee cadastra ele na memoria
      EEPROM.write( start + j, a[j] );  // escreve o ID do cartao lido na memoria EEPROM, a partir do ponto de partida calculado
      EEPROM.commit();//Salva o dado na EEPROM.
    }
    Serial.println(F("Registro ID adicionado com sucesso a EEPROM"));
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    Serial.println(F("Cartao nao cadastrado"));
    lcd.clear();
    lcd.print("Cartao nao");
    lcd.setCursor(0,4);
    lcd.print("Encntrado");
    delay(3500);
  }
  else {
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    EEPROM.commit();
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
      EEPROM.commit();
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
      EEPROM.commit();
    }
    Serial.println (F ("Registro de ID removido com sucesso da EEPROM"));
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != 0 )      // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
      break;          // Stop looking we found it
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
      break;  // Stop looking we found it
    }
    else {    // If not, return false
    }
  }
  return false;
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}


//////////////////////   Limpa todos os cartoes registrados    ///////////////////////////////////
void clearMemory( ){  // quando o pino do botão Limpeza Geral pressionado, ele e setado em baixo
      Serial.println(F("Iniciando a limpeza da EEPROM"));
      lcd.clear();
      lcd.print("Apagando");
      lcd.setCursor(0,1);
      lcd.print("Registros...");
      delay(1500);//espera 1,5 segundos
      for (uint16_t x = 0; x < EEPROM.length(); x++) {    //Loop end of EEPROM address
        EEPROM.write(x, 0);//senao, limpa o enderço de memoria da EEPROM
        EEPROM.commit();
      }
      Serial.println(F("EEPROM apagada com sucesso"));//informa que a memoria EEPROM foi apagada
      lcd.clear();
      lcd.print("Memoria Apagada");
      delay(1500);//espera 1,5 segundos
}




//=================================================================================================================================================================================================
//--------------------------------------------------------------            FUNÇÕES AUXILIARES DO RELOGIO DE TEMPO NTP       -------------------------------------------------------------------
//================================================================================================================================================================================================ 
void initNTP( ){
    connectWiFi();//função de conexao com a rede
    setupNTP();//funcao de preparação do relogio NTP

    
    xTaskCreatePinnedToCore(//Cria uma nova tarefa no core 0
        wifiConnectionTask,     //Função que será executada
        "wifiConnectionTask",   //Nome da tarefa
        10000,                  //Tamanho da memória disponível (em WORDs)
        NULL,                   //Não vamos passar nenhum parametro
        2,                      //prioridade
        NULL,                   //Não precisamos de referência para a tarefa
        0);                     //Número do core que vai executar a tarefa
}


void setupNTP(){
    ntpClient.begin();//Inicializa o client NTP
    Serial.println("//Espera pelo primeiro update online");
    lcd.clear();
    lcd.print("Conectando-se ao");
    lcd.setCursor(0,1);
    lcd.print("do NTP...");
    while(!ntpClient.update()){//repete o primeiro update enquanto ele nao for bem sussedido
        Serial.print(".");//ponto (espere no monitor)
        ntpClient.forceUpdate();//força o primeiro update
        delay(500);//espera meio segundo
    }
    lcd.clear();
    lcd.print("Update concluido.");
    Serial.println();//quebra de linha no monitor serial
    Serial.println("Primeiro update Completo");
    delay(2000);//espera 2 segundos
}


void wifiConnectionTask(void* param){//Tarefa que verifica se a conexão caiu e tenta reconectar
    while(true){
        if(WiFi.status() != WL_CONNECTED){//Se o WiFi não está conectada
            connectWiFi();//Manda conectar
        }
        vTaskDelay(100);//Delay de 100 ticks
    }
}

void connectWiFi(){
    Serial.println("Conectando...");
    lcd.clear();
    lcd.print("Conectando-se a");
    lcd.setCursor(0,1);
    lcd.print("Rede: ");
    lcd.setCursor(6,1);
    lcd.print(WIFI_Name);
    WiFi.begin("Aqui Nao", "Samuel123");//Convem ocultar os dados da rede
    while(WiFi.status() != WL_CONNECTED){//Espera enquanto não estiver conectado
        Serial.print(".");//espere no monitor serial
        delay(500);//atraso de meio segundo
    }

    Serial.println();
    Serial.print("Conectado a rede: ");
    lcd.clear();
    lcd.print("Conectado !!!");
    Serial.println(WiFi.SSID());//informa o nome da rede a qual o ESP32 se conectou
    delay(2000);//espera dois segundos
}


void exibeData(){
  Date date = getDate();//Recupera os dados sobre a data e horário
  lcd.clear();//limpa o display
  lcd.print("Data:");
  lcd.setCursor(6,0);
  lcd.print(date.day);
  lcd.setCursor(8,0);
  lcd.print("/");
  lcd.setCursor(9,0);
  lcd.print(date.month);
  lcd.setCursor(11,0);
  lcd.print("/");
  lcd.setCursor(12,0);
  lcd.print(date.year);
  
  lcd.setCursor(0,1);
  lcd.print("Hora:");
  lcd.setCursor(6,1);
  lcd.print(date.hours);
  lcd.setCursor(8,1);
  lcd.print(":");
  lcd.setCursor(9,1);
  lcd.print(date.minutes);
  lcd.setCursor(11,1);
  lcd.print(":");
  lcd.setCursor(12,1);
  lcd.print(date.seconds);
  delay(100);
}



Date getDate(){
    char* strDate = (char*)ntpClient.getFormattedDate().c_str();//Recupera os dados de data e horário usando o client NTP
    Date date;//cria uma lista chamada date
    //Passa os dados da string para a struct
    sscanf(strDate, "%d-%d-%dT%d:%d:%dZ", 
                    &date.year, 
                    &date.month, 
                    &date.day, 
                    &date.hours, 
                    &date.minutes,
                    &date.seconds);
    return date;
}

# include  < TimerOne.h >  // Precisamos disso para PWM de alta resolução

# define  MOTOR_A1_PIN  7   	// Conectado ao pino INA do motor DRIVER VNH2SP30
# define  MOTOR_B1_PIN  8 		// Conectado ao pino INB do motor VNH2SP30 DRIVER
# define  PWM_OUTPUT_PIN  9 	// Deve ser o pino 9 ou 10, os únicos pinos do Arduino para os quais o PWM de alta resolução (TimerOne) funciona
# define  PERIOD_MICROSEC  16666    // 16666us = 60Hz Uma baixa frequência é necessária para fazer o motor DC barato funcionar em ciclos de trabalho baixos (10%), mas frequências muito baixas produzem vibrações visíveis na visão do telescópio. Descobri que 60 Hz funcionou bem para visual (C90) e exposição de foto EAA / vídeo (refrator de 80 mm)
# define  COUNTLIMIT  2046    // Deve ser 1023 ou um múltiplo dele, já que 1023 é a resolução padrão do TimerOne e é suficiente para o ciclo de trabalho de resolução de 0,1%.
                          // Assim, estou usando o dobro disso como limite de contagem (2046), para depurar o codificador rotativo KY: ele sempre me dá 2 pulsos por "clique"
                          // Fazer isso é mais simples do que usar hardware de depuração (capacitores, resistores, etc).

const  int membranaButtonPins [] = {A1, A0, A3, A2}; // {key1, key2, key3, key4} pinos dos 4 botões do teclado de membrana no meu, mas fontes online disseram que a ordem era diferente. Melhor testar-se com um multímetro e reorganizar a ordem nesta linha de código.  

/ * * Código para a tela de 7 segmentos
 * É um ânodo comum, baseado em TM74HC595, display LED de 7 segmentos e 4 dígitos que comprei em:
 * https://www.aliexpress.com/item/-/1688259613.html
 * /

# define  RCLK_PIN  5 	// Conectado ao pino RCLK
# define  SCLK_PIN  6 	// Conectado ao pino SCLK
# define  DIO_PIN  4 	// Conectado ao pino DIO

boolean displayIsOn = true ;
const  static byte bits [] = // usado para mapear um byte (dígito inteiro) para os bits necessários para exibi-lo no led de 7 segmentos
    {
        0b11000000 , // 0
        0b11111001 , // 1
        0b10100100 , // 2
        0b10110000 , // 3
        0b10011001 , // 4
        0b10010010 , // 5
        0b10000010 , // 6
        0b11111000 , // 7
        0b10000000 , // 8
        0b10010000 , // 9
    };

void  display ( float percent) // exibe um número de porcentagem de 0f a 1,0f no display de 4 dígitos como uma porcentagem, por exemplo: 0,1599 é exibido como "15,99")
  {
  primeiro dígito int = porcentagem * 10 ;
  segundo dígito int = porcentagem * 100 - (primeiro dígito * 10 );
  terceiro dígito int = porcentagem * 1000 - ((primeiro dígito * 100 ) + (segundo dígito * 10 ));
  int quartoDígito = porcentagem * 10000 - ((primeiro dígito * 1000 ) + (segundo dígito * 100 ) + (terceiro dígito * 10 ));

  dígitos de byte [] = {primeiro dígito, segundo dígito, terceiro dígito, quarto dígito};
  para ( int i = 0 ; i < 4 ; i ++)
    {
    displayNumber (dígitos [i], i, i == 1 );
    }
  }

exibição de byte const [] = { 0b00001000 , 0b00000100 , 0b00000010 , 0b00000001 };
/ * *
 * Exibir um dígito em um dos quatro visores de 7 segmentos. 
 * "número" é o número a ser exibido (de 0 a 9)
* "dígito" é o identificador de qual dos quatro visores de 7 segmentos enviará o número. 0 é o mais à esquerda, 4 é o mais à direita
 * "withDecimalDot" indica se deve iluminar o ponto decimal para este número particular
 * /
void  displayNumber (número do byte, dígito do byte, booleano comDecimalDot)
{
  if (displayIsOn)
    {
    digitalWrite (RCLK_PIN, LOW);
    shiftOut (DIO_PIN, SCLK_PIN, MSBFIRST, withDecimalDot? bits [número] & 0b01111111 : bits [número]);
    shiftOut (DIO_PIN, SCLK_PIN, MSBFIRST, exibe [dígito]);
    digitalWrite (RCLK_PIN, HIGH);
    atraso ( 1 );
    }
    outro
    {
    digitalWrite (RCLK_PIN, LOW);
    shiftOut (DIO_PIN, SCLK_PIN, MSBFIRST, 0b11111111 );
    shiftOut (DIO_PIN, SCLK_PIN, MSBFIRST, exibe [dígito]);
    digitalWrite (RCLK_PIN, HIGH);
    atraso ( 1 );
    }
}


/ * * Código para o codificador giratório KY-040
 * Com base no código de http://domoticx.com/arduino-rotary-encoder-keyes-ky-040/ 
 * /

# define  KY40_PIN_A    2   // Conectado ao pino CLK do codificador giratório KY-040
# define  KY40_PIN_B    3   // Conectado ao pino DT do codificador giratório KY-040
# define  KY40_BUTTON_PIN    12   // Conectado ao pino SW do codificador giratório KY-040
// Não se esqueça de conectar o VCC (às vezes rotulado como "+") KY-040 ao Arduino VCC

longo timeOfLastEvent = 0 ; // usado para depurar o codificador rotativo no software
boolean pwmUpdateNeeded = true ;
  byte estático abOld;     // estado de inicialização
 contagem de int voláteis = COUNTLIMIT / 13 ;     // contagem rotativa atual, inicializada em 1/13 (ou 7,6%) do máx.
int old_count;     // contagem rotativa antiga

// Na interrupção, leia os pinos de entrada, calcule o novo estado e ajuste a contagem
enum {upMask = 0x66 , downMask = 0x99 };
void  pinChangeISR () {
  byte abNew = ( digitalRead (KY40_PIN_A) << 1 ) | digitalRead (KY40_PIN_B);
  critério de byte = abNew ^ abOld;
  if (critério == 1 || critério == 2 ) {
    if (upMask & ( 1 << ( 2 * abOld + abNew / 2 )))
      contagem ++;
    senão contar--;       // upMask = ~ downMask
  }
  abOld = abNew;        // Salvar novo estado

pwmUpdateNeeded = true ;
if (contagem < 0 ) contagem = 0 ;
if (contagem> COUNTLIMIT) contagem = COUNTLIMIT;
}

long lastpressed = 0 ;
void  checkKYbutton ()	 // se o botão no codificador KY-040 for pressionado, nós o usamos para ligar / desligar o display de led (para evitar a adaptação ao escuro)
{
  botão booleano = digitalRead (KY40_BUTTON_PIN);
  if ( millis () - lastpressed> 500 &&! button) // debouncing de software simples do botão de pressão
  {
    displayIsOn =! displayIsOn;
    lastpressed = millis ();
  }
}

/ * ************************************************* ***********************
 * Configuração principal do Arduino e código de loop
 * /

boolean forward = true ;
booleano interrompido = falso ;
velocidade booleana2x = false ;
boolean speedFull = false ;
float dutyCycle = 8 . 0f ;  // velocidade padrão do motor

 configuração vazia ()                         
{
  // configurando os pinos do teclado de membrana:
  para ( int i = 0 ; i < sizeof (membranaButtonPins) / sizeof ( int ); i ++)
  {
    pinMode (membranaButtonPins [i], INPUT_PULLUP);
  }  
  
  // configurando pinos de exibição:
  pinMode (RCLK_PIN, OUTPUT);
  pinMode (SCLK_PIN, OUTPUT);
  pinMode (DIO_PIN, OUTPUT);
  
  // Configurando interrupções para codificador rotativo (entrada do usuário):
  pinMode (KY40_BUTTON_PIN, INPUT_PULLUP);
  pinMode (KY40_PIN_A, INPUT_PULLUP);
  pinMode (KY40_PIN_B, INPUT_PULLUP);
  attachInterrupt ( 0 , pinChangeISR, CHANGE); // Configurar interrupções de mudança de pino
  attachInterrupt ( 1 , pinChangeISR, CHANGE);

  // Configurar Timer1 para PWM:
  Timer1. inicializar (PERIOD_MICROSEC);
    
  // Configure os pinos do driver do motor VNH2SP30:
  pinMode (MOTOR_A1_PIN, OUTPUT);
  pinMode (MOTOR_B1_PIN, OUTPUT);

  Serial. começar ( 9600 );              
}

void  loop ()
{
  checkKYbutton (); // verifique se o botão para alternar a exibição de LED foi pressionado e faça de acordo.

  int membranaButtonStatus [] = { 1 , 1 , 1 , 1 };
  for ( int i = 0 ; i < sizeof (membranaButtonPins) / sizeof ( int ); i ++)	 // verificar o status dos 4 botões de membrana
  {
    int newStatus = digitalRead (membranaButtonPins [i]);
    membranaButtonStatus [i] = newStatus;
  }

  boolean oldForward = forward;
  forward = membranaButtonStatus [ 0 ] + membranaButtonStatus [ 1 ]> = 2 ;
  boolean oldSpeed2x = speed2x;
  speed2x = membranaButtonStatus [ 1 ] + membranaButtonStatus [ 2 ] < 2 ;
  boolean oldSpeedFull = speedFull;
  speedFull = membranaButtonStatus [ 0 ] + membranaButtonStatus [ 3 ] < 2 ;

  if (forward! = oldForward || speed2x! = oldSpeed2x || speedFull! = oldSpeedFull) // se o estado mudou em um dos botões
  {
    pwmUpdateNeeded = true ;	
  }

  if (pwmUpdateNeeded) // Se uma interrupção (do codificador rotativo) ou um dos 4 botões acima sinalizou que a velocidade PWM precisa ser atualizada
  {
    pwmUpdateNeeded = false ;
    if (encaminhar)
    {
//       Serial.println ("encaminhar");
      digitalWrite (MOTOR_A1_PIN, LOW);
      digitalWrite (MOTOR_B1_PIN, HIGH);
    }
    else  if (! parou)
    {
//       Serial.println ("reverso");
      digitalWrite (MOTOR_A1_PIN, HIGH);
      digitalWrite (MOTOR_B1_PIN, LOW);      
    }
    outro
    {
//       Serial.println ("parar");
      digitalWrite (MOTOR_A1_PIN, LOW);
      digitalWrite (MOTOR_B1_PIN, LOW);            
    }
  
    dutyCycle = (( float ) count * 100 ) / (( float ) COUNTLIMIT);
    int pwm_duty_cycle = (dutyCycle / 100 ) * 1023 ;
    if (speed2x) {pwm_duty_cycle * = 2 ; }
    else  if (speedFull) {pwm_duty_cycle = 1023 ;}
    Timer1. PWM (PWM_OUTPUT_PIN, pwm_duty_cycle);
  }

 display (dutyCycle / 100.0 );	// exibe o ciclo de trabalho como porcentagem nos leds de 7 segmentos.
}



// includes
#include <util/delay.h>
#include <display.h>
#include <leds.h>
#include <Potentiometer.h>
#include <usart.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <button.h>
#include <Buzzer.h>
#include <stdbool.h>
#include <stdlib.h>

// declaraties
void startGame(void);
void showScreen(void);
void throwDice(void);
void changePlayer(void);
void checkEndGame(void);
void initTimer0(void);
void computer(void);

#define BUTTON_PORT PORTC
#define BUTTON_PIN PINC
#define BUTTON_DDR DDRC
#define BUTTON1 PC1
#define BUTTON2 PC2
#define BUTTON3 PC3

// als speler 0 is is het de Computer als speler 1 is het de echte persoon als speler
int player;
int gameStarted = 0;
int diceSize = 6;
int sumOfThrows;
int throw = 0;
int amountOfThrows = 0;
int playOn = 0;

// index 0 is altijd de computer en de speler is index 1
int scoresplayers[] = {0, 0};
char letterplayers[] = {'C', 'P'};

// Deze ISR wordt aangeroepen als Pin Change Interrupt 1 afgaat (PCINT1_vect)
// Dit is de interrupt voor PORTC, die waarop de knopjes hangen...
ISR(PCINT1_vect)
{
  // knop 1 is ingedrukt (bit staat op 0)?
  if (bit_is_clear(BUTTON_PIN, BUTTON1) || bit_is_clear(BUTTON_PIN, BUTTON2) || bit_is_clear(BUTTON_PIN, BUTTON3))
  {
    // We wachten 1000 microseconden en checken dan nog eens (debounce!)
    _delay_us(1000);
    // knop 1 is ingedrukt (bit staat op 0)?
    if ((bit_is_clear(BUTTON_PIN, BUTTON1) || bit_is_clear(BUTTON_PIN, BUTTON2) || bit_is_clear(BUTTON_PIN, BUTTON3)) && gameStarted == 0 )
    {
      while (buttonPushed(1) || buttonPushed(2) || buttonPushed(3))
      {
        startGame();
      }
      printf("\n\nSpel gestart speler die mag beginnen is: %c\n", letterplayers[player]);
    }
    if (gameStarted == 1 && bit_is_clear(BUTTON_PIN, BUTTON1)&& playOn == 1)
    {
      throwDice();
      if (player == 0)
      {
        computer();
      }
    }
    if (gameStarted == 1 && bit_is_clear(BUTTON_PIN, BUTTON3))
    {
      initBuzzer();
      playOneUpSound();
      scoresplayers[player] += sumOfThrows;
      changePlayer();
    }
  }
}

int main(void)
{
  initUSART();
  initDisplay();
  initPotentiometer();
  enableAllButtons();
  initTimer0();

  printf("\n\ndruk op eender welke knop om het spel te starten");
  BUTTON_DDR &= ~_BV(BUTTON1); // we gaan alle knoppen gebruiken gebruiken
  BUTTON_DDR &= ~_BV(BUTTON2);
  BUTTON_DDR &= ~_BV(BUTTON3);

  BUTTON_PORT |= _BV(BUTTON1); // pull up aanzetten
  BUTTON_PORT |= _BV(BUTTON2);
  BUTTON_PORT |= _BV(BUTTON3);

  PCICR |= _BV(PCIE1);    // in Pin Change Interrupt Control Register: geef aan
                          // welke interrupt(s) je wil activeren (PCIE0: poort B,
                          // PCIE1: poort C, PCIE2: poort D)
  PCMSK1 |= _BV(BUTTON1); // in overeenkomstig Pin Change Mask Register: geef
  PCMSK1 |= _BV(BUTTON2); // aan welke pin(s) van die poort de ISR activeren
  PCMSK1 |= _BV(BUTTON3);

  sei(); // Set Enable Interrupts --> globaal interrupt systeem aanzetten
  while (1)
  {
  }
}

void startGame()
{
  srand(getPotentiometerWaarde());
  if ((rand() % getPotentiometerWaarde()) % 2 == 0)
  {
    player = 0;
    playOn = 1;
  }

  else
  {
    player = 1;
  }
  gameStarted = 1;
}

void showScreen()
{
  if (gameStarted == 0)
  {
    writeNumberToSegment(1, 9);
    writeNumberToSegment(2, 9);
  }
  else
  {
    writeCharToSegment(1, letterplayers[player]);
    writeNumberToTwoSegments(1, sumOfThrows);

    if (amountOfThrows > 0)
    {
      writeNumberToSegment(0, throw);
    }
  }
}

void throwDice()
{
  throw = (rand() % 6) + 1;
  printf("dobbelsteen geworpen worp is: %d\n", throw);
  amountOfThrows++;
  sumOfThrows += throw;
  if (throw == 1)
  {
    showScreen();
    playFireBallSound();
    // playTone(getPotentiometerWaarde(), 500);
    changePlayer();
  }
}

void changePlayer()
{
  // De speler zijn worp terug op nul zetten
  throw = 0;
  // van speler veranderen
  if (player == 0)
  {
    do
    {
      enableButton(2);
    } while (!buttonPushed(3));
    player = 1;
  }
  else
  {
    disableButton(3);
    player = 0;
    playOn = 1;
  }
  printf("\nveranderd van speler de speler is nu: %c\n", letterplayers[player]);
  sumOfThrows = scoresplayers[player];
}

void initTimer0()
{
  // STAP 1: kies de WAVE FORM en dus de Mode of Operation
  // Hier kiezen we FAST PWM waardoor de TCNT0 steeds tot 255 telt
  TCCR0A |= _BV(WGM00) | _BV(WGM01); // WGM00 = 1 en WGM01 = 1 --> Fast PWM Mode

  // STAP 2: stel *altijd* een PRESCALER in, anders telt hij niet.
  // De snelheid van tellen wordt bepaald door de CPU-klok (16Mhz) gedeeld door deze factor.
  TCCR0B |= _BV(CS00); // CS02 = 1 en CS00 = 1 --> prescalerfactor is nu 1024 (=elke 64µs)

  // STAP 3: enable INTERRUPTs
  // Enable interrupts voor twee gevallen: TCNT0 == TOP en TCNT0 == OCR0A
  TIMSK0 |= _BV(TOIE0);  // overflow interrupt enablen
  TIMSK0 |= _BV(OCIE0A); // OCRA interrupt enablen

  sei(); // interrupts globaal enablen
}

// deze ISR runt telkens wanneer TCNT0 gelijk is aan de waarde in het OCRA-register
ISR(TIMER0_COMPA_vect)
{
  showScreen();
  if (sumOfThrows >= 99)
  {
    if (player == 1)
    {
      scrollingText("De computer is gewonnen!");
    }
    else
    {
      scrollingText("De speler is gewonnen!");
    }

    gameStarted = 0;
  }

  PORTB &= ~(_BV(PB2) | _BV(PB3) | _BV(PB4) | _BV(PB5));
}

// deze ISR runt telkens wanneer TCNT0 gelijk is aan de waarde TOP-waarde (255)
ISR(TIMER0_OVF_vect)
{
  showScreen();
  PORTB |= _BV(PB2) | _BV(PB3) | _BV(PB4) | _BV(PB5);
}

// inteligentie van de computer
void computer()
{
  if (sumOfThrows < 10)
  {
    playOn = 1;
  }
  if (sumOfThrows > 15)
  {
    playOn = 0;
  }
  if (scoresplayers[1] > 80 && playOn == 0 && (rand() % 100)<= 80)
  {
    scoresplayers[1]-= sumOfThrows;
  }
}
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

// declaraties
// als speler 0 is is het de Computer als speler 1 is het de echte persoon als speler

#define BUTTON_PORT PORTC
#define BUTTON_PIN PINC
#define BUTTON_DDR DDRC
#define BUTTON1 PC1
#define BUTTON2 PC2
#define BUTTON3 PC3

int speler;
int gameStarted = 0;

int totaalComputer = 0;
int totaalSpeler = 0;

int worp;

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
    if ((bit_is_clear(BUTTON_PIN, BUTTON1) || bit_is_clear(BUTTON_PIN, BUTTON2) || bit_is_clear(BUTTON_PIN, BUTTON3)) && gameStarted == 0)
    {
      gameStarted = 1;
      startSpel();
    }
  }
}

int main(void)
{
  initUSART();
  initDisplay();

  startScreen();
}
void startScreen()
{
  printf("\n\ndruk op eender welke knop om het spel te starten");
  BUTTON_DDR &= ~_BV(BUTTON1); // we gaan alle knoppen gebruiken gebruiken
  BUTTON_DDR &= ~_BV(BUTTON2);
  BUTTON_DDR &= ~_BV(BUTTON3);
  BUTTON_PORT |= _BV(BUTTON1); // pull up aanzetten
  PCICR |= _BV(PCIE1);         // in Pin Change Interrupt Control Register: geef aan
                               // welke interrupt(s) je wil activeren (PCIE0: poort B,
                               // PCIE1: poort C, PCIE2: poort D)
  PCMSK1 |= _BV(BUTTON1);      // in overeenkomstig Pin Change Mask Register: geef
  PCMSK1 |= _BV(BUTTON2);      // aan welke pin(s) van die poort de ISR activeren
  PCMSK1 |= _BV(BUTTON3);

  sei(); // Set Enable Interrupts --> globaal interrupt systeem aanzetten
  while (1)
  {
    toonscherm();
  }
}

void startSpel()
{
  printf("\n\nspel gestart");

  if (random(100) % 2 == 0)
  {
    speler = 0;
  }

  else
  {
    speler = 1;
  }

  toonscherm();
}

void toonscherm()
{
  if (gameStarted == 0)
  {
    writeCharToSegment(1, '9');
    writeCharToSegment(2, '9');
  }
  else
  {
    if (speler == 0)
    {
      writeCharToSegment(1, 'C');
    }

    else
    {
      writeCharToSegment(1, 'P');
    }
  }
}

void timer()
{
  // STAP 1: kies de WAVE FORM en dus de Mode of Operation
  // Hier kiezen we FAST PWM waardoor de TCNT0 steeds tot 255 telt
  // TCCR0A |= _BV(WGM00) | _BV(WGM01); // WGM00 = 1 en WGM01 = 1 --> Fast PWM Mode
}
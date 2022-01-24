/*

Alarm do ciężaróweczki
autor: zielo

2019-03-22

 */

#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#define PIN_DL (1 << PB2) // kontaktron drzwi lewych
#define PIN_DR (1 << PB1) // kontaktron drzwi prawych
//#defube PIN_SW (1 << PB4) // przełącznik
#define PORT_LED (1 << PB4) // dioda sygnalizacycjna
#define PORT_SIGNAL (1 << PB0) // syrana (poprzez tranzystor)


//// deklaracje fcji
void wd_setup(); // watch-dog
void tim_setup(); // licznik

// zmienne globalne
volatile uint8_t qs,s,m;
volatile uint8_t wd_count __attribute__ ((section (".noinit"))); // przetrwać reset :D

int main(void)
{
    uint8_t mcusr_cpy;
    mcusr_cpy = MCUSR;
    MCUSR = 0; // czyścimy status watchdoga
    wdt_disable(); // watchdog off (po resecie)

    tim_setup(); // konf timera
    sei(); // global interrupt on ;)

    DDRB |= PORT_SIGNAL | PORT_LED; // syrana jako wyjście
    PORTB |= PIN_DL | PIN_DR; // pull up do kontaktronów
    uint8_t l=0,alarm=0,danger=0,d_l=0,a_w=0;

    if (!(mcusr_cpy & (1 << WDRF)) && a_w == 0) // 1. urchomienie
    {
        while (s<20) // 10s na uzbrojenie
        {
            if (s%2)
                PORTB |= PORT_LED; //migamy
            else
                PORTB &= ~PORT_LED;
        }
        PORTB &= ~(PORT_LED); //gasimy
    }


    wd_count++; // licznik restartów watchdoga
    while(1)
    {
        if ( (PINB & PIN_DL) || (PINB & PIN_DR) ) // obwód przerwany - zachowany satn wysoki
        {
            danger = 1;
            d_l = 1; //blokada alarmu
            a_w = 1; //alarm wystąpił
        }
        else
            danger = 0;

        if (d_l)
        {
            if (l==0){l=1;s=0; m=0;} // zeruję liczniki
            //else if (l==1 && danger) {s=3;m=0;}
            if (s >= 3 && m < 4)
            {
                alarm = 1;
            }
            else if (m >=4 && !danger) // ustoąpiło zagrożenie i upłunął odpowiedni czas
            {
                alarm = 0;
                d_l = 0;
                l = 0;
            }
        }

        if (d_l || a_w) // alarm się włącza lub był włączony
        {
            if (qs%4 < 2)
                PORTB |= PORT_LED;
            else
                PORTB &= ~(PORT_LED);
        }

        if (alarm) //syrena
            PORTB |= PORT_SIGNAL;
        else
            PORTB &= ~(PORT_SIGNAL);

        if (!a_w) // stan czuwania migamy diodą raz na jakiś czas
        {
            if (wd_count % 16 == 0)
                PORTB |= PORT_LED;
            wd_setup();
        }


    }
    return 0;
}

//// definicje fcji

void wd_setup()
{
    wdt_enable(WDTO_250MS);
    cli(); // wyłączam przerwania
    PORTB &= ~(PIN_DL | PIN_DR); // zdjąć pull_up z kontaktronów
    //DDRB = 0; // wszystko na in
    //PORTB = 0; // zdjąć pullupy
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode();
}

void tim_setup()
{
    TCCR1 |= (1 << CTC1) | (1 << CS13) | (1 << CS11) | (1 << CS10); // ctc, prescaler = 1024;
    OCR1C = 122; // 8x na sekundę
    OCR1A = OCR1C; // do porównania
    TIMSK |= (1 << OCIE1A); // przerwanie licznika
}

ISR(TIMER1_COMPA_vect)
{
    if (++qs == 8)
    {
        qs = 0;
        if (++s == 60)
        {
            s = 0;
            if (++m == 60)
                m = 0;
        }
    }
}


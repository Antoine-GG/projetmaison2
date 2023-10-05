/*
 * main.c
 *
 * Created: 9/1/2023 12:21:16 PM
 *  Author: 2161908
 */ 
#define F_CPU 1000000UL
#include <util/delay.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>

#define DDR_SPI DDRB
#define CS 2
#define CS_SoftCode 1
#define MOSI 3
#define MISO 4
#define SCK 5


void SPI_MasterInit()
{
	/* Set MOSI, CS and SCK output, all others input */
	DDR_SPI = (1<<CS)|(1<<MOSI)|(1<<SCK)|(1<<CS_SoftCode);
	
	/* Enable SPI, Master, (j'ai aussi fuck avec le clock)set clock rate fck/16 */
	//SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA)|(1<<SPR1);
	/* test pour voir si fonction avec les 3 chip sans la saleae */
	SPCR = (1<<SPE)|(1<<MSTR);
	
	/* pour tous les mettres passif (les Slaves) */
	PORTB |= (1<<CS)|(1<<CS_SoftCode);
	_delay_ms(10);
}
char SPI_MasterTransmit_Receive(char cData)
{	
	/* Start transmission */
	SPDR = cData;

	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF))); //le ! veux dire while not (so basicly... SPIF veut dire que les 8 bytes ont ete Read/Wright donc cest basicly un loop qui t'empeche d'envoyer plus information avant que le 8 bytes a été envoyé)

	return SPDR;
}
void Blink(){
	
	PORTC |= (1<<0);
	_delay_ms(1000);
	PORTC &= ~(1<<0);
	_delay_ms(1000);
	PORTC |= (1<<1);
	_delay_ms(1000);
	PORTC &= ~(1<<1);
	_delay_ms(1000);
	
}

int main(void)
{
	/* mettre les deux portes en sortie */
	DDRC |= (1<<0)|(1<<1);
	/* test de lumière pour confirmer */
	Blink();
	/* initialiser le master SPI */
	SPI_MasterInit();
	
	/* pour envoyer plusieurs char test */
	char Master_data[] = {0x55,0x33,0x44};
	
	//ici cest i<4 cuz quand tu envoie au slave et il change sont registre SPDR, il va etre envoié dans le prochain echange de SPDR, comme si il y avait un delay de 1 char quand le slave parle au master
	for (int i=0; i<4; i++)
	{
		PORTB &= ~(1<<CS); //pour activer slave 1
		_delay_ms(10);
		
		char data = SPI_MasterTransmit_Receive(Master_data[i]); 
			
		if(data == 0x51){
			PORTC |= (1<<0);
		}
		if(data == 0x31){
			PORTC |= (1<<1);
		}
		if(data == 0xAA){
			PORTC &= ~(1<<0);
			PORTC &= ~(1<<1);
		}
		PORTB |= (1<<CS); //pour mettre slave 1 a passive
		_delay_ms(1000);
	}
	
	for (int i=0; i<4; i++)
	{
		PORTB &= ~(1<<CS_SoftCode);//pour activer slave 2
		_delay_ms(10); //ici le delay est important cuz sinon il active le slave en meme temps que le slave veux parler so ca fuck up
		
		char data = SPI_MasterTransmit_Receive(Master_data[i]);
		
		if(data == 0x51){
			PORTC |= (1<<0);
		}
		if(data == 0x31){
			PORTC |= (1<<1);
		}
		if(data == 0xAA){
			PORTC &= ~(1<<0);
			PORTC &= ~(1<<1);
		}
		PORTB |= (1<<CS_SoftCode); //pour mettre slave 1 a passive
		_delay_ms(1000);
	}
	
	return(0);
}


/* 
IMPORTANT !!!!
le master et le slave peuvent utiliser le même registre car les deux sont en train de 
byte shift en meme temps comme SPDR master envoye 01010101 au SPDR slave et a mesure
que le SPDR slave envoie par pushing les bytes vers le master, il est en train de recevoir
les bytes 1 par 1 en meme temps
Master ==> 01010101 
Slave ==> 00110011
master envoie 0,1,0,1,0,1,0,1 
master recoit 0,0,1,1,0,0,1,1 
son SPDR registre ce vide pour faire de la place au SPDR du slave a mesure qu'il envoie de bite
*/

/*
IMPORTANT(2)!!!
conceil de idir 
btw tavais pas mis le softcode cs en output avec le master chip
aussi a chaque fois que tu fais un transmission de donnée, tu dois
en premier activer le salve 
transmettre donnée
puis desactiver le slave
Cela assure qu'il n'y aura pas de probleme de transmission
*/


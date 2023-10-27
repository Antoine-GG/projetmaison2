/*
 * Slave_I2C.cpp
 *
 * Created: 2023-09-28 13:46:18
 * Author : 2161908
 */ 

#define F_CPU 1000000UL
#include <util/delay.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <string.h>

#define SDA 4
#define SCL 5
#define SLave_Address1 0x50 //fait attention, pour la deuxième chip comme tu dois changer cela quand tu pograms 1er et 2ème chip
#define SLave_Address2 0xA0 //BTW ya pas le read/write bit cuz cest juste l'addresse du slave

void Blink(){
	for (int i=0; i<2; i++){
		PORTC |= (1<<0);//<================LED
		PORTC |= (1<<1);//<================LED
		_delay_ms(1000);
		PORTC &= ~(1<<0);//<================LED
		PORTC &= ~(1<<1);//<================LED
		_delay_ms(1000);
	}
}
void ShortBlink(int direction){
	if (direction == 1){//1 = left LED
		PORTC |= (1<<0);//<================LED
		_delay_ms(100);
		PORTC &= ~(1<<0);//<================LED
		_delay_ms(100);
	}
	if (direction == 2){//2 = right LED
		PORTC |= (1<<1);//<================LED
		_delay_ms(100);
		PORTC &= ~(1<<1);//<================LED
		_delay_ms(100);
	}
}
char I2C_status()
{
	char TWIstatus = TWSR & 0xF8; //ici il faut du bit masking cuz les 3 dernier bit du registre TWSR non rien a voir avec le status
	//TWSR = 0b 0000 0000
	//mais nous on veux juste TWSR = 0b 0000 0xxx qui va aller dans le TWIstatus
	return TWIstatus;
}
void TWIInit(void) //pas besoin de cette fonction apparament cest juste le master qui genere clock puls
{
	//pour assurer qu'il y a rien sur le status register au début
	TWSR = 0x00;
	//TWBR selects the division factor for the bit rate generator
	TWBR = 0x0A;
	//enable I2c
	TWCR = (1<<TWEN)|(1<<TWINT);
}
void TWIStop(void)
{
	//stop condtion = arret la communication
	// TWINT = interrupt flag = quand le transmition complete? not sure mais cest un flag (so ici il est 1 cuz il a finit de transmettre un byte et il attent softcode application)
	// TWSTO = in master mode il va faire un STOP condition
	// TWEN = peremt de controler SDA et SCL line ==> quand il est a 0 la transmission est terminé
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
	while(TWCR & (1<<TWSTO));//basicly wait until stop
}
void I2C_Slave_Init(char slave_address)
{
	TWAR = slave_address;						/* Assign address in TWI address register */
	TWCR = (1<<TWEN) | (1<<TWEA) | (1<<TWINT);	/* Enable TWI, Enable ack generation, clear TWI interrupt */
	//PORTC |= (1<<0);//<================LED  btw led fonction pas cuz init before initialisation de LED
}

int I2C_Slave_Listen() //ici le slave va ecouter le master jusqu'à tant qu'il recoi son address et un code pour soit read le byte du master ou wright un byte au master
{
	//ShortBlink(1);
	//PORTC |= (1<<0);//<================LED
	while (1)
	{
		//ShortBlink(1);
		char status = I2C_status();
		
		//while (1)			//ye ici le while loop etait useless + creer un probleme
		//{
		//ShortBlink(2);
		//----------------------------------------
		if (status == 0x60 || status == 0x68){
			//Blink();
			//PORTC |= (1<<0);//<================LED
			return 0; // ici slave address recu + write bit et le 0x68 pour quand >> lost in arbitration lost??? aussi if TWE = 1, ACK bit will be sent
			//important!!! mais des while loop pour read/write et apprend ce que le code fait ==> il reste just a faire code du slave pour les different write ack, read ack, write nack et read nack
			//ici receive cuz cest le master qui write
		}
		//----------------------------------------
		if (status == 0xA8 || status == 0xB0){
			//Blink();
			//PORTC |= (1<<0);//<================LED
			return 1; // ici slave address recu + read bit et le 0xB0 pour quand >> lost in arbitration lost??? aussi if TWE = 1, ACK bit will be sent
			//ici write cuz cest le master qui read
		}
		//----------------------------------------
		if (status == 0xA0){
			//ShortBlink(1);
			TWCR |= (1<<TWINT);
			continue; //start condition repeated or stop condition while being addressed
		}
		//}
		//lost in arbitration ==> master n'est plus un master? genre c'est comme le dernier byte a envoyer or some shit?
	}
}

int I2C_Slave_Transmmit(char data){	//faut mettre while et break pour le nack
	//Blink();
	_delay_ms(100);
	TWDR = data;
	TWCR = (1<<TWEN) | (1<<TWINT) | (1<<TWEA); //ton probleme avec le slave transmit etait que tu avais pas mis TWEA comme idir avait pas besoin puisque il transferait juste comme un byte pour lire qqch
	while (!(TWCR & (1<<TWINT)));
	char status = I2C_status();
	if (status == 0xA0){
		TWCR |= (1<<TWINT);
		return 0; // ici qqch a fuck up dans start condition or stop condition no action est on clear le flag pour continuer transmition
	}
	if (status == 0xB8){
		//ShortBlink(1);//<================LED
		return 1; // ici data byte dans TWDR a été transmit et ACK has been received and if TWEA = 1 ACK va être reçu?
	}
	if (status == 0xC0){
		//ShortBlink(2);//<================LED
		return 2; //ici data byte dans TWDR a été transmit et ACK has not been received and if TWEA = 1 ACK va être reçu?
	}
	if (status == 0xC8){ //PROBLEME!!!!!!! somehow NACK idk how check salae + aider du prof
		return 3; //Last data byte in TWDR has been transmitted (TWEA = “0”); ACK has been received
	} else {
		return 4;
	}
}

int I2C_Slave_Receive(bool ack){	//faut mettre while et break pour le nack
	//PORTC |= (1<<0);//<================LED
	if (ack == true){
		TWCR = (1<<TWEN) | (1<<TWINT) | (1<<TWEA);
		//PORTC |= (1<<0);//<================LED
	} else {
		//PORTC |= (1<<0);//<================LED
		TWCR = (1<<TWEN) | (1<<TWINT);
	}
	//PORTC |= (1<<0);//<================LED
	while (!(TWCR & (1<<TWINT)));
	//PORTC |= (1<<0);//<================LED
	char status = I2C_status();
	if (status == 0x80){
		return 0; //ici data received + ack
	}
	if (status == 0x88){ //envoie pas de ack donc arret la transmition
		return 1; //ici data received + nack
	}
	if (status == 0xA0){
		TWCR |= (1<<TWINT);//communication fuck up so clear flag et recommence communication (not sure)
		return 2;
	} else return 3;
}

void action(char data){
	switch(data){
		case 0x55:{
			PORTC |= (1<<0);//<================LED
			break;
		}
		case 0x33:{
			PORTC |= (1<<1);//<================LED
			break;
		}
		case 0x44:{
			PORTC &= ~(1<<0);//<================LED
			PORTC &= ~(1<<1);//<================LED
			break;
		}
	}
	
	//order : | 0x55, 0x33, 0x44 | 0x33, 0x44 | 0x44 |
}

int main(void)
{
	/* initialiser le slave I2C */
	I2C_Slave_Init(SLave_Address1);
	/* mettre les deux portes pour LED en sortie */
	DDRC |= (1<<0)|(1<<1);
	/* test de lumière pour confirmer */
	Blink();
	
	/* pour envoyer plusieurs char test */
	char slave_data[] = {0x51,0x31,0x41,0x21};
	
	//data recu du slave
	char data;
	
	//ici true = ack et false = nack
	bool ack = true;
	
	//PORTC |= (1<<0);//<================LED
	while(1){
		//ShortBlink(1);
		switch(I2C_Slave_Listen()){
			case 0:{ //slave receive
				//PORTC |= (1<<1);//<================LED
				//Blink();
				while (1){
					int status = I2C_Slave_Receive(ack);
					
					if (status == 2 || status == 3 || data == 0x22){ //qqch fuck up arret la reception ou ya plus rien a recevoir
						//ShortBlink(1);
						break;
					}
					//PORTC |= (1<<1);//<================LED
					if (status == 0){ //ack envoyer so continue la reception
						data = TWDR; //ici jsp pourquoi on peut pas get la variable or du loop et trop compliqué comprendre commen enregister les data dans une list
						action(data);
						continue;
					}
					if (status == 1){
						data = TWDR; //nack envoyer so arret la reception
						action(data);
						break;
					}
				}
				break;
			}
			case  1:{ //slave write
				//TWCR &= ~ (TWINT);
				//Blink(); //PROBLEME!!!!!!! le slave arret pas de transmettre du bs au master apres le premier char   !!!essay de faire ...start condition . byte . stop condition...start condition . byte . stop condition...
				for (int i=0;i<4;i++){ //PROBLEME!!!!!!!!!!!! le loop fait juste le tour 1 fois et non 3 fois jsp pourquoi comme quand i = 1 il arret
					//ShortBlink(2);
					//PORTC |= (1<<1);//<================LED
					int status = I2C_Slave_Transmmit(slave_data[i]);
					//_delay_ms(100);
					//PORTC |= (1<<1);//<================LED
					if (status == 0 || status == 2  || status == 3 || status == 4){  //ici data transmit + nack received donc arret la transmition or qqch a fuck up
						//ShortBlink(1);
						break; //ici qqch fuck up arret le transmition
					}
					if (status == 1){
						//ShortBlink(2);
						continue; //ici data transmit + ack received
					}
					//Blink();
				}
				TWCR = 0x00; //attention <=============
				//PORTC |= (1<<1);//<================LED
				//IL SORT DU LOOP MAIS IL ENVOIE TOUJOURS DES CARACTERE??? OR LE MASTER EN ENVOYANT DES ACK IL LAISSE PAS LE SLAVE DONNER UN STOP CONDITION
				//	!!!!!!!!!!! LE VRAI PROBLEME == LE PREMIER BYTE QU'IL ENVOIT EST CONSIDÉRÉ COMME LE LAST POUR LE I2C IDK COMMENT????
				//Blink();
				//TWIStop(); <=== useless + problem cuz ----> pas ecrit dans le slave transmit part que le slave fait un stop condition
				//Blink();
				break;
			}
		}
		/* initialiser le slave I2C */
		//I2C_Slave_Init(SLave_Address1);
		//ShortBlink(2);
		//TWCR = 0x00;
		//_delay_ms(10);
		//_delay_ms(1000); //ye quand tu met le delay de 1 sec ca fuck up jsp pourquoi => maybe ca ralenti le slave trop et peut pas acknoledge in time???
		/* initialiser le slave I2C */
		I2C_Slave_Init(SLave_Address1); //<=== tu dois reinitializer les I2c slave cuz ta basicly desactivé avec TWCR = 0x00; en haut qui est nécessaire sinon le slave (not sure) garde le controle du Two Wire interface et le master peut pas faire un stop condition
		data = 0x00;//<================================================ICI MEGA IMPORTANT SINON RIEN FONCTION PLUSIEURS FOIS DE SUITE cuz il garde la valeur 0x22 donc quand il re-embarque dans le loop il a la valeur de 0x22 donc il exit et re enter le loop continuellement
	}
	//Blink();
}


//general call cest quand master addresse tout les slaves donc il envoie FF pour read et FE pour write
//If the TWEA bit is written to zero during a transfer, the TWI will transmit the last byte of the transfer. State 0xC0
//or state 0xC8 will be entered, depending on whether the Master Receiver transmits a NACK or ACK after the
//final byte




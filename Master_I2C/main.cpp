/*
 * Master_I2C.cpp
 *
 * Created: 2023-09-28 13:45:12
 * Author : 2161908
 */ 

#define F_CPU 1000000UL
#include <util/delay.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>

#define SDA 4
#define SCL 5
#define SLave_Address1_Read 0x51
#define SLave_Address1_Write 0x50
#define SLave_Address2_Read 0xA1
#define SLave_Address2_Write 0xA0

char Slave_Address[] = {SLave_Address1_Write, SLave_Address1_Read};
int Mode = 0;	//Mode =0 ---> Write && Mode =1 ---> Read


//https://www.nongnu.org/avr-libc/user-manual/modules.html pour les commands que tu peux faire sur le chip
//check page 225 pour les commands de I2c
//check page 239 pour les registres
//https://embedds.com/programming-avr-i2c-interface/ sur crome (cest meilleure)

char I2C_status()
{
	char TWIstatus = TWSR & 0xF8; //ici il faut du bit masking cuz les 3 dernier bit du registre TWSR non rien a voir avec le status
	//TWSR = 0b 0000 0000
	//mais nous on veux juste TWSR = 0b 0000 0xxx qui va aller dans le TWIstatus
	return TWIstatus;
}

void TWIInit(void){
	//------------------------------------------------------------------------------------------------------------------------
	//set SCL to 400kHz
	//page 239
	//ici TWSR – TWI Status Register de bit 7-3 cest basicly la ou tu peux lire le status du I2c comme ya des code special in case de error or whatever
	//ici TWSR bit 2 est reservé et bit 1-0 est pour le prescaler value ==> pour le clock
	TWSR = 0x00; //ici tu initie le status register a 0 comme ca rien fuckup au debut de la communication
	
	//TWBR selects the division factor for the bit rate generator
	TWBR = 0x0A;
	
	//SCL frequency = F_CPU / (16 + 2(TWBR) *PrescalerValue)
	//------------------------------------------------------------------------------------------------------------------------
	
	//enable I2c
	TWCR = (1<<TWEN)|(1<<TWINT);
}

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


//start conditon = start of communication
// TWCR = Two Wire Control Register basicly pour controler le I2c
// TWINT = interrupt flag = quand le transmition à été complété ==> il le met à 1 mais pour le clear tu dois aussi envoyer un 1 so jsp meme le prof Antone est confused, il est set à  not sure mais cest un flag (so ici il est 1 cuz il expect software application cuz il y rien qui se passe)
// TWISTA = quand tu veux set le MCU a master mode donc celui qui utilise la ligne de SDA ==> tu dois le remettre a zero (softcode) quand tu veux que le slave parle (start condition quand il y a rien sur le SDA line)
// TWEN = permet de prendre controle des fils SDA et SCL je pense???

int TWISlave_Address(uint8_t Slave_Address){
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while (!(TWCR &(1<<TWINT)));
	char status = I2C_status();
	if (status == 0x08 || status == 0x10){
		//----------------------
		//PORTC |= (1<<0);//<================LED
		//PORTC |= (1<<1);//<================LED
		//return 0; //A START condition has been transmitted    OR    A repeated START condition has been transmitted (commencer nouvelle communication back to back)
		//return 0 == qqch happened == bad
		//----------------------
	} else {
		return 0;
	}
	
	TWDR = Slave_Address; //tu rempli le Two Wire Data Register avec le slave address
	TWCR = (1<<TWINT)|(1<<TWEN); // puis tu laisse le chip parler sur la ligne de SDA puis tu clear le frag en le mettant à 1 ye jsp cest weird
	while (!(TWCR &(1<<TWINT))); //roule le while loop jusqu'à tant que la transmission est fini (le fleg est remis à 0 not sure tho)
	status = I2C_status(); //you need to call it again sinon le status change pas
	
	if (status == 0x18){
		//PORTC |= (1<<1);//<================LED
		return 1; //Slave Address + W a été transmit + ack
		//master transmit to slave
		//slave acknoledge master ==> continue byte transfer
	}
	
	
	
	if (status == 0x20){
		//PORTC |= (1<<0);//<================LED
		return 2; //Slave Address + W a été transmit + Not ack
		//master transmit to slave
		//slave acknoledge pas master ==> master va arreter de transmit
	}
	
	
	
	if (status == 0x40){
		//ShortBlink(1);
		return 3; //Slave Address + R a été transmit + ack
		//master receive from slave
		//master acknoledge slave ==> continue byte transfer
	}
	if (status == 0x48){
		//ShortBlink(2);
		return 4; //Slave Address + R a été transmit + Not ack
		//master receive from slave
		//master acknoledge pas slave ==> slave va arreter de transmit
	} 
	if (status == 0x38 || status == 0x50 || status == 0x58 || status == 0x70 || status == 0x78 || status == 0x90 || status == 0x98){
		return 5;//qqch a fuck up IDK
	}
	else return 6;
}

//-------------------------------------------------------------------------------------------------------------
int TWIWrite(uint8_t cData){
	//PORTC |= (1<<1);//<================LED
	TWDR = cData; //tu rempli le Two Wire Data Register
	//TWINT doit etre high pour send un byte et apres tu clear TWINT en le mettant a 1 (ye c'est weird de le mettre a 1 avec un autre 1)
	TWCR = (1<<TWINT)|(1<<TWEN); // puis tu laisse le chip parler sur la ligne de SDA puis tu set le flag
	while (!(TWCR &(1<<TWINT))); //roule le while loop jusqu'à tant que la transmission est fini (le fleg est remis à 0 not sure tho)
	char status = I2C_status();
	if (status == 0x28){
		return 0; // ici le byte has been transmited et ACK a été recu
	}
	if (status == 0x30){
		return 1; // ici le byte has been transmited et ACK a pas été recu
	} 
	else return 2; //retourn si qqch fuck up
}
//-------------------------------------------------------------------------------------------------------------

int I2C_Master_Transmmit(char data){	//faut mettre while et break pour le nack
	//ShortBlink(2);
	_delay_ms(100);
	TWDR = data;//tu rempli le Two Wire Data Register
	//TWINT doit etre high pour send un byte et apres tu clear TWINT en le mettant a 1 (ye c'est weird de le mettre a 1 avec un autre 1)
	TWCR = (1<<TWEN) | (1<<TWINT);// puis tu laisse le chip parler sur la ligne de SDA puis tu set le flag
	//TWCR = (1<<TWEN) | (1<<TWEA) | (1<<TWINT); idk if cette ligne est usefull
	//PORTC |= (1<<0);//<================LED
	while (!(TWCR & (1<<TWINT)));//roule le while loop jusqu'à tant que la transmission est fini (le fleg est remis à 0 not sure tho)
	//PORTC |= (1<<1);//<================LED
	char status = I2C_status();
	if (status == 0x28){
		return 0; // ici le byte has been transmited et ACK a été recu
	}
	if (status == 0x30){
		return 1; // ici le byte has been transmited et ACK a pas été recu
	} else {
		return 2;
	}
}

int I2C_Master_Receive(bool ack){	//faut mettre while et break pour le nack
	//Blink();//<================LED
	if (ack == true){
		TWCR = (1<<TWEN) | (1<<TWINT) | (1<<TWEA);
		//Blink();//<================LED
		} else {
		//PORTC |= (1<<0);//<================LED
		TWCR = (1<<TWEN) | (1<<TWINT);
	}
	while (!(TWCR & (1<<TWINT)));
	char status = I2C_status();
	if (status == 0x50){
		return 0; //ici data received + ack
	}
	if (status == 0x58){
		return 1; //ici data received + nack
	} else return 2;
	
}

//-------------------------------------------------------------------------------------------------------------
//char I2C_Read_Ack(){
//	TWCR=(1<<TWEN)|(1<<TWINT)|(1<<TWEA);	/* Enable TWI, generation of ack and clear interrupt flag */
//	while (!(TWCR & (1<<TWINT)));			/* Wait until TWI finish its current job (read operation) */
//	return TWDR;							/* Return received data */
//}
//
//char I2C_Read_Nack(){
//	TWCR=(1<<TWEN)|(1<<TWINT);				/* Enable TWI and clear interrupt flag */
//	while (!(TWCR & (1<<TWINT)));			/* Wait until TWI finish its current job (read operation) */
//	return TWDR;							/* Return received data */
//}
//-------------------------------------------------------------------------------------------------------------

void TWIStop(void){
	//PORTC |= (1<<1);//<================LED
	//stop condtion = arret la communication
	// TWINT = interrupt flag = quand le transmition complete? not sure mais cest un flag (so ici il est 1 cuz il a finit de transmettre un byte et il attent softcode application)
	// TWSTO = in master mode il va faire un STOP condition
	// TWEN = peremt de controler SDA et SCL line ==> quand il est a 0 la transmission est terminé
	TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
	while(TWCR & (1<<TWSTO));//basicly wait until stop condition
	//PORTC |= (1<<0);//<================LED			IDK why mais en write mode, il veut juste pas faire un stop condition
}
void TWIRepeat(void){
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	while(TWCR & (1<<TWSTA));
}


void action(char data){
	if(data == 0x51){
		PORTC |= (1<<0);//<================LED
	}
	if(data == 0x31){
		PORTC |= (1<<1);//<================LED
	}
	if(data == 0x41){
		PORTC &= ~(1<<0);//<================LED
		PORTC &= ~(1<<1);//<================LED
	}
}

int main(void)
{
	/* initialiser le master I2C */
	TWIInit();
	/* mettre les deux portes pour LED en sortie */
	DDRC |= (1<<0)|(1<<1);
	/* test de lumière pour confirmer */
	Blink();
   
   
	/* pour envoyer plusieurs char test */
	char master_data[] = {0x55,0x33,0x44,0x22,0x11};
	//char master_data[] = {0x55};
   
	//data recu du slave
	char data;
	
	//ici true = ack et false = nack
	bool ack = true;
   
	//int status = TWISlave_Address(SLave_Address1_Read);
   
   while (1)
   {
	   //PORTC |= (1<<1);
	 switch(TWISlave_Address(Slave_Address[Mode])){
		 case 0:{
			 //PORTC |= (1<<0);//<================LED
			 //PORTC |= (1<<1);//<================LED
			 break;  //no start condition
		 }
		 case 1:{
			 //ShortBlink(1);
			 //PORTC |= (1<<0);//<================LED
			 // cest ici que tu vas mettre le loop pour transferer des données if tu recoi le ack a chaque fois
			 for (int i=0;i<4;i++){
				 //PORTC |= (1<<1);//<================LED
				 int status = I2C_Master_Transmmit(master_data[i]);
				 //PORTC |= (1<<1);//<================LED	!!!la fonction arret ici
				 
				 if (status == 0){
					 //ShortBlink(1);
					 continue; //ack a été recu
				 }
				 if (status == 1 || status == 2){
					 //ShortBlink(2);
					 break; //nack a été recu ou qqch a fuck up
					 } else {
					 //PORTC |= (1<<1);//<================LED
					 continue; //il y a d'autre state non utilisé pour l'instant ou something else que ack ou nack est arrivé
				 }
			 }
			 //PORTC |= (1<<1);//<================LED
			 TWIStop();
			 //PORTC |= (1<<1);//<================LED
			 break;
		 }
		 case 2:{
			 //PORTC |= (1<<0);//<================LED
			 I2C_Master_Transmmit(0x11); //only write once cuz recu nack
			 TWIStop();
			 break;
		 }
		 case 3:{
			 //PORTC |= (1<<0);//<================LED
			 while (1){
				 
				 int status = I2C_Master_Receive(ack);
				 //PORTC |= (1<<0);
				 if(TWDR == 0x41){
					 ack=false;
				 }
				 if (TWDR == 0x21){
					 //PORTC |= (1<<0);//<================LED
					 TWIStop(); //cuz cest le master qui send un stop condition au slave
					 break;
				 }
				 if (status == 0){ //data recieved + ack sent
					 //ShortBlink(2);
					 data = TWDR;
					 action(data);
					 continue;
				 }
				 if (status == 1){ //data recieved + nack sent
					 //ShortBlink(2);
					 data = TWDR;
					 action(data);
					 break;
				 }
				 if (status == 2){
					 Blink();
					 break;
				 }
				 //Blink(); //PROBLEME!!!! ici ca blink jusqua l'infini cuz slave arret pas de transmettre du junk au master jsp why
			 }
			 break; //break est necessaire cuz sinon le switch case va juste passer au next case quand il finit avec le current case
		 }
		 case 4:{
			 //PORTC |= (1<<0);//<================LED
			 ack = false;
			 I2C_Master_Receive(ack);
			 data = TWDR;
			 action(data);
			 break;
		 }
		 case 5:{
			 Blink();//ici tout va mal le I2C reconnait aucune operation
		 }
		 case 6:{
			 Blink();
		 }
	 } 
	 //Blink();
	 //TWCR = 0x00;
	 //_delay_ms(10);
	 /* initialiser le master I2C */
	 //TWIInit();
	 ack=true;
	 data = 0x00;//<================================================ICI MEGA IMPORTANT SINON RIEN FONCTION PLUSIEURS FOIS DE SUITE cuz il garde la valeur 0x22 donc quand il re-embarque dans le loop il a la valeur de 0x22 donc il exit et re enter de suite en loop
	 if (Mode == 0){
		 Mode = 1;
		 } else {
		 Mode = 0;
	 }
   }
	
   //ShortBlink(2);
}


//BTW a chaque fois que tu creer un nouveau int dans le loop tu gaspille de la memoire arrange ca a la fin si ta du temps

//ACK ==> quand un parle, l'autre doit acknoledge le byte transmit genre if master transmit, cest le slave qui transmet le ACK bit et vice versa
//NACK (not ack) ==> soit le master veux finir le transmition soit le slave a fini la communication
//general call tu peux parler a tout les slaves?
//btw conseil de antoine le prof => toujours metre les pull up ou pull down à l'exterieur de la chip cuz certain chip comme master, on plus de chose a faire et il pull faire des glitchs sur il active leur pull up en retard et il peut y avoir de glitchs qui peut fuck up la communication
//exemple visuelle du fonctionnement du ACK : master >> | start condition bit | adresse bits A6-A0 | Read/Write bit |
//											: Slave >>																| ACK or NACK bit | 
//ask cest comment le stop condition
//also le ack = quand il est low sur le 9ieme bit et le nack = quand il est high sur le 9ieme bit
//antoine dit que tu peux mieux optimisé le code en faisant le status register function un global variable a la place de le call a chaque fonction





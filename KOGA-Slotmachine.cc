//----------------------------------------------------------------------
// Titel     : KOGA - Slotmachine
//----------------------------------------------------------------------
// Funktion  : Kleine ATMega Slotmachine
// Schaltung : ATMega32
//----------------------------------------------------------------------
// Prozessor : ATMega32
// Takt		 : 16 MHz
// Sprache   : C
// Datum     : 11.01.2018
// Version   : 0.1
// Autor     : Keno Oelrichs Garcia
//----------------------------------------------------------------------
/*
	Changelog:
	V1.0a - 11.01.2018
*/
#define 	F_CPU 16000000	// Taktfrequenz des myAVR-Boards
#include	<avr\io.h>		// AVR Register und Konstantendefinitionen
#include	<avr\functions_keno.h>	//Meine eigene Funktionssammlung einbinden
#include	<stdlib.h>	//Fuer die Random Funktion
//----------------------------------------------------------------------
//Taster Pins und Port definieren
#define BTNPORT DDRA
#define BTN1 PA5
#define BTN2 PA6
#define BTN3 PA7
#define SEK 49910	//Timer Dauer fuer 1 sek.
#define TIMERMAX 65535	//Timer max. Wert

//GAME
const unsigned int DL = 20;	//Spalten
const unsigned int DH = 4;	//Zeilen
const unsigned int SPEED = 10;	//IN MS
const unsigned int GAMECURSORSPEED = 70;	//Delay des Cursors im Game in ms 70
const char ANZCOMBIS = 7;
// A B PFEIL 3 HERZ 1 DOLLAR
char symbols[6] = {0x41,0x42,0x13,0x33,0x93,0xA2};
char chance[6] = {20,20,15,15,15,15};	//Zusammen muss 100 ergeben
char minanz[6] = {3,3,2,3,2,2};
long tick = 0;	//Zeit nehmen

char combis[7][5] = {
	{0,0,0,0,0},
	{1,1,1,1,1},
	{2,2,2,2,2},
	{0,0,1,2,2},
	{2,2,1,0,0},
	{0,1,2,1,0},
	{2,1,0,1,2}
};


//MENUE
const unsigned int MENUECURSORSPEED = 200;	//Delay des Cursors im Menue in ms
uint8_t SHOWEDMENUE = 1;	//Aktuell angewahltes Menue
const unsigned int ANZMENUES = 3;	//Max Anzahl der Menues 

//Fuktionsdefinitionen (damit Funktionschaos moeglich ist :) )
void gameOver();
void displayStartup();
void timerInit();
void gameTimerInit();

//Variablen 
bool gameStarted = false;	//Spiel gestartet
bool gameIsOver = false;	//Spiel ist Verloren
int blinkCounter = 0;	//Speichern von verschiedenen Countern
bool blinkShowMode = false;	//Variable fuer Blinkfunktionen
unsigned int CursorSpeed = MENUECURSORSPEED;	//Geschwindigkeit des Cursors
bool doCheck = false;
bool showWins = false;
char showWinsIndex = 0;
char wins[7][2];

typedef struct{
	float Money;
	float destMoney;
	
	void showMoney(){
		lcdGoto_420(3,11);
		lcdPrintFloat(Money,4,1,0,1);
		lcdWrite(0xA2);
	}
	
	void addMoney(float newMoney){
		Money+= newMoney;
	}
	
	void resetMoney(){
		Money = 0;
		destMoney = 0;
	}
	
	float getMoney(){
		return Money;
	}
	
	void setDestMoney(float newDest){
		destMoney = newDest;
	}
	
	float getDestMoney(){
		return destMoney;
	}

}Player;

Player player;	//Spieler Objekt

typedef struct{
	bool spinning;
	char slot[3][5];
	char speed[5];	//1=Fastest 0=Stop
	bool stopspinning;
	float einsatz;
	float desteinsatz;
	
	//Erste erstellung
	void init(){
		spinning = false;
		stopspinning = true;
		for(char i = 0; i < 5;i++){
			speed[i] = 0;
		}
		for(char z = 0; z < 3;z++){
			for(char s = 0; s < 5;s++){
				create(z,s);
			}
		}
	}
	
	void showEinsatz(){
		lcdGoto_420(4,10);
		lcdPrintFloat(einsatz,2,1,0,1);
		lcdWrite(0xA2);
	}
	
	void spin(){
		if (!spinning && !showWins){
			for(char i = 0; i < 5;i++){
				speed[i] = 1;
			}
			for(char c = 0; c < 7;c++){
				for(char d = 0; d < 2;d++){
					wins[c][d] = 0xFF;
				}
			}
			spinning = true;
			gameTimerInit();
			stopspinning = false;
			lcdWriteAt_420(4,16,"Stop");
		}else if(!stopspinning && !showWins){
			stopspinning = true;
		} else if (stopspinning && !showWins){
			stopActualSpin();
		} else if(showWins){
			
		}else{
			spinning = false;
			doCheck = true;
		}
	}
	
	//Neue erstzeilenberechnung
	void create(char zeile,char spalte){
		char random = rand() % 100;	//Neue Random zahl
		char max = 0;
		for(char s = 0; s < 6;s++){	//Jedes moegliche Zeichen durchschleifen
			max+=chance[s];	//Max Bereich der Warscheinlichkeit
			if (random >= (s == 0 ? 0:chance[s-1]) && random < max){	//Wenn Random im Warscheinlichkeitsbereich
				slot[zeile][spalte] = symbols[s];
				break;
			}
		}
	}
	
	
	
	void show(){
		for(char z = 0; z < 3;z++){
			for(char s = 0; s < 5;s++){
				lcdWriteAt_420(z+1,(1+s*2/*s+1*/),slot[z][s]);
			}
		}
	}
	
	void move(){
		if (spinning){
			for(char i = 0; i < 5;i++){
				if (speed[i] > 10)speed[i] = 0;
				if (speed[4] == 0){
					spinning = false;
					doCheck = true;
					return;
				}
				if (speed[i] == 0)continue;
				if ((tick % speed[i]) == 0){
					for(char z = 2; z > 0;z--){
						slot[z][i] = slot[z - 1][i];
					}
					create(0,i);
					if ((speed[(i != 0 ? i-1:0)] == 0 || i == 0) && stopspinning){
						speed[i]++;
						tick = 0;
					}else if (!stopspinning){
						if (tick >=30)stopspinning = true;
					}
				}
			}
			tick++;
			if (tick > 100)tick = 0;
		}else{
			if(einsatz < desteinsatz){
				einsatz+=0.05;
				showEinsatz();
			}
			if(player.getMoney() < player.getDestMoney()){
				player.addMoney(0.05);
				player.showMoney();
			}
		}
	}
	
	void clearShowCombis(){
		for(char s = 0; s < 5;s++){
			for(char z = 0; z < 3;z++){
				lcdWriteAt_420(z+1,2+s*2," ");
			}
		}
	}
	
	void stopActualSpin(){
		for(char s = 0; s < 5;s++){
			if(speed[s] != 0){
				speed[s] = 0;
				for(char z = 0; z < 3;z++){
					create(z,s);
				}
				break;
			}
		}
	
	}
	
	void showWin(char combi){
		for(char s = 0; s < 5;s++){
			if(s < wins[combi][1]){
				if(blinkShowMode){
					lcdWriteAt_420(combis[combi][s]+1,1+s*2,wins[combi][0]);
				}else{
					lcdWriteAt_420(combis[combi][s]+1,1+s*2," ");
				}
			}
			if (s < 4){
				if (combis[combi][s+1] < combis[combi][s]){
					for(char z = 0; z < 3;z++){
						if (z == combis[combi][s]){
							lcdWriteAt_420(combis[combi][s]+1,2+s*2,0x17);
						}else
							lcdWriteAt_420(z+1,2+s*2," ");
					}
				}else if (combis[combi][s+1] > combis[combi][s]){
					for(char z = 0; z < 3;z++){
						if (z == combis[combi][s]){
							lcdWriteAt_420(combis[combi][s]+1,2+s*2,0x19);
						}else
							lcdWriteAt_420(z+1,2+s*2," ");
					}
				}else{
					for(char z = 0; z < 3;z++){
						if (z == combis[combi][s]){
							lcdWriteAt_420(combis[combi][s]+1,2+s*2,0xdf);
						}else
							lcdWriteAt_420(z+1,2+s*2," ");
					}
				}
			}
		}
		blinkShowMode = !blinkShowMode;
	}

	void check(){
		cli();
		char anzsymbols[7][6];
	
		for(char i = 0; i < 7;i++){
			for(char z = 0; z < 6;z++){
				anzsymbols[i][z] = 0;
			}
		}
		for(char c = 0; c < ANZCOMBIS; c++){	//jede Combi durchschleifen
			bool found = false;
			for(char sy = 0; sy < 6;sy++){	//Jedes Symbol durchschleifen
				if (found)continue;
				for(char s = 0; s < 5;s++){	//Jede Spalte durchschleifen
					if(slot[combis[c][s]][s] == symbols[sy] && anzsymbols[c][sy] == s){
						anzsymbols[c][sy]++;
						found = true;
					}else continue;
				}
			}
		}
		for(char c = 0; c < 7;c++){
			wins[c][0] = 10;
			wins[c][1] = 10;
			for(char i = 0; i < 6;i++){
				if (anzsymbols[c][i] >= minanz[i]){
					wins[c][0] = symbols[i];
					wins[c][1] = anzsymbols[c][i];
				}
			}

		}
		doCheck = false;
		showWins = true;
		timerInit();
		sei();
	}

	bool isSpinning(){
		return spinning;
	}

}Slotmachine;

Slotmachine machine;

//Spiel vorbei
void gameOver(){
	gameIsOver = true;
	timerInit();	//Timer auf normale geschwindigkeit zuruecksetzen
	lcdWriteAt_420(2,6,"!GAME OVER!");	//Nachricht ausgben
	CursorSpeed = MENUECURSORSPEED;	//Cursorgeschwindigkeit aendern
}
//Normalen Timer initialisieren
void timerInit(){
	//Timer starten
	timer1Init(1024,TIMERMAX - (TIMERMAX - SEK) / (1000 / 500));
}
//Game Timer initialisieren
void gameTimerInit(){
	//Timer starten
	timer1Init(1024,TIMERMAX - (TIMERMAX - SEK) / (1000 / SPEED));
}
//Menueauswahlsymbole aktivieren/deaktivieren
void menueSet(bool active){
	if (active){
		lcdWriteAt_420(SHOWEDMENUE + 1,2,">");
		lcdWriteAt_420(SHOWEDMENUE + 1,19,"<");
	}else{
		lcdWriteAt_420(SHOWEDMENUE + 1,2," ");
		lcdWriteAt_420(SHOWEDMENUE + 1,19," ");
	}
}
//Wechsel der Menueauswahlsymbole durchfuehren
void toggleBedienung(){
	menueSet(blinkShowMode);
	blinkShowMode = !blinkShowMode;
}

//Spiel mit initialisierung starten
void startGame(){
	if (!gameStarted){	//nur initialisieren wenn Spiel noch nicht gestartet
		machine.init();
		lcdClear();
		machine.show();
		player.resetMoney();
		gameStarted = true;
		lcdWriteAt_420(4,16,"Dreh");
		lcdWriteAt_420(4,1,"Einsatz:");
		lcdWriteAt_420(2,12,"Geld:");
		machine.showEinsatz();
		player.showMoney();
		gameTimerInit();
	}
}

//Interrupt bei Timer Overflow
ISR(TIMER1_OVF_vect){
	cli();
	if(gameStarted){
		machine.move();
		machine.show();
		if (doCheck)machine.check();
		if (showWins){
			
			while(wins[showWinsIndex][0] == 10 && showWinsIndex < 7){
				showWinsIndex++;
				blinkCounter = 0;
				blinkShowMode = false;
			}
			
			if (showWinsIndex < 7 && wins[showWinsIndex][0] != 10){
				machine.showWin(showWinsIndex);
				blinkCounter++;
			}
			if (blinkCounter > 4 || showWinsIndex > 6){
				blinkCounter = 0;
				showWinsIndex++;
				if (showWinsIndex > 6){
					showWins = false;
					showWinsIndex = 0;
					blinkCounter = 0;
					machine.clearShowCombis();
					lcdWriteAt_420(4,16,"Dreh");
				}
			}
		
		}
	}else{
		toggleBedienung();	//Blinken der Menueelement auswahl
	}
	timer1_ResetValue();	//Timer Zahlregister zuruecksetzen
	sei();	//Interrupts erlauben
}

//Wenn Taster 1 gedrueckt wurde
void btn1_pressed(){
	if (!gameStarted){	//Wenn Spiel noch nicht gestartet
		menueSet(false);	//Menuesymbole an aktiver Position loeschen
		if (SHOWEDMENUE > 1)SHOWEDMENUE--;	//Menuesymbol verschieben
		menueSet(true);	//menuesymbol anzeigen
	}else if(!gameIsOver && !machine.isSpinning()){	//Wenn das Spiel angefangen und noch nicht verloren
	}
}

//Wenn Taster 2 gedrueckt wurde
void btn2_pressed(){
	if (!gameStarted){	//Wenn Spiel noch nicht gestartet
		menueSet(false);	//Menuesymbole an aktiver Position loeschen
		if (SHOWEDMENUE < ANZMENUES)SHOWEDMENUE++;	//Menuesymbol verschieben
		menueSet(true);	//menuesymbol anzeigen
	}else if(!gameIsOver && !machine.isSpinning()){	//Wenn das Spiel angefangen und noch nicht verloren
	}
}

//Wenn Taster 3 gedrueckt wurde
void btn3_pressed(){
	if (!gameStarted){	//Wenn Spiel noch nicht gestartet
		switch(SHOWEDMENUE){	//Aktuelles Menue ermitteln
			case 1:{	//Spiel starten
				startGame();
				break;
			}
			case 2:{	//Einstellungen
			
				break;
			}
			case 3:{	//Hilfe
			
				break;
			}
			default:{break;}
		}
	}else if (gameIsOver){	//Wenn Spiel gestartet aber Verloren
		gameStarted = false;
		gameIsOver = false;
		displayStartup();	//Haumptmenue anzeigen
	}else{
		machine.spin();
	}
}


//Startnachricht ausgeben
void displayStartup(){
	//Startinfo
	//Informationen auf dem Display ausgeben
	cli();
	lcdClear();
	lcdWriteAt_420(1,1,"=SLOTMACHINE V0.1.0=");
	lcdWriteAt_420(2,1,"   Spiel starten    ");
	lcdWriteAt_420(3,1,"   Einstellungen    ");
	lcdWriteAt_420(4,1,"       Hilfe        ");
	sei();
}
//Bedienung anzeigen
void showBedienung(){
	lcdClear();
	lcdWriteAt_420(1,1,"     BEDIENUNG      ");
	lcdWriteAt_420(2,1,"   Tastenbelegung   ");
	lcdWriteAt_420(3,1,"   des Einschubs    ");
	lcdWriteAt_420(4,1," HOCH  RUNTER   OK  ");
	_delay_ms(3000);
}

//Grundlegende Initialisierungen vornehmen
void init(){
	
	//Eingaenge definieren
	pinMode(BTNPORT, BTN1, INPUT);
	pinMode(BTNPORT, BTN2, INPUT);
	pinMode(BTNPORT, BTN3, INPUT);
	//Pullups setzen
	PORTA |= (1<<BTN1 | 1<<BTN2 | 1<<BTN3);
	
	
	//LCD Initialisieren und Loeschen
	lcdInit();
	lcdClear();
	
	//Cursor ausblenden und Startbildschirm anzeigen, Timer Initialisieren
	lcdDisableCursor();
	showBedienung();
	displayStartup();
	timerInit();
}

main(){

	init();
	sei();
	while(true){
		//Taster abfragen (Ohne Interrupt da dadurch fehler passiert sind)
		if (!digitalRead(PINA,BTN1)){btn1_pressed();_delay_ms(CursorSpeed);}
		if (!digitalRead(PINA,BTN2)){btn2_pressed();_delay_ms(CursorSpeed);}
		if (!digitalRead(PINA,BTN3)){btn3_pressed();_delay_ms(CursorSpeed);}
	}

}

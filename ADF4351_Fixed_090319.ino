//   ADF4351 with fixed frequency
//   By Alain Fort F1CJN march 9,2019
//  
//
//
//  ****************************************************** FRANCAIS *******************************************************
//  Ce programme permet de programmer un ADF 4351 à une fréquence fixe et en utilisant une fréquence de reférence de 10 MHz.
//  La fréquence de sortie peut être modifiée à la ligne 80 en conservant le format.
//  La frequence de reference peut être modidiée à la ligne ligne 70  (10MHz par défaut)
//  ********************************************* HARDWARE IMPORTANT *******************************************************
//  Avec un Arduino UN0 : utilise un pont de résistances pour réduire la tension, MOSI (pin 11) vers
//  ADF DATA, SCK (pin13) vers CLK ADF, Select (PIN 3) vers LE
//  Resistances de 560 Ohm avec 1000 Ohm à la masse sur les pins 11, 13 et 3 de l'Arduino UNO pour
//  que les signaux envoyés DATA, CLK et LE vers l'ADF4351 ne depassent pas 3,3 Volt.
//  Pin 2 de l'Arduino (pour la detection de lock) connectee directement à la sortie MUXOUT de la carte ADF4351
//  La carte ADF est alimentée en 5V par la carte Arduino (les pins +5V et GND sont proches de la LED Arduino).
//  ***********************************************************************************************************************
//  
//
//  *************************************************** ENGLISH ***********************************************************
//  This software is used to programm an ADF4351 with a fixed frequency, using a 10 MHz reference frequency.
//  The frequency can be changed at line 80, using the same format.
//  The reference frquency can be changed at line 70, using the same format (Default 10 MHz)
//  ******************************************** HARDWARE IMPORTANT********************************************************
//  With an Arduino UN0 : uses a resistive divider to reduce the voltage, MOSI (pin 11) to
//  ADF DATA, SCK (pin13) to ADF CLK, Select (PIN 3) to ADF LE
//  Resistive divider 560 Ohm with 1000 Ohm to ground on Arduino pins 11, 13 et 3 to adapt from 5V
//  to 3.3V the digital signals DATA, CLK and LE send by the Arduino.
//  Arduino pin 2 (for lock detection) directly connected to ADF4351 card MUXOUT.
//  The ADF card is 5V powered by the ARDUINO (PINs +5V and GND are closed to the Arduino LED).

#include <SPI.h>
#define ADF4351_LE 3

uint32_t registers[6] =  {0x4580A8, 0x80080C9, 0x4E42, 0x4B3, 0xBC803C, 0x580005} ; // 437 MHz avec ref à 25 MHz
//uint32_t registers[6] =  {0, 0, 0, 0, 0xBC803C, 0x580005} ; // 437 MHz avec ref à 25 MHz
int address,modif=0;
unsigned int i = 0;
double RFout, REFin, INT, PFDRFout, OutputChannelSpacing, FRACF;
double RFoutMin = 35, RFoutMax = 4400, REFinMax = 250, PDFMax = 32;
unsigned int long RFint,RFintold,INTA,RFcalc,PDRFout, MOD, FRAC;
byte OutputDivider;byte lock=2;
unsigned int long reg0, reg1;

void WriteRegister32(const uint32_t value)   //Programme un registre 32bits
{
  digitalWrite(ADF4351_LE, LOW);
  for (int i = 3; i >= 0; i--)          // boucle sur 4 x 8bits
  SPI.transfer((value >> 8 * i) & 0xFF); // décalage, masquage de l'octet et envoi via SPI
  digitalWrite(ADF4351_LE, HIGH);
  digitalWrite(ADF4351_LE, LOW);
}

void SetADF4351()  // Programme tous les registres de l'ADF4351
{ for (int i = 5; i >= 0; i--)  // programmation ADF4351 en commencant par R5
    WriteRegister32(registers[i]);
}

//************************************ Setup ****************************************
  void setup() {
  Serial.begin (19200); //  Serial to the PC via Arduino "Serial Monitor"  at 9600

  pinMode(2, INPUT);  // PIN 2 en entree pour lock
  pinMode(ADF4351_LE, OUTPUT);          // Setup pins
  digitalWrite(ADF4351_LE, HIGH);
  SPI.begin();                          // Init SPI bus
  SPI.setDataMode(SPI_MODE0);           // CPHA = 0 et Clock positive
  SPI.setBitOrder(MSBFIRST);            // poids forts en tête

  PFDRFout=10; // Frequence de reference
  RFintold=1234;//pour que RFintold soit different de RFout lors de l'init
  RFout = RFint/100 ; // fréquence de sortie
  OutputChannelSpacing = 0.01; // Pas de fréquence
 
} // Fin setup

void loop()
{
 //********************************************** 
  RFout=1888.000; // Fréquence de sortie // output frequency
 //********************************************
  if ((RFint != RFintold)|| (modif==1)) {
    if (RFout >= 2200) {
      OutputDivider = 1;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout < 2200) {
      OutputDivider = 2;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout < 1100) {
      OutputDivider = 4;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout < 550)  {
      OutputDivider = 8;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout < 275)  {
      OutputDivider = 16;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout < 137.5) {
      OutputDivider = 32;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout < 68.75) {
      OutputDivider = 64;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 0);
    }

    INTA = (RFout * OutputDivider) / PFDRFout;
    MOD = (PFDRFout / OutputChannelSpacing);
    FRACF = (((RFout * OutputDivider) / PFDRFout) - INTA) * MOD;
    FRAC = round(FRACF); // On arrondit le résultat

    registers[0] = 0;
    registers[0] = INTA << 15; // OK
    FRAC = FRAC << 3;
    registers[0] = registers[0] + FRAC;

    registers[1] = 0;
    registers[1] = MOD << 3;
    registers[1] = registers[1] + 1 ; // ajout de l'adresse "001"
    bitSet (registers[1], 27); // Prescaler sur 8/9

    bitSet (registers[2], 28); // Digital lock == "110" sur b28 b27 b26
    bitSet (registers[2], 27); // digital lock 
    bitClear (registers[2], 26); // digital lock
   
    SetADF4351();  // Programme tous les registres de l'ADF4351
    RFintold=RFint;modif=0;
 
  }
}   // fin loop


//-------------VirtuinoCM  Library and settings --------------
#include "VirtuinoCM.h"
VirtuinoCM virtuino;               
#define V_memory_count 250          // Esta es la cantidad de memorias "V" del virtuino, debe ser <=255)
float V[V_memory_count];           // Este array se syncroniza con la posicion de memorias "V" del virtuino
boolean debug = true;              // set this variable to false on the finale code to decrease the request time.
//-------------------------------------------------------------

//---------- VARIABLES (virtuales) HACIA VIRTUINO ------------
float V0_Punto_cero=0;     
float V1_Giro180=0;
float V2_Giro0=0;
float V3_Pos_ini_ok=0;
float V4_Piston_arriba=0;
float V5_Piston_abajo=0;
float V6_Led_marcha=0;
float V7_Vacio=0;
//-------------------------------------------------------------

//------------ VARIABLES (virtuales) HACIA MEGA--- ------------
float V11_Marcha=0;     
float V12_velocidad_Motor=0;
float V13_setpoint=0;
float V14_Pos_intermedia=0;
float V15_Vacio=0;
float V19_Vacio_off=0;
float V16_Tiempo1_soplo=3;
float V17_Tiempo2_sube=1;
float V18_Tiempo3_bajar=0;
//-------------------------------------------------------------

//----------- VARIABLES PARA EL ENCODER Y DEMAS ---------------
const int dirPin = 8; //Definimos el Pin 8 como salida de direccion
const int stepPin = 9; //Definimos el Pin 9 como salida de pasos
const int soplo=10;
const int cero_encoder =  2; //Definimos el Pin 2 como sensor punto cero
const int piston_arriba = 4;
const int piston_abajo = 3;
const int step1 = 25;//200; // Definimos el numero de pasos totales
const int step2 = 100;
const int step3 = 100;//75;
const int Emergencia = 11;
const int borra=10;

int stepDelay = 5000; //es una variable de tiempo entre pasos para definir la velocidad
int cero = 1; //Para salir del bucle de busqueda del punto cero
int val=0; //Almacenamos el valor del sensor en la busqueda de cero
int inicio=0;; //Almacenamos el valor cuando pulsamos "Marcha"
int giro=2;//Almacanamos el valor para saber que hemos girado 
int arriba=0; //Señal del micro piston arriba
int abajo;
int vacio=0;
int stop=0;
int salta=0;
int otra;
int pulso_cero=0; //Variable para establecer la pulsacion de busqueda del punto cero
int Busca_cero=0;
int second;
int barrera=12;
int step[10];
//-------------------------------------------------------------



/*****************************************************************/
/*************************** SETUP *******************************/
/*****************************************************************/
void setup() {

     //----------------------- SETUP COMUNICACION VIRTUINO ----------------
     if (debug) {
        Serial.begin(9600);
        while (!Serial) continue;
        }
     Serial1.begin(9600);  
     Serial1.setTimeout(50);
     virtuino.begin(onReceived,onRequested,256);  //Arrancamos el Virtuino
     //virtuino.key="1234";Este es el password del virtuino cuando se sincroniza el bluethooz
     //---------------------------------------------------------------------

     //-------------- DEFINIMOS ENTRADAS Y SALIDAS ARUDINO MEGA ------------
     pinMode(2, INPUT);//Entrada sensor Punto Cero
     pinMode(piston_arriba, INPUT_PULLUP);//Entrada sensor Piston Arriba PIN 3
     pinMode(piston_abajo, INPUT_PULLUP);//Entrada sensor Piston Abajo PIN 4
     pinMode(5, OUTPUT);//Electrovalvula Piston Baja
     pinMode(6, OUTPUT);//Electrovalvula Piston Sube
     pinMode(7, OUTPUT);//Rele Bomba Vacio
     pinMode(soplo, OUTPUT);//Electrovalvula de soplado
     pinMode(11, OUTPUT); //Activa el pin reset de la placa
     pinMode(12, INPUT_PULLUP); //Barrera de seguridad



     //---------------------------------------------------------------------

    //---------- DEFINIMOS PINES DE DIRECCION Y PASOS DEL MOTOR -----------
	  pinMode(dirPin, OUTPUT); //Pin de direccion
	  pinMode(stepPin, OUTPUT);//Pin de pasos 
    //---------------------------------------------------------------------

    //------------------ POSICIONAMOS PISTON Y SOPLADO --------------------
    digitalWrite(5, LOW); //Desactivamos la electrovalvula Piston_subir 
    delay(300);
    digitalWrite(6, HIGH); //Activamos la electrovalvula Piston_bajar   
    delay(300);
    digitalWrite(soplo, LOW); // Desactivamos la electrovalvula de soplado
    delay(300);
    digitalWrite(11,HIGH);
    step[0]=1;
    V[20]=0;
    V[44]=0;
    vDelay(1000);
    //---------------------------------------------------------------------

   //---------------------------------------------------------------------
   //digitalWrite(11, HIGH);//Activamos la salida para que no este a cero
   //---- Cargamos los tiempos en Virtuino ---
   V[16]=V16_Tiempo1_soplo;
   V[17]=V17_Tiempo2_sube;
   V[18]=V18_Tiempo3_bajar;
   V[9]=10000;
   vDelay(2000);
   //---------------------------------------------------------------------

  }



/*****************************************************************/
/*********************** PROGRAMA PRINCIPAL***********************/
/*****************************************************************/

void loop() {
     virtuinoRun();        // Arrancamos Virtuino 


     //------------------------------------------------------------------------------      
     //----- BUSCAMOS LA POSICION CERO CUANDO ARRANCAMOS POR PRIMERA VEZ (STEP_0) -----

     while(step[0]==1)
       {    

           V[20]=1; //Led indicador Step 0                  
          arriba=digitalRead(piston_arriba);
          while(pulso_cero==0){
               arriba=digitalRead(piston_arriba);
               if(arriba==0){
                    Busca_cero=V[13]; //Leemos el valor que biene del pulsador de set point de Virtuino   
                    vDelay(500);
                 }   
               if(Busca_cero==1){
                 pulso_cero=1;
                 } 
              }
              if(pulso_cero==1)
              { 
                digitalWrite(dirPin, HIGH); //Activar Dirección del motor HIGH/LOW
	              val=digitalRead(cero_encoder);
                //---- Giramos hasta encontrar el punto cero----
	              if(val == HIGH) {
		              digitalWrite(stepPin, HIGH);
		              delayMicroseconds(12000);//velocidad minima del motor para buscar el punto cero
		              digitalWrite(stepPin, LOW);
		              delayMicroseconds(12000);        
                }
                else
                {
                  V0_Punto_cero=1; 
                  V[0]=V0_Punto_cero; //Le decimos a Virtuino que tenemos el punto cero
                  step[0]=0;
                  step[1]=1;
                  pulso_cero=0; 
                }
              }         
        }


  //-----------------------------------------------------------------------------------
  //------------------ PONEMOS LA BANDEJA EN POSICION INCLIDADA (STEP 1)---------------
  
  while(step[1]==1)
       {
         V[20]=0; 
         V[21]=1; 
         vDelay(200);

         digitalWrite(dirPin, LOW); 
         V12_velocidad_Motor=8000;//V[12];
         // 200 pulsos para hacer una vuelta completa // Posicion intermedia 25 pasos 
         V14_Pos_intermedia=25;
	       for (int x = 0; x < V14_Pos_intermedia; x++){
              // Paro();//devolvemos un valor de x max y un valor para la variable salta----------------------------------------
		          digitalWrite(stepPin, HIGH);
		          delayMicroseconds(V12_velocidad_Motor);
		          digitalWrite(stepPin, LOW);
		          delayMicroseconds(V12_velocidad_Motor);	          
           }
           if(salta==0){
             step[1]=0;
             step[2]=1; 
             inicio=0;
             vacio=0;
           }
      }
  

  //----------------------------------------------------------------------------------
  //---------------- ESPERAMOS SEÑAL "MARCHA" PARA INICIAR CICLO (STEP 2) ------------

  while(step[2]==1)
       {
         V[21]=0; //Señles de estado para virtuino (steps)
         V[22]=1;
         V[24]=0;
         vDelay(200);
         ////////////////////////
         V[11]=0;
         V[15]=0;
         inicio=0;
         vacio=0;
         second=0;
         V[30]=second;
         /////////////          }    

         //arriba = digitalRead(piston_arriba);
         while(inicio==0)
              {            
                V15_Vacio=V[15];
                vDelay(100);
                if(V15_Vacio==1)  
                  {
                    digitalWrite(7, HIGH);
                    V7_Vacio=1;                 
                    vDelay(100);
                    V[7]=V7_Vacio; //Le decimos al Virtuino que hemos activado vacio
                    vacio=1;//Creo que tiene que ser cero +++++++++++++++++++++++++++++++++++++++++++++++++
                  }
                //  V11_Marcha=V[11]; 
                //  vDelay(100); 

                  /////////////////////////////////////////////////////////
                  //PROBAMOS A PONER UN BOTON DE PARO VACIO
                  V19_Vacio_off=V[19];
                  vDelay(100);
                  if(V19_Vacio_off==1)  
                    {
                      digitalWrite(7, LOW);
                      V7_Vacio=0;                                       
                      vDelay(100);
                      V[7]=V7_Vacio; //Le decimos al Virtuino que hemos desactivado el vacio
                      vacio=0;//Creo que tiene que ser cero +++++++++++++++++++++++++++++++++++++++++++++++++
                    }
                    V11_Marcha=V[11]; 
                    vDelay(100); 
                  ///////////////////////////////////////////////////////

                  if((V11_Marcha==1) && (vacio==1))
                    {
                      Emer();


                      V6_Led_marcha = 1;
                      V[6]=1;
                      while(otra==0)
                           { 
                             digitalWrite(dirPin, HIGH); //Activar Dirección del motor HIGH/LOW
	                           val=digitalRead(cero_encoder);
                             //---- Giramos hasta encontrar el punto cero----
	                           if(val == HIGH) 
                               {
		                             digitalWrite(stepPin, HIGH);
		                             delayMicroseconds(8000);
		                             digitalWrite(stepPin, LOW);
		                             delayMicroseconds(8000);        
                               }
                               else
                               {otra=1;}
                           }
                          inicio=1;
                     }  
               }
         if(salta==0){
             digitalWrite(dirPin, LOW);
          	 for (int x = 0; x < step3; x++) {
	        	     digitalWrite(stepPin, HIGH);
		             delayMicroseconds(V12_velocidad_Motor);
		             digitalWrite(stepPin, LOW);
		             delayMicroseconds(V12_velocidad_Motor);

                 Emer();//++++ PARO POR BARRERA SEGURIDAD ++++
 
      	        }
             //-----------------------------------------
             V18_Tiempo3_bajar=V[18];
             V18_Tiempo3_bajar=(V18_Tiempo3_bajar*1000);
             vDelay(V18_Tiempo3_bajar);
             //-----------------------------------------
             digitalWrite(6, LOW); //Desactivamos la electrovalvula Piston_subir 
             vDelay(100);
             digitalWrite(5, HIGH); //Activamos la electrovalvula Piston_bajar   
             step[2]=0;
             step[3]=1;
       
          } 
      }
      Emer();//++++ PARO POR BARRERA SEGURIDAD ++++

 
  
  //-----------------------------------------------------------------------------------
  //--------------- CUANDO EL PISTON LLEGA ABAJO ESPERA 1s Y SUBE STEP (3) ------------

  while(step[3]==1)
       {  
         V[22]=0;
         V[23]=1;
         vDelay(1000);
         abajo = digitalRead(piston_abajo);

         Emer();//++++ PARO POR BARRERA SEGURIDAD ++++

         if(abajo==0){  

            Emer();//++++ PARO POR BARRERA SEGURIDAD ++++

            digitalWrite(7,LOW);//Paramos vacio
            vacio=0;
            V7_Vacio=0;
            vDelay(200);
            V[7]=0;//V7_Vacio;
            digitalWrite(10, HIGH);//Activamos soplo            
            V17_Tiempo2_sube=V[17]; //Tiempo antes de subir
            V17_Tiempo2_sube=(V17_Tiempo2_sube*1000);
            vDelay(V17_Tiempo2_sube);

            Emer();//++++ PARO POR BARRERA SEGURIDAD ++++
           
            if(salta==0){
              digitalWrite(5, LOW); //Desactivamos la electrovalvula Piston_bajar 
              vDelay(1000);
              digitalWrite(6, HIGH); //Activamos la electrovalvula Piston_subir  
              digitalWrite(10, HIGH);//Activamos soplo 

              Emer();//++++ PARO POR BARRERA SEGURIDAD ++++


              //********************  Tiempo de soplado mientras sube ******************
              V16_Tiempo1_soplo=V[16];
              V16_Tiempo1_soplo=(V16_Tiempo1_soplo*1000);
              vDelay(V16_Tiempo1_soplo);
              digitalWrite(10,LOW);//Paramos Soplo
              //************************************************************************

              Emer();//++++ PARO POR BARRERA SEGURIDAD ++++
  
              step[3]=0;
              step[4]=1;
            }  
          }                    
       }     
       Emer();//++++ PARO POR BARRERA SEGURIDAD ++++


  //--------------------------------------------------------------------------------
  //------ CUANDO EL PISTON LLEGA ARRIBA GIRA 180º Y BAJA CUANDO LE DIGAMOS --------
 
  while(step[4]==1)
       {
         V[23]=0;
         V[24]=1;
         vDelay(1000);

         digitalWrite(dirPin, LOW);
         arriba = digitalRead(piston_arriba);

         Emer();//++++ PARO POR BARRERA SEGURIDAD ++++

        second=0;

        while(second==0){
             second = V[30];  //""""""""""""""""""""""""""""""""""""""""""""
             vDelay(200);     //""""""""""""""""""""""""""""""""""""""""""""
        } 
         if((arriba==0)&&(salta==0))
           { 
  	         for (int x = 0; x < step2; x++) 
                 {
                 Emer();//++++ PARO POR BARRERA SEGURIDAD ++++

                    if(salta==0){
	    	              digitalWrite(stepPin, HIGH);
		                  delayMicroseconds(V12_velocidad_Motor);
		                  digitalWrite(stepPin, LOW);
		                  delayMicroseconds(V12_velocidad_Motor);
                      }
  	              }   
  

             //---------------------------------------------- 
             V18_Tiempo3_bajar=V[18]; //Tiempo antes de subir
             V18_Tiempo3_bajar=(V18_Tiempo3_bajar*1000);
             vDelay(V18_Tiempo3_bajar);
             //----------------------------------------------

             Emer();//++++ PARO POR BARRERA SEGURIDAD ++++

             
             if(salta==0){
                 digitalWrite(6, LOW); //Desactivamos la electrovalvula Piston_subir 
                 delay(200);
                 digitalWrite(5, HIGH); //Activamos la electrovalvula Piston_bajar   
                 second=0;
                 step[4]=0;
                 step[5]=1;  
                }
            }
         
        } 

 //-----------------------------------------------------------------------------------
  //------------------ CUANDO EL PISTON LLEGA ABAJO ESPERA Y SUBE ---------------------

  while(step[5]==1)
       {  
        V[24]=0;
        V[25]=1;
        vDelay(1000);
        abajo = digitalRead(piston_abajo);

        Emer();//++++ PARO POR BARRERA SEGURIDAD ++++

        if((abajo==0)&&(salta==0)){
        //-----------------------------------------------
        V17_Tiempo2_sube=V[17]; //Tiempo antes de subir
        V17_Tiempo2_sube=(V17_Tiempo2_sube*1000);
        vDelay(V17_Tiempo2_sube);
        //-----------------------------------------------

        Emer();//++++ PARO POR BARRERA SEGURIDAD ++++
      
        if(salta==0){
          digitalWrite(5, LOW); //Desactivamos la electrovalvula Piston_subir 
          vDelay(300);
          digitalWrite(6, HIGH); //Activamos la electrovalvula Piston_bajar   
          vDelay(300);
          step[5]=0;
          step[6]=1;  
          }
        }  

      }
 
  //-----------------------------------------------------------------------------------
  //--------------- CUANDO EL PISTON LLEGA ARRIBA POR SEGUNDA VEZ  --------------------
  //------------- INCLINAMOS LA BANDEJA PARA VOLVER A EMPEZAR (STEP 4) ----------------

  while(step[6]==1)
      {
        V[25]=0;
        V[26]=1;
        vDelay(200);

        Emer();//++++ PARO POR BARRERA SEGURIDAD ++++

        arriba = digitalRead(piston_arriba);
        if((arriba==0)&&(salta==0))
          {
	         digitalWrite(dirPin, LOW);
      	   for (int x = 0; x < 25; x++)
               {
                 digitalWrite(stepPin, HIGH);
		             delayMicroseconds(V12_velocidad_Motor);
		             digitalWrite(stepPin, LOW);
		             delayMicroseconds(V12_velocidad_Motor);

  
               }  
               if(salta==0)
                  {
                   // Emer();//++++ PARO POR BARRERA SEGURIDAD ++++

                    V6_Led_marcha=0; 
                    V[4]=V6_Led_marcha;  
                    step[4]=0;    
                    for (int x = 0; x < borra; x++)
                        {
                          step[x]=0;
                        }
                    step[2]=1;
                    V[11]=0;
                    V[15]=0;
                    inicio=0;
                    vacio=0;
                    otra=0;
                  }    
                  Emer();//++++ PARO POR BARRERA SEGURIDAD ++++
           }     
        }

        Emer();//++++ PARO POR BARRERA SEGURIDAD ++++

}

//++++ PARO POR BARRERA SEGURIDAD ++++

void Emer(){

    stop=digitalRead(12); 
    if(stop==1);{
    digitalWrite(11, HIGH);}
    //vDelay(1000);}
    if(stop==0){
      digitalWrite(11, LOW);
      //vDelay(1000);
      } 
}   
//++++++++++++++++++++++++++++++++++++ 



//============================================================== onCommandReceived
//==============================================================
/* This function is called every time Virtuino app sends a request to server to change a Pin value
 * The 'variableType' can be a character like V, T, O  V=Virtual pin  T=Text Pin    O=PWM Pin 
 * The 'variableIndex' is the pin number index of Virtuino app
 * The 'valueAsText' is the value that has sent from the app   */

 
 void onReceived(char variableType, uint8_t variableIndex, String valueAsText){     
    if (variableType=='V'){
        float value = valueAsText.toFloat();        // convert the value to float. The valueAsText have to be numerical
        if (variableIndex<V_memory_count) V[variableIndex]=value;              // copy the received value to arduino V memory array
    }
}

//==============================================================
/* This function is called every time Virtuino app requests to read a pin value*/
String onRequested(char variableType, uint8_t variableIndex){     
    if (variableType=='V') {
    if (variableIndex<V_memory_count) return  String(V[variableIndex]);   // return the value of the arduino V memory array
  }
  return "";
}

//============================================================== virtuinoRun
  void virtuinoRun(){
    while (Serial1.available()) {
        char tempChar=Serial1.read();
        if (tempChar==CM_START_CHAR) {               // a new command is starting...
              virtuino.readBuffer=CM_START_CHAR;     // copy the new command to the virtuino readBuffer
              virtuino.readBuffer+=Serial1.readStringUntil(CM_END_CHAR);
              virtuino.readBuffer+=CM_END_CHAR;
              if (debug) Serial.println("\nCommand= "+virtuino.readBuffer);
              String* response= virtuino.getResponse();    // get the text that has to be sent to Virtuino as reply. The library will check the inptuBuffer and it will create the response text
              if (debug) Serial.println("Response : "+*response);
              Serial1.print(*response);
              break; 
         }
    }
 }
 

 //============================================================== vDelay
  void vDelay(int delayInMillis){long t=millis()+delayInMillis;while (millis()<t) virtuinoRun();}




#define DEBUG false

#define SWITCH_TYPE 0
#define PWM_TYPE 1
#define SENSOR_TEMPERATURA_TYPE 2
#define PULSADOR_TYPE 3

#define SLOTS_QTY 8
#define ADDRESS 10

#include <SPI.h>
#include <Ethernet.h>
//#include <Bounce.h>
#include <Bounce2.h>
#include <EEPROMEx.h>
#include <dht11.h>

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[] = {
  192,168,1,5};
#if defined(ARDUINO) && ARDUINO >= 100
EthernetServer server(80);
#else
Server server(80);
#endif

//int address = 10;
int cantidadSwitchs = 0;
int i = 0; //utilizada para los contadores
typedef struct
{
  int input;
  int output;
  int type;
  int state;
  //char name[2];
}  
input_type;

input_type config[SLOTS_QTY];

Bounce *bouncer;

dht11 DHT11;

void loadConfig (){
  EEPROM.readBlock(ADDRESS, config);
#if DEBUG
  Serial.println("Ep read"); 
  for ( i = 0; i < SLOTS_QTY; i++) {
    //Serial.print("n: ");  
    //Serial.println(config[i].name);   
    Serial.print("i: ");  
    Serial.println(config[i].input); 
    Serial.print("o: ");  
    Serial.println(config[i].output);
    Serial.print("t: ");  
    Serial.println(config[i].type);
    Serial.print("s: ");  
    Serial.println(config[i].state); 
    Serial.println("-");   
  }
#endif

}

void createConfig () {
  config[0].input = A0;
  config[0].output = 0;
  config[0].type = SWITCH_TYPE;
  config[0].state = 0;
  //String nombre = "L1";
  //nombre.toCharArray(config[0].name,2);

  config[1].input = A1;
  config[1].output = 1;
  config[1].type = SWITCH_TYPE;
  config[1].state = 0;
  //nombre = "L2";
  //nombre.toCharArray(config[1].name,2);
  
  config[2].input = A2;
  config[2].output = 2;
  config[2].type = SWITCH_TYPE;
  config[2].state = 0;
  //nombre = "L3";
  //nombre.toCharArray(config[2].name,2);

  config[3].input = A3;
  config[3].output = 3;
  config[3].type = SWITCH_TYPE;
  config[3].state = 0;
  //nombre = "L4";
  //nombre.toCharArray(config[3].name,2);
  
  config[4].input = A4;
  config[4].output = 4;
  config[4].type = SWITCH_TYPE;
  config[4].state = 0;
  //nombre = "H1";
  //nombre.toCharArray(config[4].name,2);
  
  config[5].input = A5;
  config[5].output = 5;
  config[5].type = SWITCH_TYPE;
  config[5].state = 0;
  //nombre = "H2";
  //nombre.toCharArray(config[5].name,2);
  
  config[6].input = 7;
  config[6].output = 6;
  config[6].type = SWITCH_TYPE;
  config[6].state = 0;
  //nombre = "C1";
  //nombre.toCharArray(config[6].name,2);
  
  
  config[7].input = 8;
  config[7].output = 0;
  config[7].type = SWITCH_TYPE;
  config[7].state = 0;
  //nombre = "E1";
  //nombre.toCharArray(config[7].name,2);



  //nombre = "";//limpio string
#if DEBUG
  Serial.println("ep wrt"); 
#endif

  EEPROM.writeBlock(ADDRESS, config);
}

//l
/*
String configToJson() {
  loadConfig();
  String configStr = String();
  configStr += "{\"";
  configStr += "conf\" : [";
  for ( i = 0; i < SLOTS_QTY; i++) {
    configStr += "{\"i\":";
    configStr += config[i].input;
    configStr += ",";
    configStr += "\"o\":";
    configStr += config[i].output;
    configStr += ",";
    configStr += "\"s\":";
    configStr += config[i].state;
    configStr += ",";
    configStr += "\"t\":";
    configStr += config[i].type;
    configStr += ",";
    configStr += "\"n\":\"";
    configStr += config[i].name;
    configStr += "\"}";
    if (i < (SLOTS_QTY - 1)){
      configStr += ",";
    }

  }
  configStr += "]}";

  //no se utiliza el nombre para la ejecucion del programa por eso se lo vuela de la sram
  for ( i = 0; i < SLOTS_QTY; i++) {
    memset(config[i].name , 0, sizeof config[i].name );
  }

#if DEBUG
   Serial.println("Config output: "); 
   Serial.println(configStr);
#endif

  return configStr;
}
//DIc 2015: Se remueve porque no se utiliza
*/

//usado para lo que llega por ethernet, porque fuerza estado
void updateOutput(int pin, int state){

  for ( i = 0; i < SLOTS_QTY; i++) {
    if (config[i].output==pin){
      if (state){
        digitalWrite(pin, HIGH);
        config[i].state=1;
      }
      else{
        digitalWrite(pin, LOW);
        config[i].state=0;
      }      
    }
  }
  //esta es la linea que hace que se guarden los estados en la eprom
  //EEPROM.updateBlock(ADDRESS, config);
}

//usado para los switchs, que solo invierten
void changeOutput(int pin){

  for ( i = 0; i < SLOTS_QTY; i++) {
    if (config[i].output==pin){
      if (digitalRead(pin)){
        digitalWrite(pin, LOW);
        config[i].state=0;
      }
      else{
        digitalWrite(pin, HIGH);
        config[i].state=1;
      }
      return;      
    }
  }
  //esta es la linea que hace que se guarden los estados en la eprom
  //EEPROM.updateBlock(ADDRESS, config);
}

void imprimirCabeceraHTTP(EthernetClient client, String type){
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: application/");
  client.println(type);
  client.println("Access-Control-Allow-Origin: *");
  client.println();
}


void setup()
{
#if DEBUG
  Serial.begin(9600);
  Serial.println("---"); 
#endif
  createConfig(); 
  loadConfig ();

  for ( i = 0; i < SLOTS_QTY; i++) {
    //Se aprovecha el for para eliminar el name de la config en ejecucion
    //memset(config[i].name , 0, sizeof config[i].name );
    if (config[i].type==SWITCH_TYPE || config[i].type==PULSADOR_TYPE){
      cantidadSwitchs++;
    }
  }
  bouncer = (Bounce *) malloc(sizeof(Bounce) * cantidadSwitchs);

  int bouncerIndex=0;
  //Rutina de inicializacion de estado y configuracion
  for ( i = 0; i < SLOTS_QTY; i++) {
    if (config[i].input!=-1){
      pinMode(config[i].input , INPUT); 
    }
    //digitalWrite(config[i].input, HIGH); //se utilizara el pull up resistor de 20k interno
    if (config[i].output!=-1){
      pinMode(config[i].output, OUTPUT);  
    }
    if (config[i].type==SWITCH_TYPE || config[i].type==PULSADOR_TYPE){
      //Analizar el state, creo que no va mas
      if (config[i].state){
        digitalWrite(config[i].output, HIGH);
      } 
      else {
        digitalWrite(config[i].output, LOW); 
      }
      if (config[i].input!=-1){
        bouncer[bouncerIndex] = Bounce( /*config[i].input,100  */);    // Se remueven los parametros al actualizar a bounce2
        bouncer[bouncerIndex].attach( config[i].input );
        bouncer[bouncerIndex].interval(100);
        bouncerIndex++; 
      }

    }
    else  if (config[i].type==PWM_TYPE){
      analogWrite(config[i].output, config[i].state);      

    }
    else  if (config[i].type==SENSOR_TEMPERATURA_TYPE){
      DHT11.attach(config[i].input);
    }  




  }
  //Fin inicializacion

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
#if DEBUG
  Serial.println("---"); 
#endif
}

//  url buffer size
#define BUFSIZE 32
#define BUFSIZE_PARAMS 80
// Toggle case sensitivity
#define CASESENSE true

void loop()
{

  //·····································Comienzo codigo input handling·····································//  
  for ( i = 0; i < cantidadSwitchs; i++) {

    if (bouncer[i].update()){
      int indexAux=0;
      int h;  
      for ( h = 0; h < SLOTS_QTY; h++) {

        if (config[h].type==SWITCH_TYPE || config[h].type==PULSADOR_TYPE){
          if (indexAux==i){
            if (config[h].type==SWITCH_TYPE){
              changeOutput(config[h].output);
            }/* else{  //Se comenta porque no se usan pulsadores
              if (bouncer[i].risingEdge()){
                changeOutput(config[h].output);
              }
            }*/

#if DEBUG
            Serial.print("Cambio en pin: ");
            Serial.println(config[h].input);
#endif  
            return;
          }
          else{
            indexAux++;
          }  
        }
      }




    }
  }


  //·····································Comienzo codigo restDuino·····································//
  char clientline[BUFSIZE];
  char paramsline[BUFSIZE_PARAMS];
  int index = 0;
  // listen for incoming clients
#if defined(ARDUINO) && ARDUINO >= 100
  EthernetClient client = server.available();
#else
  Client client = server.available();
#endif
  if (client) {
    memset(clientline, 0, BUFSIZE);
    memset(paramsline, 0, BUFSIZE);

    //  reset input buffer
    index = 0;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //  fill url the buffer
        if(c != '\n' && c != '\r' && c != '?' && index < BUFSIZE){ // Reads until either an eol character is reached or the buffer is full
          clientline[index++] = c;
          continue;
        } 


        if (c == '?'){//HAY PARAMETROS
          clientline[index++] = ' '; // Se agrega un espacio al final porque este caracter se usa para cortar el string luego, solo aplica para cuando hay parametros, con ? en el url
          index = 0; 
          c = client.read();
          while (c != '\n' && c != '\r' && c != ' ' && index < BUFSIZE){   
            paramsline[index++] = c;
            c = client.read();
          }  
        }


#if DEBUG
        Serial.print("client available bytes before flush: "); 
        Serial.println(client.available());
        Serial.print("request = "); 
        Serial.println(clientline);
        Serial.print("params = "); 
        Serial.println(paramsline);
#endif

        // Flush any remaining bytes from the client buffer
        client.flush();

#if DEBUG
        // Should be 0
        Serial.print("client available bytes after flush: "); 
        Serial.println(client.available());
#endif

        //  convert clientline into a proper
        //  string for further processing
        String urlString = String(clientline);
        String urlStringParams = String(paramsline);

        //  extract the operation
        String op = urlString.substring(0,urlString.indexOf(' '));

        //  we're only interested in the first part...
        urlString = urlString.substring(urlString.indexOf('/'), urlString.indexOf(' ', urlString.indexOf('/')));


        //  put what's left of the URL back in client line
#if CASESENSE
        urlString.toUpperCase();
#endif
        urlString.toCharArray(clientline, BUFSIZE);

        //  get the first two parameters
        char *pin = strtok(clientline,"/");
        char *value = strtok(NULL,"/");

        //  this is where we actually *do something*!
        char outValue[20] = "MU";
        String jsonOut = String();
        if (urlString!="/FAVICON.ICO"){
              if(pin != NULL){
#if DEBUG
                Serial.print("Selected pin: "); 
                Serial.println();
#endif
                if(value != NULL){
#if DEBUG
                  Serial.print("Selected value: "); 
                  Serial.println(value);
#endif

#if DEBUG
                  //  set the pin value
                  Serial.println("setting pin");
#endif

                  //  select the pin
                  int selectedPin = atoi (pin);
#if DEBUG
                  Serial.println(selectedPin);
#endif
                  //Revisar si esta bien que cambie los tipos de output input
                  //  set the pin for output
                  //pinMode(selectedPin, OUTPUT);

                  //  determine digital or analog (PWM)
                  if(strncmp(value, "HIGH", 4) == 0 || strncmp(value, "LOW", 3) == 0){

#if DEBUG
                    //  digital
                    Serial.println("digital");
#endif

                    if(strncmp(value, "HIGH", 4) == 0){
#if DEBUG
                      Serial.println("HIGH");
#endif
                      //digitalWrite(selectedPin, HIGH);
                      updateOutput(selectedPin, 1);
                    }

                    if(strncmp(value, "LOW", 3) == 0){
#if DEBUG
                      Serial.println("LOW");
#endif
                      //digitalWrite(selectedPin, LOW);
                      updateOutput(selectedPin, 0);
                    }

                  } 
                  else {

#if DEBUG
                    //  analog
                    Serial.println("analog");
#endif
                    //  get numeric value
                    int selectedValue = atoi(value);              
#if DEBUG
                    Serial.println(selectedValue);
#endif
                    analogWrite(selectedPin, selectedValue);

                  }

                  //  return status
                  imprimirCabeceraHTTP(client, "html");
                } 
                else {
#if DEBUG
                  //  read the pin value
                  Serial.println("reading pin");
#endif

                  //  determine analog or digital
                  if(pin[0] == 'a' || pin[0] == 'A'){

                    //  analog
                    int selectedPin = pin[1] - '0';

#if DEBUG
                    Serial.println(selectedPin);
                    Serial.println("analog");
#endif
                    sprintf(outValue,"%d",analogRead(selectedPin));

#if DEBUG
                    Serial.println(outValue);
#endif

                  } 
                  else if(pin[0] != NULL) {

                    //  digital
                    int selectedPin = pin[0] - '0';

#if DEBUG
                    Serial.println(selectedPin);
                    Serial.println("digital");
#endif

                    for ( i = 0; i < SLOTS_QTY; i++) {
                      if (config[i].type==SENSOR_TEMPERATURA_TYPE){
                        if (config[i].input==selectedPin){
                          DHT11.read();
                          
                          String out ="<t>";
                          out+="</t>";
                          out+="<h>";
                          out+="</h";
                          //client.println(out);
                         
                          //sprintf(outValue,"%s","ON");
                          sprintf(outValue,"<t>%d</t><h>%d</h>",DHT11.temperature,DHT11.humidity);
                          //sprintf(outValue,"%s",out);

#if DEBUG                    
                          Serial.print("Humidity (%): ");
                          Serial.println((float)DHT11.humidity, DEC);

                          Serial.print("Temperature (°C): ");
                          Serial.println((float)DHT11.temperature, DEC);
#endif
                        }
                      }
                      else if (config[i].type==SWITCH_TYPE || config[i].type==PULSADOR_TYPE){
                        if (config[i].output==selectedPin){

                          int inValue = digitalRead(selectedPin);

                          if(inValue == 0){
                            sprintf(outValue,"%s","OFF");
                            //sprintf(outValue,"%d",digitalRead(selectedPin));
                          }
                          else if(inValue == 1){
                            sprintf(outValue,"%s","ON");
                          }

                        }
                      }                      
                    }




                  }

                  //  assemble the json output
                  /*jsonOut += "{\"";
                   jsonOut += pin;
                   jsonOut += "\":\"";*/
                  jsonOut = "<state>";
                  jsonOut += outValue;
                  jsonOut += "</state>";
                  /*jsonOut += "\"}";*/
#if DEBUG
                  Serial.println(outValue);
#endif
                  imprimirCabeceraHTTP(client, "xml");
                  client.println(jsonOut);
                  jsonOut="";
                }
              } 
              else {

                //  error
#if DEBUG
                Serial.println("erroring");
#endif
                client.println("HTTP/1.1 404 Not Found");
                client.println("Content-Type: text/html");
                client.println();

              }
            
            //Dic 2015: Se desactiva toda esta func.
            /*
            else{
              //se solicito /CONFIG
#if DEBUG
              Serial.println("Pedido de config");
#endif              
              imprimirCabeceraHTTP(client, "json");
              client.println(configToJson());
            }
            */
          
          
          //Dic 2015: Se desactiva toda esta func.
          /*
          else{
            //se solicito /SAVECONFIG
            
            
            
            imprimirCabeceraHTTP(client, "html");
            client.println("Configuracion guardada con exito");


            int indexBgn = 0;          
            int indexEnd = 0;
            String param;
            String value;
            int slot;
            while (indexEnd!=-1){
              indexEnd = urlStringParams.indexOf("=",indexBgn);             
              param = urlStringParams.substring(indexBgn, indexEnd);
              indexBgn = indexEnd+1;
              indexEnd = urlStringParams.indexOf("&",indexBgn);
              if (indexEnd!=-1){
                value = urlStringParams.substring(indexBgn, indexEnd);
                indexBgn=indexEnd +1;
              }
              else{
                value = urlStringParams.substring(indexBgn, urlStringParams.length());
              }  
#if DEBUG
              Serial.print("param: ");
              Serial.println(param);
              Serial.print("value: ");
              Serial.println(value);
#endif         
              if (param=="slot"){
                slot = value.toInt();
              }
              else if (param == "n"){
                value.toCharArray(config[slot].name,20);                
              }
              else if (param == "i"){
                config[slot].input = value.toInt();
              }
              else if (param == "o"){
                config[slot].output = value.toInt();
              }
              else if (param == "s"){
                config[slot].state = value.toInt();
              }
              else if (param == "t"){
                config[slot].type = value.toInt();
              }
            }
            EEPROM.writeBlock(ADDRESS, config);
            
            
          }
          //Dic 2015: Se desactiva toda esta func.
          */
        }
        break;
      }
    }

    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    //client.stop();
    client.stop();
    while(client.status() != 0){
      delay(5);
    }
  }
}

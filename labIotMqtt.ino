#include <WiFi.h>
#include <PubSubClient.h>

#include <ESP32Servo.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT11

#include "ESP32_MailClient.h"

//**************************************
//*********** MQTT CONFIG **************
//**************************************
const char *mqtt_server = "ioticos.org";
const int mqtt_port = 1883;
const char *mqtt_user = "kAPk05Tvb0piZoc";
const char *mqtt_pass = "w3mWV0q6UE769xe";
const char *root_topic_subscribe = "xROmbOs5kE5KW58/input";
const char *root_topic_publish = "xROmbOs5kE5KW58/output";

const char *root_topic_subscribe_light = "xROmbOs5kE5KW58/light";
const char *root_topic_subscribe_door = "xROmbOs5kE5KW58/door";
const char *root_topic_subscribe_modifyTemp = "xROmbOs5kE5KW58/modifyTemp";
const char *root_topic_publish_data = "xROmbOs5kE5KW58/data";
const char *root_topic_publish_temp = "xROmbOs5kE5KW58/temp";
const char *root_topic_publish_humedity = "xROmbOs5kE5KW58/humedity";

//Config mail
#define emailSenderAccount    "casanareiot@gmail.com"    
#define emailSenderPassword   "CASIOT99."
#define emailRecipient        "jorgepatarroyo@unisangil.edu.co"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465

String emailSubject = "ESP32 Test - PRIMER ENVIO";
String emailBodyMessage = "<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP32 board</p></div>";

//**************************************
//*********** WIFICONFIG ***************
//**************************************
const char* ssid ="Yopal19" ;
const char* password="Yopaltqm";

//**************************************
//*********** GLOBAL CONFIG **************
//**************************************
float temperature=0;
float humidity=0;
int stateRele=0;
int stateServo=0;
static const int servoPin = 23;
const int fan = 5;
const int light = 21;

String topic="";
String body="";
int tempModify=34;
int banderaControllerTempMail = 0;
float lastTemperature;

const unsigned long publishTempAll = 25000;
unsigned long lastPublishTempAll;

const unsigned long publishHumedityAll = 25000;
unsigned long lastPublishHumedityAll;


//objects
DHT dht(DHTPIN, DHTTYPE);
Servo myservo;
SMTPData smtpData;



//**************************************
//*********** GLOBALES   ***************
//**************************************
WiFiClient espClient;
PubSubClient client(espClient);
char msg[25];
long count=0;


//************************
//** F U N C I O N E S ***
//************************
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setup_wifi();
// Callback function to get the Email sending status
void sendCallback(SendStatus info);

void read_sensor_data(void * parameter) {
   for (;;) {
    readDataSensor();
    readModifyTemp();
   }
}

void readDataSensor(){
  humidity = dht.readHumidity();
  lastTemperature = dht.readTemperature();
  if(!isnan(lastTemperature)){
    temperature = lastTemperature;
    }
  Serial.print(F("Humedad: "));
  Serial.print(humidity);
  Serial.print("%");
  Serial.print(F("  Temperatura: "));
  Serial.print(temperature);
  Serial.println(F("°C "));
  delay(3000);
  }

void readTopicDoor(){
  Serial.println(topic);
  if(topic!=""){
    String readTopic = topic.substring(16,20);
    Serial.println(readTopic);
    if(readTopic=="door"){
      if(body=="abrir"){
        Serial.println("readTopic -> ok");
        Serial.println(body);
        myservo.write(90);
        emailSubject = "ESP32 ALERTA - PUERTA ABIERTA";
        emailBodyMessage = "<div style=\"color:#2f4468;\"><h1>Hola esto es una notificacion!</h1><p>- La puerta a sido abierta</p></div>";
        ConfigSendMailTo();
        topic="";
        stateServo = 1;
        }
      if(body=="cerrar"){
        Serial.println("readTopic -> ok");
        Serial.println(body);
        myservo.write(1);
        topic="";
        stateServo = 0;
        }
       if(body=="enviarCorreo"){
        Serial.println("enviarCorreo -> ok");
        Serial.println(body);
        ConfigSendMailTo();
        }
      }
    }
  }

 void readTopicLight(){
  Serial.println(topic);
  if(topic!=""){
    String readTopic = topic.substring(16,21);
    Serial.println(readTopic);
    if(readTopic=="light"){
      if(body=="encender"){
        Serial.println("readTopicLight -> ok");
        Serial.println(body);
        digitalWrite(light,LOW);
        topic="";
        }
      if(body=="apagar"){
        Serial.println("readTopicLight -> ok");
        Serial.println(body);
        digitalWrite(light,HIGH);
        topic="";
        }
      }
    }
  }

 void readModifyTemp(){
  Serial.println(topic);
  if(topic!=""){
    String readTopic = topic.substring(16,26);
    Serial.println(readTopic);
    if(readTopic=="modifyTemp"){
      if(body.toInt()>=0){
        Serial.println("readModifyTemp -> ok");
        Serial.println(body.toInt());
        tempModify = body.toInt();
        topic="";
        }
      }
    }
  }

 void controllerTemp(){
  if(temperature>tempModify){
    Serial.println("Encendido Ventilador -> ok");
    digitalWrite(fan,LOW);
    emailSubject = "ESP32 ALERTA - VENTILADOR ENCENDIDO";
    emailBodyMessage = "<div style=\"color:#2f4468;\"><h1>Hola señor usuario, esto es una notificacion!</h1><p>- La temperatura a subido, obligando a encender el ventilador</p></div>";
    if(banderaControllerTempMail == 0){
      ConfigSendMailTo();
      banderaControllerTempMail = 1; 
      }
    }else{
      Serial.println("Apagado ventilador -> ok");
      digitalWrite(fan,HIGH);
      banderaControllerTempMail = 0;
      }
  }

  // Callback function to get the Email sending status
void sendCallback(SendStatus msg) {
  // Print the current status
  Serial.println(msg.info());

  // Do something when complete
  if (msg.success()) {
    Serial.println("----------------");
  }
}
  


void setup_task() {
  xTaskCreate(
    read_sensor_data,    
    "Read sensor data",  
    1000,            
    NULL,            
    1,               
    NULL            
  );
}

void initServo(){
  // Allow allocation of all timers for servo library
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  // Set servo PWM frequency to 50Hz
  myservo.setPeriodHertz(50);
  // Attach to servo and define minimum and maximum positions
  // Modify as required
  myservo.attach(servoPin,500, 2400);
  }

void setup() {
  pinMode(fan,OUTPUT);
  pinMode(light,OUTPUT);
  digitalWrite(light,HIGH);
  initServo();
  dht.begin();
  Serial.begin(115200);
  setup_wifi();
  setup_task();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {    
  if (!client.connected()) {
    reconnect();
  }

  if (client.connected()){
    dataPublish();
    tempPublish();
    humedityPublish();
  }
  client.loop();
  readTopicDoor();
  readTopicLight();
  controllerTemp();
}

void tempPublish(){
  //My timer
  unsigned long topLoop = millis();
  // this is setInterval
  if (topLoop - lastPublishHumedityAll >= publishHumedityAll) {
    lastPublishHumedityAll = topLoop;
    String str = "La Temperatura es -> " + String(temperature)+ "°C";
    str.toCharArray(msg,30);
    client.publish(root_topic_publish_temp,msg);
    delay(200);
    //delay(25000);
    }
}

void humedityPublish(){
  //My timer
  unsigned long topLoop2 = millis();
  // this is setInterval
  if (topLoop2 - lastPublishTempAll >= publishTempAll) {
    lastPublishTempAll = topLoop2;
    String str = "La Humedad es -> "+ String(humidity)+ "%";
    str.toCharArray(msg,30);
    client.publish(root_topic_publish_humedity,msg);
    str="";
    delay(200);
    //delay(25000);
    }
}

void dataPublish(){
  String str = "La Temperatura es -> " + String(temperature)+ "°C";
  str.toCharArray(msg,30);
  client.publish(root_topic_publish_data,msg);
  str="";
  delay(200);
  str = "La Humedad es -> "+ String(humidity)+ "%";
  str.toCharArray(msg,30);
  client.publish(root_topic_publish_data,msg);
  str="";
  delay(200);
  str = "El estado de la puerta es -> "+ String(stateServo);
  str.toCharArray(msg,35);
  client.publish(root_topic_publish_data,msg);
  str="";
  delay(200);
  str = "La temperatura limite es -> "+ String(tempModify);
  str.toCharArray(msg,35);
  client.publish(root_topic_publish_data,msg);
  delay(2000);
  }


//*****************************
//***    CONEXION WIFI      ***
//*****************************
void setup_wifi(){
  delay(10);
  // Nos conectamos a nuestra red Wifi
  Serial.println();
  Serial.print("Conectando a ssid: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conectado a red WiFi!");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}



//*****************************
//***    CONEXION MQTT      ***
//*****************************

void reconnect() {

  while (!client.connected()) {
    Serial.print("Intentando conexión Mqtt...");
    // Creamos un cliente ID
    String clientId = "IOTICOS_H_W_";
    clientId += String(random(0xffff), HEX);
    // Intentamos conectar
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
      Serial.println("Conectado!");
      // Nos suscribimos
      if(client.subscribe(root_topic_subscribe)){
        Serial.println("Suscripcion ok");
      }else{
        Serial.println("fallo Suscripciión");
      }
      if(client.subscribe(root_topic_subscribe_light)){
        Serial.println("Suscripcion topic light ok");
      }else{
        Serial.println("fallo Suscripciión to light");
      }
      //topic suscribe door
      if(client.subscribe(root_topic_subscribe_door)){
        Serial.println("Suscripcion topic door ok");
      }else{
        Serial.println("fallo Suscripciión to door");
      }
      //topic suscribe modifyTemp
      if(client.subscribe(root_topic_subscribe_modifyTemp)){
        Serial.println("Suscripcion topic modifyTemp ok");
      }else{
        Serial.println("fallo Suscripciión to modifyTemp");
      }
    } else {
      Serial.print("falló :( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 5 segundos");
      delay(5000);
    }
  }
}


//*****************************
//***       CALLBACK        ***
//*****************************

void callback(char* topico, byte* payload, unsigned int length){
  String incoming = "";
  Serial.print("Mensaje recibido desde -> ");
  Serial.print(topico);
  topic=topico;
  Serial.println("");
  for (int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }
  incoming.trim();
  body=incoming;
  Serial.println("Mensaje -> " + incoming);

}

void ConfigSendMailTo(){
  Serial.println("Preparing to send email");
  Serial.println();  
  // Set the SMTP Server Email host, port, account and password
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);

  // For library version 1.2.0 and later which STARTTLS protocol was supported,the STARTTLS will be 
  // enabled automatically when port 587 was used, or enable it manually using setSTARTTLS function.
  //smtpData.setSTARTTLS(true);

  // Set the sender name and Email
  smtpData.setSender("ESP32", emailSenderAccount);

  // Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High");

  // Set the subject
  smtpData.setSubject(emailSubject);

  // Set the message with HTML format
  smtpData.setMessage(emailBodyMessage, true);
  // Set the email message in text format (raw)
  //smtpData.setMessage("Hello World! - Sent from ESP32 board", false);

  // Add recipients, you can add more than one recipient
  smtpData.addRecipient(emailRecipient);
  //smtpData.addRecipient("YOUR_OTHER_RECIPIENT_EMAIL_ADDRESS@EXAMPLE.com");

  smtpData.setSendCallback(sendCallback);

  //Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData))
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

  //Clear all data from Email object to free memory
  smtpData.empty();
  topic="";
  }

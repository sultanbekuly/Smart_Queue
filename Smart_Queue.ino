#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

#include <Servo.h>
Servo myservo;


// Replace with your network credentials
const char* ssid = "";      //wifi name
const char* password = "";  //wifi password

// Initialize Telegram BOT
#define BOTtoken "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"  // your Bot Token (Get from Botfather)


#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 0.1 second.
int botRequestDelay = 100;
unsigned long lastTimeBotRan;


int servo_pin = 14;//servo pin
int servo_locked_value = 180;
int servo_unlocked_value = 30;

const int ledPin = 2;
//bool ledState = LOW;

String queue[10] ={};//array of queue
int queue_array_len = 0;//index of queue
bool room_is_free = true;//initially the room is free
bool door_is_locked = true;// the door is locked

//check if person already in the queue:
bool if_chat_id_not_in_queue(String chat_id){
  for(int i = 0; i < queue_array_len; i++){ 
    if(chat_id == queue[i]){
      return false;
    }
  }
  return true;
}

//printing the queue
void print_queue(){
  Serial.print("In queue - ");Serial.println(queue_array_len);
  for(int i = 0; i < queue_array_len; i++){
    Serial.println(queue[i]);
  }
}



// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);

    // Print the received message
    String text = bot.messages[i].text;
    Serial.print("From: ");Serial.println(chat_id);
    Serial.print("Message: ");Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "By this bot you can stand in line to the Gaming room.\n";
      welcome += "Please see my /options.\n\n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/options") {
      //String keyboardJson = "[[\"/Unlock\", \"/Lock\"],[\"/WC\"],[\"/cancel\", \"/status\"]]";
      String keyboardJson = "[[\"/Stand_in_line\"],[\"/cancel\", \"/status\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, "Here is my options:", "", keyboardJson, true);
    }
    
    if (text == "/Stand_in_line") {
      //check if person already in the queue:
      if(if_chat_id_not_in_queue(chat_id)){
        //add person to the queue
        queue[queue_array_len] = chat_id;//array of queue
        queue_array_len++;
        bot.sendMessage(chat_id, "You joined the queue.", "");
      }else{
        bot.sendMessage(chat_id, "You are already in the queue.", "");
      }
      
      //printing the queue
      print_queue();
    }
    
    if (text == "/cancel") {
      //check if person already in the queue:
      if(if_chat_id_not_in_queue(chat_id)){
        bot.sendMessage(chat_id, "You were not in the queue.", "");
      }else{
        //checking if the person first one to set room to free again.
        if (queue[0]=chat_id){
          room_is_free = true;
        }
        
        //remove person from the queue
        for(int i = 0; i < queue_array_len; i++){
          if(queue[i]=chat_id){
            queue[i] = "";
            //move items after the deleted one:
            for(int j = i;j < queue_array_len; j++){
              queue[j] = queue[j+1];
            }
          }
        }
        queue_array_len--;
        bot.sendMessage(chat_id, "You are no longer in the queue.", "");

        //printing the queue
        print_queue();
      }
    }

    if (text == "/status") {
      //printing the queue
      print_queue();
      
      int current_person_id_in_queue = -1;
      for(int i = 0; i < queue_array_len; i++){
        if(queue[i]==chat_id){
          current_person_id_in_queue = i+1;
        }
      }
      String message = "The queue consist of " + String(queue_array_len ) + " person(s).\n";
      if(current_person_id_in_queue != -1){
        message +="You are #"+String(current_person_id_in_queue)+ " in line.\n";
      }
      
      bot.sendMessage(chat_id, message, "");
    }
    
    if (text == "/unlock_the_door_from_outside") {
      //checking if this first person in queue:
      if(chat_id == queue[0]){
        door_is_locked = false;
        myservo.write(servo_unlocked_value);//unlocked
        bot.sendMessage(chat_id, "The door is unlocked! \nCome in and lock the door by sending /lock_the_door_from_inside.", "");
      }else{
        bot.sendMessage(chat_id, "Something went wrong. \nPlease try to check your status in queue by /status command.", "");
      }
    }

    if (text == "/lock_the_door_from_inside") {
      //checking if this first person in queue:
      if(chat_id == queue[0]){
        door_is_locked = true;
        myservo.write(servo_locked_value);//locked
        bot.sendMessage(chat_id, "The door is locked! \nTo unlock the door send /unlock_the_door_from_inside.", "");
      }else{
        bot.sendMessage(chat_id, "Something went wrong. \nPlease try to check your status in queue by /status command.", "");
      }
    }
    
    if (text == "/unlock_the_door_from_inside") {
      //checking if this first person in queue:
      if(chat_id == queue[0]){
        door_is_locked = false;
        myservo.write(servo_unlocked_value);//unlocked
        bot.sendMessage(chat_id, "The door is unlocked! We will lock it in 3 sec", "");
        delay(3000);
        door_is_locked = true;
        room_is_free = true;
        myservo.write(servo_locked_value);//locked
        
        //remove the person from the queue
        for(int i = 0; i < queue_array_len; i++){
          if(queue[i]=chat_id){
            queue[i] = "";
            //move items after the deleted one:
            for(int j = i;j < queue_array_len; j++){
              queue[j] = queue[j+1];
            }
          }
        }
        queue_array_len--;
        bot.sendMessage(chat_id, "The door is locked. Bye!", "");

      }else{
        bot.sendMessage(chat_id, "Something went wrong. \nPlease try to check your status in queue by /status command.", "");
      }
    }

    
  }
    

}

void setup() {
  Serial.begin(115200);

  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
    client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  #endif

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  //Servo - close:
  myservo.attach(servo_pin);
  myservo.write(180);
}

void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();

    //check if the room free
    if(room_is_free ==true && queue[0]!=""){
      room_is_free = false;
      //invite to the room:
      Serial.println("Invinting to the room: "+queue[0]);
      bot.sendMessage(queue[0], "The room is free! \n Please come and send /unlock_the_door_from_outside to unlock the door.", "");
    }

    //Indicating and opening/closing the door:
    digitalWrite(ledPin, door_is_locked);//ledState
    if(door_is_locked){
      myservo.write(servo_locked_value);//locked
    }else{
      myservo.write(servo_unlocked_value);//unlocked
    }
    
  }
}

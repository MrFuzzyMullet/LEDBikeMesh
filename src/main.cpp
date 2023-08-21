//Includes and debug setup
#include <Arduino.h>
#include <painlessMesh.h>
#include <Button2.h> 
#include <FastLED.h>
#include <TaskScheduler.h>
#include <esp_task_wdt.h>
#include <patterns.h>

#define DEBUG 0    // SET TO 0 OUT TO REMOVE TRACES
#if DEBUG
#define D_SerialBegin(...) Serial.begin(__VA_ARGS__);
#define D_printf(...)    Serial.printf(__VA_ARGS__)
#define D_write(...)    Serial.write(__VA_ARGS__)
#define D_println(...)  Serial.println(__VA_ARGS__)
#else
#define D_SerialBegin(...) 
#define D_printf(...)
#define D_write(...)
#define D_println(...)
#endif

// Controller setup and buttons
// Defined in platformio.ini

Button2 button;

// Mesh details
#define MESH_PREFIX "MeshNetwork"
#define MESH_PASSWORD "somethingsneaky"
uint16_t MESH_PORT = 5555;
WiFiMode_t MESH_MODE = WIFI_AP_STA;
uint8_t MESH_CHANNEL = 6;          //Mesh channel
uint8_t MESH_HIDDEN = 1;           //0 for broadcast, 1 for hidden
#define   MAX_NODES         25  //max number of nodes that can connect to a single source
#define   MAX_MESH_LEDS      MAX_NODES*NUM_LEDS
SimpleList<uint32_t> nodes;
uint8_t  numNodes = 1; // default
uint8_t  nodePos = 0; // default
uint16_t meshNumLeds = NUM_LEDS; // at boot we are alone
char role[7] = "MASTER" ; // default start out as master unless told otherwise
uint32_t activeSlave ;
int Quin_ID = 12345678;
painlessMesh mesh;

//Patterns
#define DEFAULT_BRIGHTNESS 100
#define AUTO_ADVANCE_DELAY 120
#define CURRENTPATTERN_SELECT_DEFAULT_INTERVAL 10
CRGB statusLeds[NUM_STATUS_LEDS];
CRGB meshLeds[MAX_MESH_LEDS];
LEDPattern currentPattern = LEDPattern::RB_March;
unsigned long currentTime;

//Function Initialize
uint32_t get_millisecond_timer();
void showStatusLED();
void sendMessage();
void currentPatternRun();
void selectNextPattern();
void findRootNode() ;
void receivedCallback( uint32_t from, String &msg ) ;
void newConnectionCallback(uint32_t nodeId) ;
void changedConnectionCallback() ;
void nodeTimeAdjustedCallback(int32_t offset) ;
void delayReceivedCallback(uint32_t from, int32_t delay) ;
void shortPress(Button2& button);
void longPress(Button2& button);
void doublePress(Button2& button);

//Task and Scheduler
Scheduler userScheduler; // to control your personal task
#define TASK_CHECK_BUTTON_PRESS_INTERVAL    20   // in milliseconds
Task tCheckButtonPress( TASK_CHECK_BUTTON_PRESS_INTERVAL, TASK_FOREVER, []() {button.loop();});
Task tSendMessage( TASK_SECOND * 15, TASK_FOREVER, &sendMessage ); // check every 15 second if we have a new message to send
Task tSelectNextPattern( TASK_SECOND * AUTO_ADVANCE_DELAY, TASK_FOREVER, &selectNextPattern);  // switch to next pattern every AUTO_ADVANCE_DELAY seconds
Task tCurrentPatternRun( CURRENTPATTERN_SELECT_DEFAULT_INTERVAL, TASK_FOREVER, &currentPatternRun);

void setup() {
  D_SerialBegin(115200);
  delay(random(1000,1500)); // Startup delay; let things settle down. Randomized so nodes don't start at the same time and connect faster
  
  //Start Mesh network
  if (DEBUG) {
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION | SYNC );  // set before init() so that you can see startup messages
  } else {
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
  }
  //mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, MESH_MODE, MESH_CHANNEL, MESH_HIDDEN, MAX_NODES);  //more details to mesh network. not necessary?
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  // Init LED
  //For M5 Atom series
  #ifdef atomlite
    mesh.setContainsRoot();
    FastLED.addLeds<WS2812B, STATUS_LED_PIN, GRB>(statusLeds, NUM_STATUS_LEDS);
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(DEFAULT_BRIGHTNESS); 
    FastLED.setMaxPowerInVoltsAndMilliamps(5,500);
  #endif

  //For QuinLED ESP32
  #ifdef QuinLED
    mesh.setRoot();
    FastLED.addLeds<WS2811, LED_PIN, RGB>(leds, NUM_LEDS);
    FastLED.addLeds<WS2811, LED_PIN2, RGB>(leds2, NUM_LEDS2);
    FastLED.addLeds<WS2812B, STATUS_LED_PIN, GRB>(statusLeds, NUM_STATUS_LEDS);
    FastLED.setBrightness(255); 
    FastLED.setMaxPowerInVoltsAndMilliamps(12,8000);
  #endif
  
  // Init button
  button.begin(BUTTON_PIN);
  button.setClickHandler(shortPress);
  button.setDebounceTime(20);
  button.setLongClickDetectedHandler(longPress);
  button.setLongClickTime(1000);
  button.setDoubleClickHandler(doublePress);
  button.setDoubleClickTime(300);

  //Start scheduler tasks
  userScheduler.addTask( tSendMessage );
  tSendMessage.enable();
  userScheduler.addTask( tCheckButtonPress );
  tCheckButtonPress.enable() ;
  userScheduler.addTask(tSelectNextPattern);
  tSelectNextPattern.enable();
  userScheduler.addTask( tCurrentPatternRun );

  //Start watchdog timer which resets after 10s if the LEDS do not update
  esp_task_wdt_init(10, true);
  esp_task_wdt_add(NULL);

  D_printf("Starting up... my Node ID is: ");
  D_println(mesh.getNodeId()) ;
  findRootNode();

  tCurrentPatternRun.enable();
}

void loop() {
  mesh.update();
}

//setup mesh timer in ms
uint32_t get_millisecond_timer() {
   return mesh.getNodeTime()/1000 ;
}

//Setup button presses
//Short press changes to next pattern if master
void shortPress(Button2& button) {
  if((strcmp(role, "MASTER") == 0 || strcmp(role, "Quin") == 0) && tSelectNextPattern.isEnabled() == true)  {
    tSelectNextPattern.forceNextIteration();
    tSendMessage.forceNextIteration();
  }
}

//doublepress pauses pattern on current pattern
void doublePress(Button2& button) {
  if( tSelectNextPattern.isEnabled() ) {
    tSelectNextPattern.disable();
    D_printf("SelectPattern Disabled\n");
  }
  else if((strcmp(role, "MASTER") == 0 || strcmp(role, "Quin") == 0) && tSelectNextPattern.isEnabled() == false ) {
    tSelectNextPattern.enableDelayed(CURRENTPATTERN_SELECT_DEFAULT_INTERVAL);
    D_printf("SelectPattern Enabled\n");
  }
}

//Long press (1s) pauses the mesh and gives back master control
bool isMeshRunning = true;
void longPress(Button2& btn) {
  if(isMeshRunning) {
    // If the mesh network is running, stop it
    mesh.stop();
    isMeshRunning = false;
    strcpy(role, "MASTER");
    tSelectNextPattern.enableIfNot();
    tCheckButtonPress.enableIfNot();
    findRootNode();
    D_printf("Mesh Disabled\n");
  } else {
    // If the mesh network is not running, restart controller
    D_printf("Restarting\n");
    ESP.restart();
  }
}

//Send message of pattern
void sendMessage() {
  static StaticJsonDocument<100> msg;
  msg["Pattern"] = static_cast<int>(currentPattern);
  char str[100];
  serializeJson(msg, str);
  mesh.sendBroadcast(str);
  D_printf("%s (%u) %u: Sent broadcast message: ", role, nodePos, mesh.getNodeTime() );
  D_println(str);
}

//find the master node or find Quin and set as master
void findRootNode() {
  auto nodes = mesh.getNodeList(true);
  
  uint32_t masterNodeId = 0; // Start with min possible ID
  for(auto& node : nodes) {   
    masterNodeId = max(masterNodeId, node);
  }
  D_printf("Starting findRootNode(). Node count: %d\n", nodes.size());

  // If "Quin" is in range, it becomes the master node
  for(auto& node : nodes) {
    D_printf("Checking node: %d\n", node);
    if(node == Quin_ID) { //Quin_ID defined above
      strcpy(role, "Quin");
      masterNodeId = node;
      D_printf("Found Quin, setting as master.\n");
      break;
    }
  }

  // If "Quin" is not in range, then the node with the highest ID becomes the master
  if ((mesh.getNodeId() != Quin_ID) && (mesh.getNodeId() == masterNodeId)) {
    strcpy(role, "MASTER");
    tSendMessage.enableIfNot();
    tCheckButtonPress.enableIfNot();
    tSelectNextPattern.enableIfNot();
    D_printf("%s %u: findRootNode(); I am MASTER\n", role, mesh.getNodeTime());
  } 
  else if (mesh.getNodeId() == Quin_ID) {
    strcpy(role, "Quin");
    tSendMessage.enableIfNot();
    tCheckButtonPress.enableIfNot();
    tSelectNextPattern.enableIfNot();
    D_printf("%s %u: findRootNode(); I am Quin\n", role, mesh.getNodeTime());
  }
  else {
    strcpy(role, "SLAVE");
    tSendMessage.disable();         // Only MASTER sends broadcast
    tSelectNextPattern.disable();
    D_printf("%s %u: findRootNode(); I am SLAVE\n", role, mesh.getNodeTime());
  }
  meshNumLeds = NUM_LEDS * numNodes;
  D_printf("\n%s %u: %d nodes. Mesh Leds: %d. NodePos: %u root: %d\n", role, mesh.getNodeTime(), numNodes, meshNumLeds, nodePos, mesh.isRoot());
  showStatusLED();
  tSendMessage.forceNextIteration();
}

//how to handle received messages
void receivedCallback( uint32_t from, String &msg ) {
  D_printf("Received msg from %u: %s at %u\n", from, msg.c_str(),mesh.getNodeTime());
  if( strcmp(role, "SLAVE") == 0 ) {
    static StaticJsonDocument<100> doc;
    deserializeJson(doc, msg);
    int patternAsInt = doc["Pattern"];

    if(patternAsInt != static_cast<int>(currentPattern)) {
      if(patternAsInt >= static_cast<int>(LEDPattern::RB_March) && patternAsInt < static_cast<int>(LEDPattern::COUNT)) {
        currentPattern = static_cast<LEDPattern>(patternAsInt);
        D_printf("Pattern changed to %d. \n", patternAsInt);
      } else {
        D_println("Invalid pattern received");
      }
    }
  }
}

//How to handle new connections to the mesh and broadcasts pattern
void newConnectionCallback(uint32_t nodeId) {
  D_printf("%s %u: New Connection from nodeId = %u\n", role, mesh.getNodeTime(), nodeId);
  findRootNode();
  nextChangeTime = 0;
  if( strcmp(role, "Quin") == 0 || strcmp(role, "MASTER") == 0) {
    tSendMessage.forceNextIteration();
  }
}

//Checks master on changed connection and broadcasts pattern
void changedConnectionCallback() {
  D_printf("%s %u: Changed connections %s\n", role, mesh.getNodeTime(), mesh.subConnectionJson().c_str());
  findRootNode();
  nextChangeTime = 0;
  if( strcmp(role, "Quin") == 0 || strcmp(role, "MASTER") == 0) {
    tSendMessage.forceNextIteration();
  }
}

void nodeTimeAdjustedCallback(int32_t offset) {
  D_printf("%s: Adjusted time %u. Offset = %d\n", role, mesh.getNodeTime(), offset);
  nextChangeTime = 0;
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  D_printf("Delay to node %u is %d us\n", from, delay);
}

// Status LED color based on connectivity
void showStatusLED() {
  if (!isMeshRunning) {
    statusLeds[0] = CHSV(64,255,50); //yellow
  } else if( (strcmp(role,"MASTER") == 0 )  and (mesh.getNodeList().size() > 0)) {
    statusLeds[0] = CHSV(192,255,50); //Purple
  } else if ((strcmp(role, "Quin") == 0)) {
    statusLeds[0] = CHSV(0,0,50); //White
  } else if (strcmp(role, "SLAVE")== 0) {
    statusLeds[0] = CHSV(96,255,50); //Green
  } else {
    statusLeds[0] = CHSV(255,255,50); //Red
  } 
  FastLED.show();
}

//Select next pattern from the patterns file. Force the message of the next pattern to all nodes
void selectNextPattern() {
  // Gets the next pattern.
  currentPattern = getNextPattern(currentPattern);
  tSendMessage.forceNextIteration();
  D_printf("Pattern Changed. New Pattern: %d\n", static_cast<int>(currentPattern));
}

//Fuction to display pattern
void currentPatternRun () {
  currentTime = get_millisecond_timer();
  if ( currentTime >= nextChangeTime) {
      PatternResult result = setPattern(currentPattern, leds, NUM_LEDS, currentTime);
      #ifdef QuinLED
        PatternResult result2 = setPattern(currentPattern, leds2, NUM_LEDS2, currentTime);
      #endif
      currentPattern = result.pattern;
      nextChangeTime = result.nextChange;
      esp_task_wdt_reset();
  }
  FastLED.show();
}

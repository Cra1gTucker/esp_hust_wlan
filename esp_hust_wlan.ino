#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>

#include <Regexp.h>
#include "lwip/lwip_napt.h"
#include "lwip/app/dhcpserver.h"

#define prnt Serial.print
#define prntln Serial.println

void Websetup();
void WiFisetup();
String getContentType(String filename);
bool handleFileRead(String path);
void handleConfig();
void handleAp();
void Loginsetup();
String adhocQueryStringProcess(String query);
void datafrommatch(const char *match,const unsigned int length,const MatchState & ms);

ESP8266WebServer server(80);
char eportalURL[50];
char queryString[500];

void setup() {
  Serial.begin(115200);
  delay(100);
  SPIFFS.begin();
  WiFisetup();
  Websetup();
  Loginsetup();
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
}

void WiFisetup(){
  WiFi.mode(WIFI_AP_STA);
  prntln("Starting WiFi interfaces...");
  WiFi.begin("HUST_WIRELESS");
  int i;
  for(i=1;i<=30&&WiFi.status()!=WL_CONNECTED;i++){
    delay(1000);
    prnt(i);
    prnt(' ');
  }
  if(WiFi.status()!=WL_CONNECTED){
    prntln("\nConnection to upstream AP timed out!");
  }
  else{
    prntln("\nConnected to upstream AP.");
    prnt("STA IP address:\t");
    prntln(WiFi.localIP());
  }
  File f = SPIFFS.open("ap_config.txt","r");
  if(f==NULL){
    prntln("Cannot read AP config file! Using default config!");
    WiFi.softAP("ESP8266","8266devkit");
  }
  else{
    String ssidstr = f.readStringUntil('\n');
    char *ssidarr = (char *)malloc(ssidstr.length()+1);
    ssidstr.toCharArray(ssidarr,ssidstr.length()+1);
    ssidarr[ssidstr.length()] = 0;
    String pwstr = f.readStringUntil('\n');
    char *pwarr = (char *)malloc(pwstr.length()+1);
    pwstr.toCharArray(pwarr,pwstr.length()+1);
    pwarr[pwstr.length()] = 0;
    f.close();
    WiFi.softAP(ssidarr,pwarr);
    prntln("AP started.");
    prnt("SSID: ");
    prntln(ssidarr);
    prnt("Password: ");
    prntln(pwarr);
    free(ssidarr);
    free(pwarr);
  }
  prnt("AP IP address:\t");
  prntln(WiFi.softAPIP());
  ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);
  ip_napt_enable_no(1, 1);
  dhcps_set_DNS(WiFi.dnsIP());
}

void Websetup(){
  server.on("/",[](){
    server.sendHeader("Location","/index.html");
    server.send(303);
  });
  server.on("/login.txt",[](){
    server.send(403,"text/plain","403: Forbidden");
  });
  server.on("/config",HTTP_POST,handleConfig);
  server.on("/ap",HTTP_POST,handleAp);
  server.on("/login",[](){
    server.send(200,"text/html","<title>Login</title><h1>Login process triggered.</h1>");
    Loginsetup();
  });
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });
  server.begin();
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  prntln("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
}

void handleConfig(){
  if(!(server.hasArg("user")&&server.hasArg("pass")&&server.arg("user")!=NULL&&server.arg("pass")!=NULL)){
    server.send(400,"text/plain","400: Invalid Request");
  }
  else{
    String user = server.arg("user");
    String pass = server.arg("pass");
    File f = SPIFFS.open("login.txt","w");
    if(f==NULL){
      server.send(200,"text/html","<title>Error</title><h1>Cannot open file for writing!</h1>");
      prntln("Login config save failed!");
      return;
    }
    f.print(user);
    f.print('\n');
    f.print(pass);
    f.print('\n');
    f.close();
    prntln("Login config saved.");
    prnt("user: ");
    prntln(user);
    prnt("encrypted password: ");
    prntln(pass);
    server.send(200,"text/html","<title>Config Saved</title><h1>Configuration Saved</h1>");
  }
}

void handleAp(){
  if(server.hasArg("ssid")&&server.hasArg("pass")&&server.arg("ssid")!=NULL&&server.arg("pass")!=NULL){
    File f = SPIFFS.open("ap_config.txt","w");
    if(f==NULL){
      server.send(200,"text/html","<title>Error</title><h1>Cannot open file for writing!</h1>");
      prntln("AP config save failed!");
      return;
    }
    f.print(server.arg("ssid"));
    f.print('\n');
    f.print(server.arg("pass"));
    f.print('\n');
    f.close();
    prntln("AP config saved.");
    prnt("ssid: ");
    prntln(server.arg("ssid"));
    prnt("password: ");
    prntln(server.arg("pass"));
    server.send(200,"text/html","<title>Config Saved</title><h1>Configuration Saved</h1>");
  }
  else{
    server.send(400,"text/plain","400: Invalid Request");
  }
}

//queryString example(length: 446 chars): wlanuserip=fe7a11a2f7e8500e53ea996ac24eac2e&wlanacname=dfa22de185f6a920bebafa95a8548665&ssid=&nasip=32fa8a3fadb3d1634e008be6991aff23&snmpagentip=&mac=721e722c19bf0a1a821f6594599d8676&t=wireless-v2&url=2c0328164651e2b4f13b933ddf36628bea622dedcc302b30&apmac=&nasid=dfa22de185f6a920bebafa95a8548665&vid=0af4b043da1ac240&port=65184f32a03e53f9&nasportid=763022418fea5ba14e539fb438c3d0237bfc4be455c8b1292d62d872a5afb1032a34d02c7d6cafdfdc62fa8e1c2addf9
void Loginsetup(){
  HTTPClient loginbot;
  prntln("Starting login.");
  loginbot.setTimeout(3000);
  if(loginbot.begin("http://123.123.123.123/")){
    if(loginbot.GET()!=200) {prntln("Got unexpected HTTP response code!");return;}
    else{
      char payload[700];
      loginbot.getString().toCharArray(payload,700);
      MatchState ms (payload);
      prntln("HTTP payload received:");
      prntln(payload);
      if(ms.GlobalMatch("(http://.+eportal/).+%?([^\']+)",datafrommatch)){
        prntln("URL matched:");
        prntln(eportalURL);
        prntln("query string matched:");
        prntln(queryString);
      }
      else {prntln("Regexp match failed!");return;}
    }
  }
  else{
    prntln("Cannot connect to captive portal! Maybe already logged in?");
    return;
  }
  loginbot.end();
  String post_url = (String)eportalURL+(String)"InterFace.do?method=login";
  loginbot.begin(post_url);
  loginbot.addHeader("Content-Type","application/x-www-form-urlencoded",false,true);
  prnt("Post to: ");
  prntln(post_url);
  File f = SPIFFS.open("login.txt","r");
  if(f==NULL){
    prntln("Cannot open login config file!");
    return;
  }
  String user = f.readStringUntil('\n');
  String pass = f.readStringUntil('\n');
  f.close();
  String encodedQuery = adhocQueryStringProcess(queryString);
  //build the post now
  String post_data = "userId="+user+"&password="+pass+"&service=&queryString="+encodedQuery+"&operatorPwd=&validcode=&passwordEncrypt=true";
  String post_response;
  for(byte i=0;i<3;i++){
    loginbot.POST(post_data);
    prntln("POST data:");
    prntln(post_data);
    char post_response[220];
    loginbot.getString().toCharArray(post_response,220);
    prntln("POST response:");
    prntln(post_response);
    MatchState ms1 (post_response);
    if(ms1.GlobalMatch("success",[](const char *match,const unsigned int length,const MatchState & ms){})) break;
    delay(2000);
  }
  loginbot.end();
}

String adhocQueryStringProcess(char *query){
  String encoded = "";
  int i;
  for(i=0;query[i]!='\0';i++){
    if(query[i]=='&') encoded += "%2526";
    else if(query[i]=='=') encoded += "%253D";
    else encoded += query[i];
  }
  return encoded;
}

void datafrommatch(const char *match,const unsigned int length,const MatchState & ms){
  if(ms.level==2){
    ms.GetCapture(eportalURL,0);
    ms.GetCapture(queryString,1);
  }
}

const int totalStations = 8;    // if you add/delete stations change this accordingly

char *host[totalStations] = {
  "mp3.ffh.de", 
  "iphone.live24.gr", 
  "mp3.ffh.de", 
  "mp3.ffh.de", 
  "mp3.ffh.de", 
  "rautemusik-de-hz-fal-stream11.radiohost.de", 
  "stream.srg-ssr.ch", 
  "deluxe.hoerradar.de"
};

char *path[totalStations] = { 
  "/radioffh/hqlivestream.mp3", 
  "/radio998",
  "/ffhchannels/hqlounge.mp3", 
  "/ffhchannels/hqsummerfeeling.mp3", 
  "/ffhchannels/hqvoting.mp3", 
  "/top40", 
  "/m/rsp/mp3_128", 
  "/deluxe-easy-mp3-hq"
};

int port[totalStations] = {
  80, 80, 80, 80, 80, 80, 80, 80
};

char *radioname[totalStations] = {  // Station names to be displayed on OLED (max 13 chars)
  "Hit Radio FFH",
  "   Smooth    ",
  " FFH Lounge  ", 
  " FFH Sommer  ",  
  " Leider Geil ", 
  "   TOP 40    ", 
  "  Swiss Pop  ", 
  " Deluxe Easy "
 };

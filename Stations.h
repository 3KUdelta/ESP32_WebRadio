const int totalStations = 10;    // if you add/delete stations change this accordingly

char *radioname[totalStations] = {  // Station names to be displayed on OLED (max 13 chars)
  "Hit Radio FFH",
  " FFH Lounge  ", 
  " FFH Sommer  ",  
  " Leider Geil ", 
  "  Swiss Pop  ", 
  " Deluxe Easy ",
  "Absolut Chill",
  " Energy Hits ",
  "  Energy Me  ",
  "NRJ on social"
 };

char *host[totalStations] = {
  "mp3.ffh.de", 
  "mp3.ffh.de", 
  "mp3.ffh.de", 
  "mp3.ffh.de", 
  "stream.srg-ssr.ch", 
  "deluxe.hoerradar.de",
  "ais-sa5.cdnstream1.com",
  "energyhits.ice.infomaniak.ch",
  "energymetime.ice.infomaniak.ch",
  "energyspecial2.ice.infomaniak.ch"
};

char *path[totalStations] = { 
  "/radioffh/hqlivestream.mp3", 
  "/ffhchannels/hqlounge.mp3", 
  "/ffhchannels/hqsummerfeeling.mp3", 
  "/ffhchannels/hqvoting.mp3", 
  "/m/rsp/mp3_128", 
  "/deluxe-easy-mp3-hq",
  "/b05055_128mp3",
  "/energyhits-high.mp3",
  "/energymetime-high.mp3",
  "/energyspecial2-high.mp3"
};

int port[totalStations] = {
  80, 80, 80, 80, 80, 80, 80, 80, 80, 80
};

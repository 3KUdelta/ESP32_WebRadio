const int totalStations = 9;    // if you add/delete stations change this accordingly

char *radioname[totalStations] = {  // Station names to be displayed on OLED (max 13 chars)
  "Hit Radio FFH",
  " FFH Lounge  ", 
  " FFH Sommer  ",  
  " Leider Geil ", 
  "  Swiss Pop  ", 
  " Deluxe Easy ",
  "Absolut Chill",
  " Energy Hits ",
  "  Energy Me  "
 };

char *host[totalStations] = {
  "http://mp3.ffh.de/radioffh/hqlivestream.mp3", 
  "http://mp3.ffh.de/ffhchannels/hqlounge.mp3", 
  "http://mp3.ffh.de/ffhchannels/hqsummerfeeling.mp3", 
  "http://mp3.ffh.de/ffhchannels/hqvoting.mp3", 
  "https://stream.srg-ssr.ch/m/rsp/mp3_128", 
  "http://deluxe.hoerradar.de/deluxe-easy-mp3-hq",
  "http://ais-sa5.cdnstream1.com/b05055_128mp3",
  "http://energyhits.ice.infomaniak.ch/energyhits-high.mp3",
  "http://energymetime.ice.infomaniak.ch/energymetime-high.mp3"
};


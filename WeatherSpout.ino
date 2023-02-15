#include "bitmaps.h"
#include "weatherbm.h"
#include "Free_Fonts.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <Bme280.h>
#include <TimeLib.h>
#include <wordwrap.h>



const char* ssid = "";
const char* password = "";
String my_Api_Key = "";
String my_latitude = "";
String my_longitude = "";


#define SW1 6     // Switch 1 calls up the 6 day forecast
#define SW2 18    // Switch 2 shows the next screen

// There are 5 screens. The Alert screen only shows if there's an alert. 
// We're calling NUM_SCREENS 4 instead of 5 because the 5th only shows when
// Switch 1 is pressed.
//
// If additional screens are added, it is sufficient just to increase
// NUM_SCREENS. No other adjustment for the 6-day screen is needed.
//
#define NUM_SCREENS 4

String exclude_str = "hourly,minutely";

String g_json_array;
JSONVar g_my_obj;
Bme280TwoWire sensor;

const uint32_t g_normalDelay = 5000;
const uint32_t g_longDelay = 15000;
const uint32_t g_weatherDelay = 3600 * 1000;  // 1 hour

uint32_t g_CycleTime, g_CheckWeatherTime;
uint32_t g_CycleTimeout, g_CheckWeatherTimeout;
int g_curState;
double g_baroPressure;
double g_lastPressure[3];

uint16_t gradientBtoR[] = { 0x001F, 0x281A, 0x6013, 0xA00B, 0xD804, 0xF800 };

bool getWeather( void );

TFT_eSPI tft = TFT_eSPI();  //pins defined in User_Setup.h

void setup() {
  g_CycleTime = millis();
  g_CycleTimeout = g_normalDelay;
  g_CheckWeatherTime = millis();
  g_CheckWeatherTimeout = g_weatherDelay;
  g_curState = 0;
  g_lastPressure[0] = 0;
  g_lastPressure[1] = 0;
  g_lastPressure[2] = 0;
  g_baroPressure = 0;
  
  pinMode(SW1, INPUT_PULLDOWN);
  pinMode(SW2, INPUT_PULLDOWN);
  Serial.begin(9600);
  Wire.begin();
  sensor.begin(Bme280TwoWireAddress::Primary);
  sensor.setSettings(Bme280Settings::indoor());
  tft.init();
  tft.setRotation(3);
}

void loop() 
{
  static int triggered = 0;
  static int sw1_laststate = LOW;
  static int sw2_laststate = LOW;
  int buttonstate;
  
  show_wifi_setup();

  g_CheckWeatherTimeout = getWeather() ? g_weatherDelay : g_normalDelay;

  while( true )
  {
    if( millis() - g_CheckWeatherTime >= g_CheckWeatherTimeout )
    {
      Serial.println( "Getting New Weather" );
      g_CheckWeatherTime = millis();
      
      g_CheckWeatherTimeout = getWeather() ? g_weatherDelay : g_normalDelay;
    }

    if( millis() - g_CycleTime >= g_CycleTimeout )
    {
      g_CycleTime = millis();
      g_CycleTimeout = g_normalDelay;
      triggered = SW2;
    }

    buttonstate = digitalRead( SW1 );
    
    if( sw1_laststate == HIGH && buttonstate == LOW )
    {
      g_CycleTime = millis();
      g_CycleTimeout = g_longDelay;
      triggered = SW1;
    }

    sw1_laststate = buttonstate;
    
    buttonstate = digitalRead( SW2 );
    
    if( sw2_laststate == HIGH && buttonstate == LOW )
    {
      g_CycleTime = millis();
      g_CycleTimeout = g_longDelay;
      triggered = SW2;
    }

    sw2_laststate = buttonstate;

    if( triggered )
    {
      if( triggered == SW1 )
      {
        g_curState = NUM_SCREENS;
      }
      else if( triggered == SW2 )
      {
        g_curState++;

        if( g_curState >= NUM_SCREENS )
          g_curState = 0;
      }

      triggered = 0;

      switch( g_curState )
      {
        case 0:
          show_BME280_measurements();
          break;

        case 1:
          show_CurrentWeather();
          break;

        case 2:
          show_TodayForecast();
          break;

        case 3:
          if( JSON.typeof( g_my_obj["alerts"] ) != "null" )
            show_Alert();
          else
            g_CycleTimeout = 0;   // We shouldn't be here. Timeout immediately to cycle to next screen
            
          break;

        case NUM_SCREENS:
          show_WeekForecast();
          break;
      }
    }
  }
}

bool getWeather( void )
{
  JSONVar newObj;
  
  if (WiFi.status() == WL_CONNECTED) 
  {
    String server = "http://api.openweathermap.org/data/3.0/onecall?lat=" + my_latitude + "&lon=" + my_longitude + "&exclude=" + exclude_str + "&units=imperial&appid=" + my_Api_Key;
    g_json_array = GET_Request(server.c_str());

//    Serial.println(g_json_array);

    // Get the JSON data into a temporary object, so if there's an error, we can
    // keep displaying the old data until its fixed
    //
    newObj = JSON.parse(g_json_array);
    
    if (JSON.typeof(newObj) == "undefined") 
    {
      Serial.println("Parsing input failed!");
      return( false );
    }

    // The JSON data is good, replace the old data now
    //
    g_my_obj = newObj;

    Serial.println("JSON object = ");
    Serial.println(g_my_obj);

    // We have fresh barometric pressure. Remember the last
    // reading, too.
    //
    g_lastPressure[2] = g_lastPressure[1];
    g_lastPressure[1] = g_lastPressure[0];
    g_lastPressure[0] = g_baroPressure;
    g_baroPressure = double( g_my_obj["current"]["pressure"] );
  }
  else 
  {
    Serial.println("WiFi Disconnected");

    // Ouch. Just tear everything down and try anew
    //
    WiFi.end();
    show_wifi_setup();
    return( false );
  }

  return( true );
}


String GET_Request(const char* server) 
{
  HTTPClient http;
  http.begin(server);
  int httpResponseCode = http.GET();
  String payload = "{}";

  
  if (httpResponseCode > 0) 
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else 
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
  return payload;
}

const unsigned char *getWeatherIcon( int code )
{
  if( code >= 200 && code <= 299 )  // Thunderstorms
  {
    if( code == 200 || code == 210 || code == 230 )
      return( xbm_thunder_lt );

    return( xbm_thunder );
  }
  
  if( code >= 300 && code <= 399 )  // Drizzle
    return( xbm_rain_lt );

  if( code >= 500 && code <= 599 )  // Rain
  {
    if( code == 500 || code == 520 )
      return( xbm_rain_lt );

    if( code == 511 )
      return( xbm_rain_freezing );

    return( xbm_rain );
  }

  if( code >= 600 && code <= 699 )  // Snow
  {
    if( code == 600 || code == 612 || code == 615 || code == 620 )
      return ( xbm_snow_lt );

    if( code == 602 || code == 622 )
      return( xbm_snow_hvy );

    return( xbm_snow );
  }

  if( code >= 700 && code <= 799 )  // Atmosphere
  {
    if( code == 701 || code == 711 || code == 731 )
      return( xbm_haze_lt );

    return( xbm_haze );
  }

  if( code >= 800 && code <= 899 )  // Clouds
  {
    if( code == 800 )
      return( xbm_clear );

    if( code == 801 )
      return( xbm_clouds_lt );

    if( code == 804 )
      return( xbm_clouds_hvy );

    return( xbm_clouds );
  }

  return( NULL );
}


uint16_t get_TemperatureColor( double temperature )
{
  uint16_t temperatureColor = (uint16_t)( ( temperature - 32.0 ) / 11.3 );
  
  if( temperatureColor < 0 )  temperatureColor = 0;
  if( temperatureColor > 5 )  temperatureColor = 5;

  return( gradientBtoR[ temperatureColor ] );
}


uint16_t get_UVIColor( double uvi )
{
  if( uvi < 2.5 )
    return( TFT_GREEN );

  if( uvi < 5.5 )
    return( TFT_YELLOW );

  if( uvi < 7.5 )
    return( TFT_ORANGE );

  if( uvi < 10.5 )
    return( TFT_RED );

  return( TFT_VIOLET );
}



String get_CardinalDir( int degree )
{
  if( degree <= 11 || degree > 349 )
    return( String( "N" ) );

  if( degree <= 34 )
    return( String( "NNE" ) );

  if( degree <= 56 )
    return( String( "NE" ) );

  if( degree <= 79 )
    return( String( "ENE" ) );

  if( degree <= 101 )
    return( String( "E" ) );

  if( degree <= 124 )
    return( String( "ESE" ) );

  if( degree <= 146 )
    return( String( "SE" ) );

  if( degree <= 169 )
    return( String( "SSE" ) );

  if( degree <= 191 )
    return( String( "S" ) );

  if( degree <= 214 )
    return( String( "SSW" ) );

  if( degree <= 236 )
    return( String( "SW" ) );

  if( degree <= 259 )
    return( String( "WSW" ) );

  if( degree <= 281 )
    return( String( "W" ) );

  if( degree <= 304 )
    return( String( "WNW" ) );

  if( degree <= 326 )
    return( String( "NW" ) );

  if( degree <= 349 )
    return( String( "NNW" ) );

  return( String( "" ) );
}


// future should be false if we're showing current conditions,
// and true otherwise
//
void show_WeatherBlock( const JSONVar &data2, bool future )
{
  String workStr;
  uint16_t temperatureColor;
  int ypos, i;
  JSONVar data = data2;
  double testPressure;


  if (JSON.typeof(g_my_obj) == "undefined") 
    return;


  tft.setTextDatum( TC_DATUM );

  if( future )
    temperatureColor = get_TemperatureColor( double( data["temp"]["day"] ) );
  else
    temperatureColor = get_TemperatureColor( double( data["temp"] ) );


  //------------------------------------- Display the icon and description

  tft.setTextColor( temperatureColor );
  tft.drawXBitmap(110,20, getWeatherIcon( data["weather"][0]["id"] ), weather_width, weather_height, temperatureColor );

  workStr = JSON.stringify( data["weather"][0]["description"] );
  workStr.remove( workStr.indexOf( '\"' ), 1 );     // Remove the damned initial quote mark
  workStr.remove( workStr.lastIndexOf( '\"' ), 1 ); // Remove the damned trailing quote mark

  // If we're showing a "cloud" type, include the cloud cover percentage
  //
  if( int( data["weather"][0]["id"] ) >= 800 && int( data["weather"][0]["id"] ) <= 899 )
    workStr = workStr + " (" + JSON.stringify( data["clouds"] ) + "%)";

  ypos = 120;
  tft.drawString( workStr, 160, ypos, FONT4 );
  ypos += tft.fontHeight( FONT4 );
  ypos += 10;

  // Show the temperature line
  //
  tft.setTextColor(TFT_YELLOW);
  tft.setTextDatum( TL_DATUM );
  tft.setFreeFont(&FreeSans12pt7b);

  if( future )
  {
    workStr = "High: " + JSON.stringify(data["temp"]["max"]) + " F";
    tft.drawString(workStr, 10, ypos, GFXFF);
    tft.setTextDatum( TR_DATUM );
    workStr = "Low: " +  JSON.stringify(data["temp"]["min"]) + " F";
    tft.drawString(workStr, 320, ypos, GFXFF);
  }
  else
  {
    workStr = JSON.stringify(data["temp"]) + " F";
    tft.drawString(workStr, 10, ypos, GFXFF);
    tft.setTextDatum( TR_DATUM );
    workStr = "(feels like " + JSON.stringify(data["feels_like"]) + " F)";
    tft.drawString(workStr, 320, ypos, GFXFF);
  }
  
  tft.setTextDatum( TL_DATUM );
  ypos += tft.fontHeight(GFXFF);


  // -------------------------------------Show the wind Line
  //
  tft.setTextColor(TFT_GREEN);
  tft.setFreeFont(&FreeSans9pt7b);
  workStr = "RH: " + JSON.stringify(data["humidity"]) + "%";
  tft.drawString(workStr, 10, ypos, GFXFF);
  int wspeed = int(data["wind_speed"]);
  int wgust = int(data["wind_gust"]);
  workStr = "Wind: " + String( wspeed ) + " MPH (" + String( wgust ) + ") " + get_CardinalDir( int( data["wind_deg"] ) );
  tft.setTextDatum( TR_DATUM );
  tft.drawString(workStr, 320, ypos, GFXFF);
  
  ypos += tft.fontHeight(GFXFF);
  tft.setTextDatum( TL_DATUM );

  //------------------------------------- Show the bottom line
  //
  tft.setTextColor(TFT_GREEN);

  // Dew point. If the temp of interest is below the dew point, color
  // the dew point red
  //
  if( !future )
  {
    if( double( data["temp"] ) <= double( data["dew_point"] ) )
      tft.setTextColor(TFT_RED);
  }
  else
  {
    if( double( data["temp"][ "min" ] ) <= double( data["dew_point"] ) )
      tft.setTextColor(TFT_RED);
  }
  
  workStr = "Dew: " + JSON.stringify(data["dew_point"]) + " F";
  tft.drawString(workStr, 10, ypos, GFXFF);

  // Barometric pressure. If the barometric pressure is falling, make this red
  //                      Otherwise, green.
  
  tft.setTextColor(TFT_BLUE);

  if( !future )
  {
    bool neg;
    double amt;
    uint16_t backColor;

    
    if( g_lastPressure[0] != 0 ) // use the blue if not yet valid
    {
      for( i = 2; i >= 0; i-- )
      {
        testPressure = g_lastPressure[i];
        if( testPressure )
          break;
      }
      
      neg = g_baroPressure < testPressure ? true : false;
      amt = abs( g_baroPressure - testPressure );

      if( g_baroPressure > 1022.68 )
        backColor = 0x6000;
      else if( g_baroPressure > 1009.14 )
        backColor = 0x0000;
      else
        backColor = 0x0300;
      
      
      if( amt > 6.09 )
      {
        tft.setTextColor( neg ? TFT_RED : TFT_GREEN, backColor );
      }
      else if( amt > 1.35 )
      {
        tft.setTextColor( neg ? TFT_MAROON : TFT_DARKGREEN, backColor );
      }
    }

    // Use the blue if the pressure hasn't changed (default)
  }

  workStr = "Prs: " + JSON.stringify(data["pressure"]) + " hPa";
  tft.drawString(workStr, 120, ypos, GFXFF);

  // UV Index. Color it according to the standard indicator colors
  //
  tft.setTextDatum( TR_DATUM );
  tft.setTextColor( get_UVIColor( double( data["uvi"] ) ) );
  workStr = "UVI: " + JSON.stringify(data["uvi"]);
  tft.drawString(workStr, 320, ypos, GFXFF);
}


void show_wifi_setup()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Wi-Fi...");
  tft.setTextDatum( CC_DATUM );
  tft.drawString("Connecting to Wi-Fi...", 160, 120, 4);

  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("Wi-Fi Not Connected.");
    delay(1000);
  }
  
  Serial.println("");
  Serial.println("Wi-Fi Connected.");
  Serial.print("IP:");
  Serial.println(WiFi.localIP());
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Wi-Fi Connected.", 160, 120, 4);
}



void show_BME280_measurements()
{
  tft.fillScreen(TFT_BLACK);
  tft.drawXBitmap(27, 50, temp, iconWidth, iconHeight, TFT_RED);
  tft.drawXBitmap(27, 105, humidity, iconWidth, iconHeight, TFT_BLUE);
  tft.drawXBitmap(27, 160, pressure, iconWidth, iconHeight, TFT_YELLOW);
  
  float tempC = sensor.getTemperature();
  auto temperatureC = String(tempC) + " C";
  float tempF = ((tempC * 9) / 5) + 32;
  auto temperatureF = String(tempF) + " F";
  auto pressure = String(sensor.getPressure() / 100.0) + " hPa";
  auto humidity = String(sensor.getHumidity()) + " %";

  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum( TC_DATUM );
  tft.drawString("Indoor Conditions", 160, 10, FONT4);
  tft.setTextDatum( TL_DATUM );
  tft.setTextColor(TFT_RED);
  String measurements = temperatureC + " / " + temperatureF;
  tft.drawString(measurements, 95, 65, FONT4);
  tft.setTextColor(TFT_BLUE);
  tft.drawString(humidity, 95, 120, FONT4);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(pressure, 95, 175, FONT4);
}


void show_Alert()
{
  int ypos = 15;
  char timedate[ 80 ];
  String workStr, newStr, finalStr;
  int charIndex, lastIndex;
  bool firstLine = true;
  
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_BLACK);
  tft.setTextDatum( TL_DATUM );

//  Serial.println( "event: " + JSON.stringify( g_my_obj[ "alerts" ][0][ "event" ] ) );
//  Serial.println( "start: " + JSON.stringify( g_my_obj[ "alerts" ][0][ "start" ] ) + " end: " + JSON.stringify( g_my_obj[ "alerts" ][0][ "end" ] ) );
//  Serial.println( "description: " + JSON.stringify( g_my_obj[ "alerts" ][0][ "description" ] ) );
  
  tft.setCursor( 10, ypos );
  tft.setTextFont(2);

  newStr = JSON.stringify( g_my_obj[ "alerts" ][0][ "description" ] );

  newStr.remove( newStr.indexOf( '\"' ), 1 );     // Remove the damned initial quote mark
  newStr.remove( newStr.lastIndexOf( '\"' ), 1 ); // Remove the damned trailing quote mark

  // Go through the string and skip the literal "\n" sequences. Not a newline, a '\' followed
  // by 'n'.
  //
  // If the "\n" is followed by an asterisk, then insert an actual newline.

  charIndex = 0;

  // Find the first "\n"
  //
  while( charIndex != -1 )
  {
    if( ( charIndex = newStr.indexOf( '\\', charIndex ) ) != -1 )
    {
      if( newStr.charAt( charIndex + 1 ) != 'n' )
        charIndex++;
      else
        break;
    }
  }

  // charIndex is now pointing at the start of the "\n" (or is -1)

  lastIndex = 0;  // The start of the segment we're printing
  finalStr = "";

  while( charIndex != -1 )
  {
    workStr = newStr.substring( lastIndex, charIndex );

    // Now we have the string segment. We need to peek ahead past
    // the "\n" to see if there's an asterisk. If there is, we need
    // to newline
    //
    if( newStr.charAt( charIndex + 2 ) == '*' )
    {
      finalStr = finalStr + workStr;

      if( firstLine )
      {
        tft.println( wordwrap.wrap( finalStr, 42 ) );
        firstLine = false;
      }
      else
        tft.println( wordwrap.wrap( finalStr, 46 ) );
      
      finalStr = "";
    }
    else
    {
      // We didn't see the asterisk, so no newline. Just
      // append the string segment and move on without
      // printing
      //
      finalStr = finalStr + workStr + " ";
    }
      
    lastIndex = charIndex + 2;  // Skip past the "\n"

    // Find the next "\n"
    //
    while( charIndex != -1 )
    {
      if( ( charIndex = newStr.indexOf( '\\', lastIndex ) ) != -1 )
      {
        if( newStr.charAt( charIndex + 1 ) == 'n' );
          break;
      }
    }

    if( charIndex == -1 ) // No more string. Print what we got
    {
      workStr = newStr.substring( lastIndex );
      finalStr = finalStr + workStr + " ";
      tft.print( wordwrap.wrap( finalStr, 46 ) );
    }
  }
}


void show_CurrentWeather()
{
  String workStr;
  uint16_t temperatureColor, timeDiff;
  int ypos;
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum( TC_DATUM );


  // How many minutes since we last updated the weather?
  //
  timeDiff = ( millis() - g_CheckWeatherTime ) / ( 1000 * 60 ); // Convert to minutes

  if( !timeDiff )
    tft.drawString("Current Conditions", 160, 7, FONT4);
  else
  {
    workStr = "Conditions " + String( timeDiff ) + " Min" + ( timeDiff != 1 ? "s" : "" ) + " Ago";
    tft.drawString( workStr, 160, 7, FONT4 );
  }

#if 0
  Serial.print("Temperature: ");
  Serial.println(g_my_obj["current"]["temp"]);
  Serial.print("Humidity: ");
  Serial.println(g_my_obj["current"]["humidity"]);
  Serial.print("Pressure: ");
  Serial.println(g_my_obj["current"]["pressure"]);
  Serial.print("Wind Speed: ");
  Serial.println(g_my_obj["current"]["wind_speed"]);
  Serial.print("Weather code: " );
  Serial.print( g_my_obj["current"]["weather"][0]["id"] );
  Serial.print( ", " );
  Serial.println( g_my_obj["current"]["weather"][0]["description"] );
#endif

  show_WeatherBlock( g_my_obj["current"], false );
}

void show_TodayForecast()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum( TC_DATUM );
  tft.drawString("Today's Forecast", 160, 7, 4);
  tft.setTextDatum( TL_DATUM );  

#if 0
  Serial.print("TF Temperature: ");
  Serial.println(g_my_obj["daily"][0]["temp"]["day"]);
  Serial.print("TF Humidity: ");
  Serial.println(g_my_obj["daily"][0]["humidity"]);
  Serial.print("TF Pressure: ");
  Serial.println(g_my_obj["daily"][0]["pressure"]);
  Serial.print("TF Wind Speed: ");
  Serial.println(g_my_obj["daily"][0]["wind_speed"]);
  Serial.print("Weather code: " );
  Serial.print( g_my_obj["daily"][0]["weather"][0]["id"] );
  Serial.print( ", " );
  Serial.println( g_my_obj["daily"][0]["weather"][0]["description"] );
#endif

  show_WeatherBlock( g_my_obj["daily"][0], true );
}

void show_WeekForecast()
{
  String workStr;
  uint16_t temperatureColor;
  int i, x, y;
  int min_temp, max_temp;

  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum( TC_DATUM );
  tft.drawString("6 Day Forecast", 160, 7, 4);

  
  tft.setFreeFont(&FreeMonoBold9pt7b);

  for( i = 0; i < 6; i++ )
  {
#if 0
    Serial.print("TF Temperature: ");
    Serial.println(g_my_obj["daily"][i+1]["temp"]["day"]);
    Serial.print("Weather code: " );
    Serial.print( g_my_obj["daily"][i+1]["weather"][0]["id"] );
    Serial.print( ", " );
    Serial.println( g_my_obj["daily"][i+1]["weather"][0]["description"] );
#endif

    x = ( 110 * ( i < 3 ? i : i - 3 ) );
    y = i < 3 ? 30 : 120;

    temperatureColor = get_TemperatureColor( double( g_my_obj["daily"][i+1]["temp"]["day"] ) );

    tft.setTextColor( temperatureColor );
    tft.drawXBitmap(x,y, getWeatherIcon( g_my_obj["daily"][i+1]["weather"][0]["id"] ), weather_width, weather_height, temperatureColor );

    min_temp = int( g_my_obj["daily"][i+1]["temp"]["min"] );
    max_temp = int( g_my_obj["daily"][i+1]["temp"]["max"] );
    workStr = String( max_temp ) + "/" +  String(  min_temp );
    tft.drawString(workStr, x + 50, y + 80, GFXFF);
  }
}

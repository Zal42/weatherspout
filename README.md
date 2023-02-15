# weatherspout
Enhanced software for Hackerbox 87 Weather Station

![pic1](/pics/pic1.jpg) ![pic2](/pics/pic2.jpg) ![pic3](/pics/pic3.jpg) ![pic4](/pics/pic4.jpg)

This is intended to run on the weather station that is in [Hackerbox 87](https://hackerboxes.com/products/hackerbox-0087-picow). Please see [the instructions](https://www.instructables.com/HackerBox-0087-Picow/) for programming information. This software is based on WeatherStation.ino and replaces it.

The left button calls up a 6 day forecast, the right button cycles to the next page.

Features:
  * A special page is added to the rotation to display weather alerts when they happen
  * The weather icon and descriptive text changes color in 6 steps from blue to red to roughly indicate temperature
  * The Dew Point text turns red if the temperature is at or below the dew point
  * The Atmospheric Pressure text is color coded:
    * Black background means that the pressure is normal
    * Red background means that the pressure is low (below 1009.14 hPa)
    * Green background means that the pressure is high (above 1022.68 hPa)
    * Blue text means the pressure is holding steady
    * Dark green text means the pressure is trending higher
    * Green text means the pressure is rising rapidly
    * Dark red text means the pressure is trending lower
    * Red text means the pressure is falling rapidly
    * NOTE: This text will be Blue on Black until there is enough data to detect trends (2 weather checks, or 1 hour)
  * The UV Index is color coded according to [the standard risk colors](https://www.epa.gov/sunsafety/uv-index-scale-0).

The images used are from the [Hackerbox project](https://www.instructables.com/HackerBox-0087-Picow/) and from [FreePik](https://www.freepikcompany.com/).

This code is designed to use openweathermap.org's [One Call API 3.0](https://openweathermap.org/api/one-call-3)

Don't forget to edit WeatherSpout.ino to add the details for your WiFi AP, OpenWeather credentials, and your location.

Arduino libraries required:

 * TFT_eSPI [See configuration instructions](https://hackerboxes.com/products/hackerbox-0087-picow)
 * Bme280 by Eduard Malokhvii.
 * Arduino_JSON by Arduino
 * Time by Paul Stoffregen
 * wordwrap by Nick Reynolds

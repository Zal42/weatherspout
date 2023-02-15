# weatherspout
Enhanced software for Hackerbox 87 Weather Station

This is intended to run on the weather station that is in Hackerbox 87 (https://hackerboxes.com/products/hackerbox-0087-picow). Please see https://www.instructables.com/HackerBox-0087-Picow/ for programming instructions. This software is based on WeatherStation.ino and replaces it.

The images used are from the Hackerbox's project and from FreePik (https://www.freepikcompany.com/).

This code is designed to use openweathermap.org's Once Call API 3.0

Don't forget to edit weatherspout.ino to add the details for your WiFi AP, OpenWeather credentials, and your location.

Arduino libraries required:

TFT_eSPI (See https://hackerboxes.com/products/hackerbox-0087-picow for configuration instructions)
Bme280 by Eduard Malokhvii.
Arduino_JSON by Arduino
Time by Paul Stoffregen
wordwrap by Nick Reynolds

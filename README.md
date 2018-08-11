# Weatherstation

A simple weatherstation connected to Thingspeak using the ESP8266 and BME280 multi-sensor. 

The weatherstation boots itself up between given intervals, reads the sensor values, and reports them to Thingspeak. 

# YR.no
The software tries to connect to YR.no (weather forecasting service) to pull the latest pressure values from the given area - this was done with the hopes that this pressure is defined at sea-level, but im not quite sure. 
The reason for getting this value, is so to test out the calculation for meters above sealevel. 
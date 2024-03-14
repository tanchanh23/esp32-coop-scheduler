# esp32-coop-scheduler
 ESP32 based schedule controller  

# Logic Functionalities
- ESP32 AP/STA mode
- Webservice/Pages(Bootstrap4 + Jquery) to configure WiFi credential and MQTT Broker, client identifier
- Adjusting schedules and testing hardware components on the page
- Receive commands from cloud server to override schedule and actuators 
- Sending event notifications to cloud server via MQTT

# Hardware Components
- ESP32 Dev KitC
- DS3231 RTC module
- Light level sensor
- 12V Motor driver module
- 4 of Mosfets & 2 proximity sensors
- 1 of button switch
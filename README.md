System based on ESP32-wroom to to warn when the postman drops off a letter or a package.
The code published in mqtt.
The system also monitors the temperature, humidity, battery, solar panel charge, currents, RRSI, IP address.
Communication is done via wifi.

The sensors boards are:
2 x INA219 boards for current measure (each sensor must have a different adress in code)
1 x DHT22 board for temperature and humidity 
2 x Magnetics ILS switchs
2 x Battery 16650
1 x ESP32 30 pins version
1 x board 18650-Lithium charger
1 x solar panel 5V


System based on ESP32-wroom to to warn when the postman drops off a letter or a package.
The code published in mqtt.
The system also monitors the temperature, humidity, battery, solar panel charge, currents, RRSI, IP address.
Communication is done via wifi. The code includes a deep sleep function.

The sensors boards are:
2 x INA219 boards for current measure (each sensor must have a different adress in code)
1 x DHT22 board for temperature and humidity 
2 x Magnetics ILS switchs
2 x Battery 16650
1 x ESP32 30 pins version  with antenna modification to increase wifi range
1 x board 18650-Lithium charger
1 x solar panel 5V
1 x regulator Lowdrop 3,3V
2 x resistors 1K switch pull up

Here prototype.

![IMG_8351](https://github.com/user-attachments/assets/60a31244-e1a2-4c79-baa5-33fd4c6e503e)

Antenna modification:

![sma soudée](https://github.com/user-attachments/assets/e3c38ca4-8527-49a5-a807-65e3b5092613)


Mqtt publish:

<img width="183" alt="Capture d’écran 2024-12-24 à 21 27 17" src="https://github.com/user-attachments/assets/04fd6b65-55a6-476b-b858-d97a1729d2ee" />

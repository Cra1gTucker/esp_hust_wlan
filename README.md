# esp_hust_wlan
Automated login sketch for HUST_WIRELESS with NAT on ESP8266.

**Warning**: Might not work as of 2020-9-29 due to a change in decrytion method. I will not fix this.

Depends on:
* [martin-ger's lwip implementation](https://github.com/martin-ger/lwip_nat_arduino) for NAT feature
* [arduino-esp8266fs-plugin](https://github.com/esp8266/arduino-esp8266fs-plugin) for data upload
Also the RSA encryption JavaScript files are copied from the login portal, which are actually copied from [ohdave.com](http://ohdave.com/rsa/) and under open source license.

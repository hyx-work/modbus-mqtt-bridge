# modbus-mqtt-bridge
Manage the Modbus RTU slave through MQTT Broker.

For the configuration file, please check with `config.json`.

## Requirements
* [**libmosquitto**](https://mosquitto.org/man/libmosquitto-3.html) - MQTT client library
* [**libmodbus**](http://libmodbus.org/) - Modbus library
* **GCC + Make** - GNU C Compiler and build automation tool

## Compile and Run

```bash
make
./bridge-mb-mqtt
or
./bridge-mb-mqtt-c path/to/config/jsfile
```


## Refer
1. https://github.com/nskygit/mqtt4modbus

如果本项目对您有所帮助，请打赏在下，不胜感激^_^

**打赏**

![pay](https://github.com/hyx-work/modbus-mqtt-bridge/raw/master/img/pay.png)

**本人微信**

![image](./img/wx.jpg)

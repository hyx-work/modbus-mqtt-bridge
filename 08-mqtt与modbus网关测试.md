## 0 modbus slave 模拟器

.\diagslave.exe -m rtu -b 9600 -d 8 -s 1 -p none -a 1 COM3

## 1 读测试

### function code 0x01
```bash
mosquitto_pub -h localhost -p 1883 -t modbus/homeassistant/switch/READ_COILS/state -m a
```
###  function code 0x02
```bash
mosquitto_pub -h localhost -p 1883 -t modbus/homeassistant/switch/READ_DISCRETE_INPUTS/state -m a
```
###  function code 0x03
```bash
mosquitto_pub -h localhost -p 1883 -t modbus/homeassistant/register/READ_HOLDING_REGISTERS/state -m a
```
###  function code 0x04
```bash
mosquitto_pub -h localhost -p 1883 -t modbus/homeassistant/register/READ_INPUT_REGISTERS/state -m a
```

## 2 写测试

### function code 0x05
```bash
mosquitto_pub -h localhost -p 1883 -t homeassistant/switch/WRITE_SINGLE_COIL/set -m ON
```
### function code 0x0F
```bash
mosquitto_pub -h localhost -p 1883 -t homeassistant/switch/WRITE_MULTIPLE_COILS/set -m OFF,OFF,ON,OFF,OFF,ON,ON,OFF,OFF,ON
```
### function code 0x06
```bash
mosquitto_pub -h localhost -p 1883 -t homeassistant/register/WRITE_SINGLE_REGISTER/set -m {"something":-10.1}
```
### function code 0x10
```bash
mosquitto_pub -h localhost -p 1883 -t homeassistant/register/WRITE_MULTIPLE_REGISTERS/set -m [{"something":-10.2},{"something":-10.3},{"something":-10.4}]
```
.
├── platformio.ini
├── boards
    ├── [board]
    │   └── interface.cpp
    ├── pinouts
    │   ├── pins_arduino.h
    │   └── [board].h
    ├── [board].json
    └── [board].ini
...
```

# Arquivos
(Substitua [board] pelo nome da placa)

## boards/pinouts/pins_arduio.h
Aqui é onde você colocará a flag que incluirá o cabeçalho de pinagem da sua placa.

## boards/pinouts/[board].h
Aqui é onde você coloca as flags e pinagens da placa. Veja outras placas para entender o que é necessário.
Aqui está um exemplo oficial e o que estamos realmente usando aqui:
https://github.com/espressif/arduino-esp32/blob/master/variants/esp32s3/pins_arduino.h

## boards/[board]/interface.cpp
Aqui é onde você faz o código de configuração específico da placa

## boards/[board].json
Esta é a configuração da placa. Veja outras placas para entender o que é necessário.
Aqui está um exemplo oficial e o que estamos realmente usando aqui:
https://github.com/platformio/platform-espressif32/blob/master/boards/esp32-s3-devkitc-1.json

## boards/[board].ini
Esta é a configuração do platformio para o dispositivo. Veja outras placas para entender o que é necessário.
#include <Arduino.h>
#include "apwifiesp32.h"

void setup() {

initAP("espAP","123456789");//nombre de wifi a generarse y contrasena
}

void loop() {
    loopAP();
}

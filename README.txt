# La caja
## Requisitos
Todos los parámetros deberán ser configurables mediante el uso de una API
### Función dormir
Deberá gestionar la intensidad y color de un led RGB de forma que induzca al sueño. El led irá perdiendo intensidad hasta aapagarse.

### Función despertar
Al igual que la función dormir, deberá gestionar intensidad y color del led RGB de forma que induzca a despertar. El led se encenderá 1 hora antes e irá ganando intensidad hasta la hora prevista de despertar. 20 minutos antes de la hora prevista sonará un buzzer (un pequeño toque; cada cinco minutos, irá incrementándose duración hasta llegar a la hora ). Una vez llegado a ese punto el led parpadeará a máxima intensidad y alternando colores molestos junto con el sonido del buzzer.
Tiempos y colores deberán ser personalizables.

### Función ver la hora
Un sensor PIR detectará el movimiento (pasar la mano por encima) y mostrará la hora.
También:
  - reiniciará la función dormir 
  - si ha llegado la hora de despertar, detendrá el buzzer durante 5 minutos, pero en ningún caso se podrá desactivar
Tiempos y colores deberán ser personalizables

## Componentes 
- Microcontrolados ESP8266 (nodemcu)
- Led RGB
- Pantalla + I2C
- Reloj  ZS-042. No es necesario ya que tendrá conectividad a internet
- Sensor PIR o Ultrasonidos
- Buzzer 
- Batería lipo
- Cargador de batería
. KA2288 Indicador de batería

## API
El microcontrolador ofrecerá una API para gestionar:
- Establecer Fecha y hora
- Establecer Hora de la alarma
- Para el led en modo dormir
  - RBG con el color para dormir
  - Cadencia de dicho color (incrementar / decrementar luminiosidad) 
  - Tiempo total que durará el modo dormir
- Para el led en modo despertar
  - RGB con el color para despertar 
  - Cadencias de dicho color (incrementar / decrementar luminosidad)
  - Tiempo de encendido del led (una hora, por ejemplo)
  - Incrementar intensidad
  - Cada cuanto tiempo se activa el buzzer si es hora (20 minutos antes de la alarma


## TODO
### Despertar 

    Rojo/ámbar: al inicio; es suave y poco molesto.
    Naranja: durante la transición.
    Amarillo cálido o blanco cálido: al acercarse la hora de despertar; ayuda a activar el organismo.
    Blanco azulado o azul intenso: mejor evitarlo al principio, porque puede resultar demasiado estimulante y deslumbrante.

Configúralo para que aumente lentamente durante 20–30 minutos, con brillo moderado. Si tienes mucha dificultad para despertarte, puedes terminar con una luz blanca cálida más intensa.

### Dormir
Rojo oscuro o ámbar, naranja rojizo o rojo tenue
20-45 minutos

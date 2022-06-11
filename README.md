# TD3Notas
Ejemplos para TD3

El PDF contiene la presentacion.

En Timer Mal Ejemplo usamos el contador del TIM1 con un ciclo incompleto (prescaler a 64000 pero el contador esta limitado en 250). Esto no es lo idea ya que el timer queda limitado a contar este valor y desperdiciamos los canales.

En Timer Con Output Compare dejamos el contador de TIM1 free running, y con el Canal 1 configurado en Output Compare vamos cambiando el valor de comparacion para que cada 250 cuentas haya una interrupcion. Habilitamos que el PIN T1C1 haga Toggle en cada comparacion.

En Uart Bloqueante enviamos datos de forma bloquante por la UART.

En I2CDisplay utilizamos un display LCD 4x20 conectado por I2C utilizando el chip I2C-GPIO.


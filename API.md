# API — La caja

Servidor HTTP (`ESPAsyncWebServer`) en el puerto `80`, JSON en request/response.
Todas las rutas `PUT` aceptan body **parcial**: solo se actualizan los campos
que envíes, el resto conserva su valor actual. Cada `PUT` persiste inmediatamente
en `/settings.json` (LittleFS) y devuelve el estado resultante completo.

Ejemplo genérico con curl:
```bash
curl http://la-caja.local/api/presence
curl -X PUT http://la-caja.local/api/presence -d '{"thresholdCm": 20}'
```
Si `la-caja.local` no resuelve por mDNS, usa la IP directa de la placa.

## Estado de la máquina de alarma (`state`)

`GET /api/status` devuelve `state` como entero. Valores (`alarm_engine.h`):

| Valor | Estado | Significado |
|---|---|---|
| 0 | `IDLE` | Reposo, esperando hora de alarma o `sleep/start` |
| 1 | `SLEEP_MODE` | Modo dormir: LED bajando de intensidad hasta apagarse |
| 2 | `WAKE_RAMP` | Subida gradual de luz antes de la alarma (`leadTimeMin`→`buzzLeadTimeMin`) |
| 3 | `WAKE_BUZZ_ESCALATION` | LED sigue subiendo + toques de buzzer cada `buzzIntervalMin` |
| 4 | `ALARM_ACTIVE` | Alarma sonando: LED parpadeando + buzzer continuo |
| 5 | `ALARM_SNOOZED` | Buzzer silenciado tras un gesto, LED sigue parpadeando |

La alarma automática (`WAKE_RAMP`) puede interrumpir `SLEEP_MODE` si coincide la hora.

## Rutas

### `GET /api/status`
Resumen rápido de estado.
```json
{ "state": 0, "alarmEnabled": true, "alarmHour": 7, "alarmMinute": 30 }
```

### `GET /api/settings`
Volcado completo de la configuración persistida (ver estructura combinada de
`time` + `alarm` + `sleep` + `wake` + `presence` + `network` más abajo, tal
como la devuelve cada ruta individual, anidada bajo esas claves).

### `POST /api/settings/reset`
Restaura todos los valores a los defaults de `applyDefaults()` y los guarda.
`204 No Content`.

### `POST /api/alarm/acknowledge`
Parada administrativa: `ALARM_ACTIVE`/`ALARM_SNOOZED` → `IDLE` (apaga LED y
buzzer). No-op en cualquier otro estado. Es el único canal que desactiva la
alarma — el gesto del sensor nunca lo hace. `204 No Content`.

### `POST /api/sleep/start`
`IDLE` → `SLEEP_MODE`. No-op si no está en `IDLE`. `204 No Content`.

### `POST /api/sleep/stop`
`SLEEP_MODE` → `IDLE` (cancelación manual). No-op en cualquier otro estado.
`204 No Content`.

---

### `GET/PUT /api/presence`
Sensor de presencia (HC-SR04) y timeout de la pantalla.

| Campo | Tipo | Descripción |
|---|---|---|
| `thresholdCm` | uint16 | Distancia máxima (cm) para considerar gesto detectado |
| `pollIntervalMs` | uint16 | Cada cuánto se mide distancia |
| `displayTimeoutMs` | uint32 | Cuánto tiempo queda la hora en la LCD tras un gesto |

```bash
curl -X PUT http://la-caja.local/api/presence -d '{"thresholdCm": 20}'
```

---

### `GET/PUT /api/time`
Lectura o ajuste manual de fecha/hora (fallback sin NTP).

| Campo | Tipo | Descripción |
|---|---|---|
| `synced` | bool | Solo en GET — si hay hora válida (NTP o fijada a mano) |
| `year` | int | Año completo (ej. `2026`) |
| `month` | int | 1-12 |
| `day` | int | 1-31 |
| `hour` | int | 0-23 |
| `minute` | int | 0-59 |
| `second` | int | 0-59 |

En `GET`, si `synced` es `false` no se incluyen el resto de campos. En `PUT`,
los campos omitidos toman el valor actual (o `2026-01-01 00:00:00` si aún no
hay hora sincronizada). Fijar la hora aquí marca `synced=true`.

```bash
curl -X PUT http://la-caja.local/api/time -d '{"hour": 22, "minute": 15}'
```

---

### `GET/PUT /api/alarm`

| Campo | Tipo | Descripción |
|---|---|---|
| `enabled` | bool | Si la alarma automática está activa |
| `hour` | uint8 | 0-23 |
| `minute` | uint8 | 0-59 |

```bash
curl -X PUT http://la-caja.local/api/alarm -d '{"enabled": true, "hour": 7, "minute": 30}'
```

---

### `GET/PUT /api/sleep`
Parámetros del modo dormir (LED bajando de intensidad).

| Campo | Tipo | Descripción |
|---|---|---|
| `color` | `{r,g,b}` (0-255 c/u) | Color del LED durante el modo dormir |
| `startIntensity` | uint8 (0-100) | Intensidad inicial (%) al arrancar `sleep/start` |
| `cadenceMs` | uint32 | Cada cuánto se aplica un paso del fundido |
| `stepPercent` | uint8 | Cuánto baja la intensidad (%) en cada paso |
| `durationMin` | uint16 | Tiempo total hasta llegar a intensidad 0 |

```bash
curl -X PUT http://la-caja.local/api/sleep -d '{"color":{"r":255,"g":40,"b":20},"cadenceMs":18000,"stepPercent":1}'
```

---

### `GET/PUT /api/wake`
Parámetros de la rampa de despertar, escalada de buzzer y alarma final.

| Campo | Tipo | Descripción |
|---|---|---|
| `color` | `{r,g,b}` | Color del LED durante `WAKE_RAMP`/`WAKE_BUZZ_ESCALATION` |
| `cadenceMs` | uint32 | Cada cuánto se aplica un paso del fundido de subida |
| `stepPercent` | uint8 | Cuánto sube la intensidad (%) en cada paso |
| `leadTimeMin` | uint16 | Minutos antes de la alarma en que arranca `WAKE_RAMP` |
| `buzzLeadTimeMin` | uint16 | Minutos antes de la alarma en que empieza a pitar (`WAKE_BUZZ_ESCALATION`) |
| `buzzIntervalMin` | uint16 | Cada cuántos minutos suena un toque de buzzer en esa fase |
| `buzzBaseDurationMs` | uint16 | Duración del primer toque de buzzer |
| `buzzDurationIncrementMs` | uint16 | Cuánto se alarga cada toque sucesivo |
| `buzzFrequencyHz` | uint16 | Tono (Hz) de los toques de `WAKE_BUZZ_ESCALATION` |
| `alarmColors` | `[{r,g,b}, {r,g,b}]` | Los dos colores que alterna el parpadeo en `ALARM_ACTIVE` |
| `alarmFlashCadenceMs` | uint16 | Cada cuánto alterna entre los dos colores |
| `alarmBuzzFrequencyHz` | uint16 | Tono (Hz) del buzzer continuo en `ALARM_ACTIVE` |
| `snoozeDurationMin` | uint16 | Minutos que dura `ALARM_SNOOZED` tras un gesto |
| `maxRingDurationMin` | uint16 | Freno de seguridad: tiempo máx. en `ALARM_ACTIVE` sin `acknowledge` antes de volver a `IDLE` |

```bash
curl -X PUT http://la-caja.local/api/wake -d '{"cadenceMs":24000,"stepPercent":1}'
```

---

## Notas

- Ninguna ruta valida rangos (p. ej. `hour > 23`) — el firmware confía en el cliente.
- `PUT` con JSON inválido devuelve `400` con `{"error":"invalid json"}`.
- Rutas no reconocidas devuelven `404` con `{"error":"not found"}`.
- El cambio de un valor por defecto en el firmware **no** se aplica a una placa
  con `/settings.json` ya persistido — hay que enviarlo explícitamente por
  `PUT` (o `POST /api/settings/reset`, que sí reaplica todos los defaults).

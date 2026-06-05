# Red de Comunicaciones IoT para - UABC

Este repositorio contiene el ecosistema de desarrollo de hardware y firmware para la implementación de una red privada de telemetría inalámbrica de largo alcance y bajo consumo, diseñada en vinculación con el Instituto de Ingeniería de la Universidad Autónoma de Baja California (UABC) para el monitoreo de consumo eléctrico en comunidades aisladas (Caso de estudio: Puertecitos, B.C.).

El sistema implementa la tecnología de radiofrecuencia LoRa en la banda libre de 915 MHz, logrando una tasa de pérdida de paquetes muy baja en entornos exteriores a distancias críticas de 300 metros, superando la atenuación por obstáculos físicos y mitigando colisiones por transmisiones concurrentes.

---

## 🛠️ Estructura del Repositorio

* `/placa_lora2`: Archivos de diseño asistido por computadora (CAD) electrónicos correspondientes a la primera etapa de la PCB adaptadora para la integración del transceptor RFM95W y el microcontrolador.
* `/PVVCSMD`: Proyecciones y diseños avanzados de la placa de circuito impreso migrada a tecnología de montaje superficial (SMD) para la optimización de espacio y reducción de costos de fabricación.
* `emisorLora.ino`: Código fuente en C++ para los nodos emisores integrados en los medidores comunitarios. Implementa la estructura de la trama personalizada y el algoritmo de desfasamiento temporal.
* `receptorCentralLora.ino`: Código fuente en C++ para el nodo base centralizador o Gateway encargado de la recepción simultánea, validación de redundancia de errores y puenteo de datos hacia servidores en la nube.

---

## 💻 Descripción de los Códigos de Firmware

### 1. Emisor (`emisorLora.ino`)
Firmware programado para ejecutarse en las unidades remotas de medición. Sus funciones principales son:
* **Estructuración de Trama Propia:** Empaqueta los datos de telemetría bajo un formato optimizado de bajo overhead: `[ID | Seq | Len | Payload | Checksum]` para agilizar el tiempo en el aire (Time-on-Air).
* **Algoritmo de Jitter Aleatorio (Anticolisiones):** Utiliza el ruido electromagnético caótico captado por un pin analógico configurado al aire como semilla para generar un desfasamiento dinámico de +/- 800 ms. Esto evita que los nodos que transmiten simultáneamente bloqueen el canal de comunicación.
* **Control de Errores:** Realiza cálculos de redundancia de datos mediante suma de verificación XOR por software antes de enviar el paquete por el transceptor.

### 2. Receptor Central (`receptorCentralLora.ino`)
Firmware dedicado al Gateway o nodo base de recolección local. Sus funciones principales son:
* **Escucha Concurrente y Modulación:** Configura el módulo de radio bajo parámetros de alta sensibilidad en la frecuencia de 915 MHz, Ancho de Banda (BW) de 125 kHz y Factor de Ensanchamiento (SF) de 7.
* **Doble Validación de Integridad:** Corrobora la exactitud de los bits recibidos mediante el análisis combinado de CRC por hardware y la verificación XOR por software, descartando tramas corruptas provocadas por interferencias del entorno.
* **Métricas de Diagnóstico:** Extrae y evalúa en tiempo real los valores de RSSI (Indicador de Fuerza de Señal Recibida) y SNR (Relación Señal-Ruido) para el diagnóstico físico del enlace.

---

## 📡 Especificaciones de Radiofrecuencia (RF)
* **Tecnología:** LoRa (Chipset Semtech SX1276 / RFM95W)
* **Frecuencia:** 915 MHz (Banda ISM libre para México)
* **Ancho de Banda (BW):** 125 kHz
* **Spreading Factor (SF):** 7
* **Potencia de Salida:** 17 dBm (50 mW)

---

## 👥 Desarrolladores
* Diego Castañeda Covarrubias
* Oscar Eduardo Flores García
* Herick Gerardo Vizcarra Macías

*Proyecto de Vinculación con Valor en Créditos (PVVC) bajo la mentoría del Dr. Saúl - Laboratorio de Energías Renovables, Instituto de Ingeniería de la UABC.*

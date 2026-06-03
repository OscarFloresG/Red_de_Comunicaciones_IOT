/*
 * =====================================================
 * NODO CLIENTE - LoRa Transmitter
 * ESP32 + LoRa SX1276/SX1278
 * =====================================================
 * *** ÚNICO CAMBIO ENTRE CLIENTES: NODE_ID (línea 30) ***
 * Cliente 1 → NODE_ID 0xA1
 * Cliente 2 → NODE_ID 0xA2
 * Cliente 3 → NODE_ID 0xA3
 * (y así sucesivamente...)
 *
 * Todos los demás parámetros deben ser IDÉNTICOS al base.
 * =====================================================
 */

#include <SPI.h>
#include <LoRa.h>

// ── Pines (TTGO LoRa32 / ajustar si es necesario) ──
#define LORA_SCK   4
#define LORA_MISO  15
#define LORA_MOSI  2
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  21

// ── ★★ CAMBIA ESTO EN CADA ESP32 ★★ ──────────────
#define NODE_ID  0xA2          // 0xA1, 0xA2, 0xA3, 0xA4...
// ──────────────────────────────────────────────────

// ── Parámetros de radio (IDÉNTICOS en todos los nodos) ──
#define LORA_FREQ       915E6
#define LORA_BW         125E3
#define LORA_SF         9
#define LORA_CR         5
#define LORA_TX_POWER   17
#define LORA_PREAMBLE   8
#define LORA_SYNC_WORD  0xF3

// ── ID del nodo destino (base) ──
#define BASE_ID    0x00

// ── Intervalo base de transmisión (ms) y rango de Jitter ──
#define TX_INTERVAL_MS   5000    // Tiempo base: 5 segundos
#define JITTER_RANGE_MS  800     // Desfase de +/- 800ms (Total entre 4.2s y 5.8s)

// ── Estructura del paquete ──
#define PKT_SRC_ID      0
#define PKT_SEQ         1
#define PKT_LEN         2
#define PKT_PAYLOAD     3
#define PKT_HEADER_SIZE 3
#define PKT_MAX_PAYLOAD 50

// ── Contador de secuencia y tiempos ──
uint8_t seq_counter = 0;
uint32_t last_tx_ms = 0;
uint32_t current_interval = TX_INTERVAL_MS; // Intervalo dinámico actual

// ──────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== NODO CLIENTE LoRa ===");
  Serial.printf("  ID: 0x%02X  |  Destino: 0x%02X\n", NODE_ID, BASE_ID);

  // Inicializar semilla aleatoria usando ruido de un pin analógico sin conectar (ej. GPIO 34)
  // Esto evita que todos los emisores generen el mismo patrón al encenderse juntos.
  randomSeed(analogRead(34) + NODE_ID);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("[ERROR] LoRa no inicializado. Revisa conexiones.");
    while (true) { delay(1000); }
  }

  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setTxPower(LORA_TX_POWER);
  LoRa.setPreambleLength(LORA_PREAMBLE);
  LoRa.setSyncWord(LORA_SYNC_WORD);
  LoRa.enableCrc();

  // Calcular el primer intervalo aleatorio para romper la simetría inicial
  current_interval = TX_INTERVAL_MS + random(-JITTER_RANGE_MS, JITTER_RANGE_MS);

  Serial.println("[OK] LoRa listo. Transmitiendo...\n");
}

// ──────────────────────────────────────────────────
void loop() {
  uint32_t now = millis();
  
  // Comparamos contra el intervalo dinámico en vez del estático
  if (now - last_tx_ms >= current_interval) {
    last_tx_ms = now;

    // Calcular el intervalo para el SIGUIENTE paquete
    current_interval = TX_INTERVAL_MS + random(-JITTER_RANGE_MS, JITTER_RANGE_MS);

    // ── Arma tu payload aquí ──────────────────────
    float temp   = 22.5 + (random(-30, 30) / 10.0);
    uint32_t uptime_s = now / 1000;

    char payload[PKT_MAX_PAYLOAD];
    int payload_len = snprintf(payload, sizeof(payload),
                               "T:%.1f C  up:%lus", temp, uptime_s);
    if (payload_len < 0 || payload_len >= PKT_MAX_PAYLOAD) {
      payload_len = PKT_MAX_PAYLOAD - 1;
    }
    // ─────────────────────────────────────────────

    send_packet((uint8_t*)payload, (uint8_t)payload_len);
    
    Serial.printf("[INFO] Próximo envío en: %.2f segundos.\n", current_interval / 1000.0);
  }
}

// ──────────────────────────────────────────────────
void send_packet(uint8_t* data, uint8_t data_len) {
  // Construir buffer: [ src_id | seq | len | payload | checksum ]
  uint8_t buf[PKT_HEADER_SIZE + PKT_MAX_PAYLOAD + 1];
  buf[PKT_SRC_ID] = NODE_ID;
  buf[PKT_SEQ]    = seq_counter++;
  buf[PKT_LEN]    = data_len;
  memcpy(&buf[PKT_PAYLOAD], data, data_len);

  // Checksum: XOR de todos los bytes anteriores
  uint8_t chk = 0;
  int total = PKT_HEADER_SIZE + data_len;
  for (int i = 0; i < total; i++) chk ^= buf[i];
  buf[total] = chk;

  // Transmitir
  LoRa.beginPacket();
  LoRa.write(buf, total + 1);
  bool ok = LoRa.endPacket();

  Serial.printf("[TX] ID:0x%02X  Seq:%d  Len:%d  OK:%s  Data: %.*s\n",
                NODE_ID, buf[PKT_SEQ], data_len,
                ok ? "si" : "no", data_len, (char*)data);
}
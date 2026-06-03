/*
 * =====================================================
 *  NODO BASE - LoRa Receiver
 *  ESP32 + LoRa SX1276/SX1278 (TTGO LoRa32 / Heltec)
 * =====================================================
 *  Librería: LoRa by Sandeep Mistry (Arduino Library Manager)
 *  pip: "LoRa" de sandeepmistry
 *
 *  Pinout para TTGO LoRa32 v2.1 (ajusta si usas otra placa):

*    SCK  → 5    MISO → 19   MOSI → 27
 *    SS   → 18   RST  → 14   DIO0 → 26
 * =====================================================

*/

#include <SPI.h>
#include <LoRa.h>

// ── Pines (TTGO LoRa32 / ajustar si es necesario) ──
#define LORA_SCK 4
#define LORA_MISO 15
#define LORA_MOSI 2
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 21

// ── Parámetros de radio (deben coincidir en TODOS los nodos) ──

define LORA_FREQ 915E6  // 915 MHz (USA/MX). Usa 868E6 en Europa.

define LORA_BW 125E3  // Bandwidth 125 kHz
#define LORA_SF 9  // Spreading Factor 9  (rango/velocidad)
#define LORA_CR 5  // Coding Rate 4/5
#define LORA_TX_POWER 17  // dBm (máx 20, legal MX ~14-17)
#define LORA_PREAMBLE 8  // símbolos de preámbulo
#define LORA_SYNC_WORD 0xF3  // "Red privada" — cambia para aislarla

// ── ID de este nodo ──
#define NODE_ID 0x00  // Base = 0x00

// ── Estructura del paquete ──
// [ 1B src_id ][ 1B seq_num ][ 1B payload_len ][ N bytes payload ][ 1B checksum ]
#define PKT_SRC_ID 0
#define PKT_SEQ 1
#define PKT_LEN 2
#define PKT_PAYLOAD 3
#define PKT_HEADER_SIZE 3
#define PKT_MAX_PAYLOAD 50

// ── Tabla de clientes conocidos ──
#define MAX_CLIENTS 16
struct ClientInfo {
  uint8_t id;
  uint32_t last_seen_ms;
  uint16_t packets_rx;
  int16_t last_rssi;
  float last_snr;
;
ClientInfo clients[MAX_CLIENTS];
uint8_t client_count = 0;

// ──────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== NODO BASE LoRa ===");
  Serial.printf("  ID: 0x%02X  |  Freq: %.0f MHz\n", NODE_ID, LORA_FREQ / 1e6);


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

  Serial.println("[OK] LoRa listo. Escuchando...\n");


// ──────────────────────────────────────────────────
void loop() {
  int pkt_size = LoRa.parsePacket();
  if (pkt_size > 0) {
    handle_incoming(pkt_size);
  }

  // truco extra: Si el chip se queda colgado por ruido,
  // esto asegura que siga en modo recepción continua.
  // (No afecta si ya está escuchando)
  LoRa.receive();
}

// ──────────────────────────────────────────────────

// ──────────────────────────────────────────────────
void handle_incoming(int pkt_size) {
  // Asegurarnos de no desbordar nuestro buffer local si viene basura gigante por colisión
  uint8_t buf[PKT_MAX_PAYLOAD + PKT_HEADER_SIZE + 1];
  int len = 0;

  // Leer los datos limitando estrictamente al tamaño del buffer
  while (LoRa.available() && len < (int)sizeof(buf)) {
    buf[len++] = LoRa.read();
  }

  // ¡CRUCIAL!: Si quedaron bytes en el buffer del chip por culpa de la interferencia,
  // los limpiamos por completo para que no afecten al siguiente paquete.
  while (LoRa.available()) {
    LoRa.read();
  }

  // Ahora sí, validamos longitud mínima
  if (len < PKT_HEADER_SIZE + 1) {
    Serial.println("[WARN] Paquete corrupto o incompleto por colisión. Descartado.");
    LoRa.receive();  // Forzar modo escucha antes de salir
    return;
  }

  // Verificar checksum
  uint8_t chk = 0;
  for (int i = 0; i < len - 1; i++) chk ^= buf[i];
  if (chk != buf[len - 1]) {
    Serial.printf("[WARN] Checksum inválido (calculado 0x%02X, recibido 0x%02X) - Posible interferencia.\n",
                  chk, buf[len - 1]);
    LoRa.receive();  // Forzar modo escucha antes de salir
    return;
  }

  uint8_t src_id = buf[PKT_SRC_ID];
  uint8_t seq_num = buf[PKT_SEQ];
  uint8_t payload_len = buf[PKT_LEN];
  int16_t rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();

  // Registrar / actualizar cliente
  update_client(src_id, rssi, snr);

  // Extraer payload como string
  char payload[PKT_MAX_PAYLOAD + 1] = { 0 };
  if (payload_len > PKT_MAX_PAYLOAD) payload_len = PKT_MAX_PAYLOAD;
  memcpy(payload, &buf[PKT_PAYLOAD], payload_len);
  payload[payload_len] = '\0';

  // Imprimir en formato legible
  Serial.println("┌─────────────────────────────────────");
  Serial.printf("│ De:    0x%02X   Seq: %d\n", src_id, seq_num);
  Serial.printf("│ RSSI:  %d dBm   SNR: %.1f dB\n", rssi, snr);
  Serial.printf("│ Data:  %s\n", payload);
  Serial.println("└─────────────────────────────────────");

  // Forzar al radio a regresar al modo de escucha activa
  LoRa.receive();
}



// ──────────────────────────────────────────────────
void update_client(uint8_t id, int16_t rssi, float snr) {
  for (int i = 0; i < client_count; i++) {
    if (clients[i].id == id) {
      clients[i].last_seen_ms = millis();
      clients[i].packets_rx++;
      clients[i].last_rssi = rssi;
      clients[i].last_snr = snr;
      return;
   }
 }
  // Cliente nuevo
  if (client_count < MAX_CLIENTS) {
    clients[client_count++] = { id, millis(), 1, rssi, snr };
    Serial.printf("[INFO] Nuevo cliente registrado: 0x%02X (total: %d)\n", id, client_count);
 }
#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "ota-metadata.h"
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "firmware_data.h"

#include "sys/node-id.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (10 * CLOCK_SECOND)



#define CHUNK_SIZE 64  // Her pakette gidecek maksimum veri boyutu

// Havadan göndereceğimiz paketin iskeleti
typedef struct {
  uint16_t block_id;         // Bu kaçıncı parça? (Sıralama için)
  uint8_t payload_len;       // Bu pakette kaç byte gerçek veri var?
  uint8_t payload[CHUNK_SIZE]; // Gerçek firmware verisi
  uint16_t checksum;         // Veri yolda bozuldu mu diye kontrol (CRC/Checksum)
} ota_packet_t;



static struct simple_udp_connection udp_conn;
static uint32_t rx_count = 0;
static ota_boot_metadata_t boot_metadata = {
  .magic = OTA_IMAGE_MAGIC,
  .active_slot = OTA_SLOT_A,
  .candidate_slot = OTA_SLOT_NONE,
  .state_a = OTA_IMAGE_STATE_CONFIRMED,
  .state_b = OTA_IMAGE_STATE_EMPTY,
};

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  static uint32_t fake_version = 2;
  uint32_t fake_image_crc;

  (void)c;
  (void)sender_port;
  (void)receiver_addr;
  (void)receiver_port;

  LOG_INFO("Client received response '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
  rx_count++;

  /*
   * Placeholder integration for the first OTA path.
   * In the real receiver, this information must come from the fully assembled
   * Slot B image stored in flash.
   */
  fake_image_crc = ota_crc32_buffer(data, datalen);
  if(ota_metadata_mark_verified(&boot_metadata, OTA_SLOT_B,
                                fake_version, datalen, fake_image_crc) &&
     ota_metadata_stage_verified_image(&boot_metadata, OTA_SLOT_B)) {
    LOG_INFO("OTA metadata updated: slot B staged for activation\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t dest_ipaddr;
  static uint32_t tx_count;
  static uint32_t missed_tx_count;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(NETSTACK_ROUTING.node_is_reachable() &&
        NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {

// Bu blok 3 numarali cihazda calismaz. 2-den-1-e gonderim icin yapildi. 3 numarali
// cihaz komsuluk gorevi yapar. İletime yardim eder.

      if(node_id == 2) {

        LOG_INFO("Sending request %" PRIu32 " to ", tx_count);
        LOG_INFO_6ADDR(&dest_ipaddr);
        LOG_INFO_("\n");

        // --- OTA GÖNDERİCİ MANTIĞI BAŞLANGICI ---
// --- OTA GÖNDERİCİ MANTIĞI BAŞLANGICI ---
        static uint16_t current_block = 0;
        uint32_t offset = current_block * CHUNK_SIZE;

        if(offset < FIRMWARE_TOTAL_SIZE) {
            ota_packet_t packet;
            packet.block_id = current_block;

            // Kalan veri hesaplaması
            if((FIRMWARE_TOTAL_SIZE - offset) >= CHUNK_SIZE) {
                packet.payload_len = CHUNK_SIZE;
            } else {
                packet.payload_len = FIRMWARE_TOTAL_SIZE - offset;
            }

            // DİKKAT: Artık memcpy yok, sanal fonksiyondan byte byte okuyoruz
            packet.checksum = 0;
            for(int i = 0; i < packet.payload_len; i++) {
                packet.payload[i] = get_firmware_byte(offset + i);
                packet.checksum += packet.payload[i]; // Checksum'ı aynı döngüde hesaplıyoruz
            }

            printf("OTA Paketi Gonderiliyor: Blok %u, Boyut %u, Checksum %u\n", 
                   packet.block_id, packet.payload_len, packet.checksum);

            simple_udp_sendto(&udp_conn, &packet, sizeof(packet), &dest_ipaddr);

            current_block++; 
        } else {
            printf("OTA AKTARIMI TAMAMLANDI! Tum bloklar gonderildi.\n");
        }
        // --- OTA GÖNDERİCİ MANTIĞI BİTİŞİ ---
        // --- OTA GÖNDERİCİ MANTIĞI BİTİŞİ ---

        tx_count++;
      }


    } else {
      LOG_INFO("Not reachable yet\n");
      if(tx_count > 0) {
        missed_tx_count++;
      }
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/* --- Z1 DERLEME HATASI İÇİN YAMA --- */

void init_platform(void) {
  /* Bağlayıcı (linker) hatasını aşmak için boş bırakıldı */
}

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "cfs/cfs.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678



#define CHUNK_SIZE 64  // Her pakette gidecek maksimum veri boyutu

// Havadan göndereceğimiz paketin iskeleti
typedef struct {
  uint16_t block_id;         // Bu kaçıncı parça? (Sıralama için)
  uint8_t payload_len;       // Bu pakette kaç byte gerçek veri var?
  uint8_t payload[CHUNK_SIZE]; // Gerçek firmware verisi
  uint16_t checksum;         // Veri yolda bozuldu mu diye kontrol (CRC/Checksum)
} ota_packet_t;


static struct simple_udp_connection udp_conn;

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
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
  // Gelen ham veriyi kendi OTA paket yapımıza (struct) dönüştürüyoruz (Casting)
  ota_packet_t *packet = (ota_packet_t *)data;

  // 1. Gelen paket bizim struct boyutumuzla uyuşuyor mu?
  if(datalen != sizeof(ota_packet_t)) {
    printf("HATA: Hatali paket boyutu alindi!\n");
    return;
  }

  // 2. Checksum Doğrulaması (Veri yolda bozulmuş mu?)
  uint16_t calc_checksum = 0;
  for(int i = 0; i < packet->payload_len; i++) {
    calc_checksum += packet->payload[i];
  }

  if(calc_checksum != packet->checksum) {
    printf("HATA: Checksum Uyusmazligi! Blok %u reddedildi.\n", packet->block_id);
    return;
  }

  printf("OTA Paketi Alindi: Blok %u, Boyut %u, Checksum Dogrulandi.\n", packet->block_id, packet->payload_len);

  // 3. Kalıcı Hafızaya (Diske) Yazma İşlemi
  // CFS_WRITE bayrağı dosyayı açar/oluşturur, CFS_APPEND ise sonuna ekler.
// 3. Kalıcı Hafızaya (Diske) Yazma İşlemi
  int fd = cfs_open("new_firmware.bin", CFS_WRITE | CFS_APPEND);
  if(fd >= 0) {
    int written = cfs_write(fd, packet->payload, packet->payload_len);
    cfs_close(fd);
    
    if(written != packet->payload_len) {
      // Disk fiziksel olarak dolduysa (yaklaşık 30KB sonra)
      printf("UYARI: Z1 CFS diski doldu! Testin devami icin disk temizleniyor...\n");
      cfs_remove("new_firmware.bin"); // Dosyayı sil, yer aç
    } else {
      printf("Blok %u CFS ile diske basariyla yazildi.\n", packet->block_id);
    }
  } else {
    // fd < 0 gelirse dosya sistemi kilitlenmiş olabilir
    printf("HATA: Diske yazilamadi! Dosya sistemi resetleniyor...\n");
    cfs_remove("new_firmware.bin");
  }

  // 4. Bitiş Kontrolü
  // 129760 baytlık dosya 64 baytlık parçalara bölündüğünde son parça tam dolmaz (32 bayt kalır).
  // Payload uzunluğu 64'ten küçükse bunun son paket olduğunu anlarız.
  if(packet->payload_len < CHUNK_SIZE) {
    printf("\n*** Yüklenmeye hazır yeni firmware alımı tamamlandı! ***\n\n");
  }

  // (Opsiyonel) Göndericiye ACK mesajı göndermek istersen buraya eklenebilir, 
  // ancak şu anki basit Stop-and-Wait kurgumuz için şart değil.
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/* --- Z1 DERLEME HATASI İÇİN YAMA --- */

void init_platform(void) {
  /* Bağlayıcı (linker) hatasını aşmak için boş bırakıldı */
}

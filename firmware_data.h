#ifndef FIRMWARE_DATA_H
#define FIRMWARE_DATA_H

#include <stdint.h>

/* Toplam dosya boyutumuz: 129760 bayt (32-bit tanımladık ki taşmasın) */
#define FIRMWARE_TOTAL_SIZE 129760ul

/* * 129KB veriyi 92KB'lık Z1'e sığdıramayacağımız için,
 * anlık olarak hafızada yer kaplamayan sahte veri (mock) üretiyoruz.
 */
static inline uint8_t get_firmware_byte(uint32_t offset) {
    // Offset değerine göre deterministik bir desen oluşturur.
    // Alıcı düğüm bu sayede checksum doğrulamasını kusursuz yapabilir.
    return (uint8_t)(offset % 256); 
}

#endif

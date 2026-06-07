# BİL 304 - OTA Firmware Güncelleme Projesi (Faz 1)

Bu proje, Contiki-NG ve Cooja simülatörü kullanılarak Z1 düğümleri (MSP430) arasında parçalı firmware aktarımını ve Coffee File System (CFS) entegrasyonunu içermektedir.

## 🎥 Proje Sunum ve Demo Videosu
ŞUANLIK YOK

## 🛠️ Gerçeklenen Yöntemler ve Alınan Önlemler

### 1. Bellek Taşkını (Overflow) Önlemi: Sanal Veri Üretimi
Z1 cihazının 92KB Flash ve 8KB RAM limitleri göz önüne alındığında, 129KB boyutundaki `new-firmware.z1` dosyasının fiziksel bir C dizisi olarak belleğe alınması "size of array is too large" hatasına sebep olmuştur. Bu donanımsal limitasyonu aşmak için `get_firmware_byte()` fonksiyonu yazılarak sanal (mock) veri üretilmiş ve RAM tasarrufu sağlanmıştır.

### 2. Döngüsel CFS Yazma (Circular Buffer) Simülasyonu
Alıcı (Node 1) tarafında CFS (Coffee File System) kullanılarak veriler diske yazılmıştır. Ancak Z1'in CFS için ayırdığı flaş alanı yaklaşık 30KB'dır. Aktarımın 466. bloğunda disk dolduğu için `cfs_write` hata fırlatmaktadır. Bu durum öngörülerek, disk dolduğunda `cfs_remove` ile dosya sistemini temizleyip aktarıma devam eden bir mekanizma kurgulanmıştır.

## 📦 Paket Uzunlukları ve Yapısı
Ağ üzerinden (IEEE 802.15.4 ve 6LoWPAN) gönderilen verilerin MTU sınırlarına takılıp parçalanmasını (fragmentation) önlemek amacıyla paket payload boyutu **64 Bayt** olarak belirlenmiştir. Toplam paket yapısı (struct) şu şekildedir:

```c
#define CHUNK_SIZE 64

typedef struct {
  uint16_t block_id;           // 2 Bayt - Parça numarası
  uint8_t payload_len;         // 1 Bayt - Gerçek veri uzunluğu
  uint8_t payload[CHUNK_SIZE]; // 64 Bayt - Firmware verisi
  uint16_t checksum;           // 2 Bayt - Bütünlük doğrulama
} ota_packet_t;                // Toplam: 69 Bayt

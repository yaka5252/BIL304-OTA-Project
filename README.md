BÖLÜM 2: ELF Analizi ve Bellek Yerleşim Stratejisi
Bu bölümde, Contiki-NG ortamında geliştirilen new-firmware.z1 bellenim (firmware) dosyasının araç zinciri (toolchain) ile yapılan tersine mühendislik ve bellek analizleri sunulmuştur.

1. Dosyanın ELF Sınıfı, Mimarisi ve Giriş Adresi
file ve readelf -h komutlarıyla yapılan analiz sonucunda dosyanın temel kimliği şu şekildedir:

ELF Sınıfı: ELF32 (32-bit formatında, little-endian veri yapısına sahip).

Mimari (Machine): Texas Instruments msp430 microcontroller (Z1 donanım platformunun çekirdeğini temsil eder).

Giriş Adresi (Entry Point): 0x3100. Bu adres, işlemcinin enerji verildiğinde veya resetlendiğinde (boot) kodu icra etmeye başlayacağı fiziksel Flash bellek adresidir.

2. Neden "Ham Binary" Değil de "ELF Executable" Olarak Değerlendiriliyor?
Gömülü sistemlerde havadan (OTA) veya seri porttan atılan saf veri "Ham Binary" (Raw Binary) olarak adlandırılır. Ancak analiz edilen .z1 dosyası, readelf çıktısında görüldüğü üzere bir ELF (Executable and Linkable Format) yapısına sahiptir.
Bunun sebebi: İçerisinde sadece çalıştırılabilir makine kodu değil; kodun bellekte nereye yükleneceği (Section Headers), sembol tabloları (.symtab), debug bilgileri (.debug_info) ve kesme (interrupt) vektörlerini barındıran kompleks bir veri kapsülü olmasıdır. Sistem, bu dosyayı icra etmeden önce işletim sisteminin bağlayıcısı (linker) aracılığıyla bu yapısal bölgeleri çözümlemek zorundadır.

3. Kod ve Veri Boyutları ile Anlamları
msp430-size komutuyla elde edilen bellek kullanım özeti şöyledir:

text (71715 bayt): Salt okunur olan çalıştırılabilir makine kodlarını, fonksiyonları ve sabitleri barındırır. Doğrudan ROM/Flash belleğe yazılır.

data (336 bayt): İlk değer ataması (initialize) yapılmış global ve statik değişkenlerin boyutudur. Kod başladığında Flash'tan SRAM'e kopyalanırlar.

bss (5706 bayt): İlk değeri atanmamış (veya sıfır atanmış) global değişkenlerin boyutudur. Fiziksel dosyada yer kaplamaz, ancak sistem boot edildiğinde SRAM üzerinde bu kadar baytlık alan sıfırlanarak ayırtılır.

4. Temel Bölümlerin (Sections) Varlığı ve Bellek Stratejisi
msp430-readelf -S komutuyla çıkartılan tabloya göre, firmware'in belleğe oturma stratejisi şu şekildedir:

.text (Adres: 0x3100): Sistemin ana çalıştırılabilir kod bloğudur. Yukarıda belirtilen giriş adresiyle uyuşmaktadır.

.rodata (Adres: 0xc870): Read-Only Data (Salt Okunur Veri). Kod içindeki değiştirilemez const yapılar ve karakter dizileri (string'ler) burada tutulur.

.data (Adres: 0x1100) ve .bss (Adres: 0x1250): Her iki bölüm de SRAM üzerinde birbirine ardışık olarak yerleşmiştir (WA - Write/Alloc bayraklarına sahiptir). Cihazın geçici çalışma belleğini oluştururlar.

.far.text (Adres: 0x10000): MSP430 mimarisinin 64 KB limitini aşan fonksiyonlar için genişletilmiş hafıza bölgesine (Extended Memory) taşan kod bloklarını ifade eder.

5. Kesme Vektörleri (Interrupt Vectors) ve Başlangıç Adresi İlişkisi

.vectors Bölümü: Tabloda 0xffc0 adresinde, 64 baytlık (0x40) bir alan olarak görünmektedir.

Anlamı: MSP430 donanım mimarisinde donanımsal kesmeler (Zamanlayıcılar, UART, Buton kesmeleri vb.) en üst hafıza adreslerine sabitlenmiştir. İşlemci bir kesme aldığında doğrudan 0xffc0 ile 0xffff arasındaki vektör tablosuna bakar. Bu tablonun en sonundaki adres (0xfffe), sistemin "Reset Vektörü"dür ve bizi tekrar Entry Point olan 0x3100 adresine yönlendirir.

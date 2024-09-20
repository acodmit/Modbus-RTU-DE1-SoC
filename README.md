# Modbus-RTU-DE1-SoC
Implementacija Modbus RTU komunikacije na DE1-SoC ploči koristeći libmodbus biblioteku.

## Izvještaj o implementaciji Modbus RTU komunikacije na DE1-SoC ploči

U ovom izvještaju opisano je kako je realizovana **Modbus RTU** komunikacija na **DE1-SoC** ploči pomoću libmodbus biblioteke, uključujući sve korake od podešavanja Buildroot okruženja do razvoja i pokretanja aplikacija za master i slave uređaje. Glavni cilj projekta je omogućavanje komunikacije preko **RS-485** interfejsa sa jednim Modbus RTU slave čvorom radi upravljanja i dobijanja informacija o trenutnom vremenu.
Korišćena platforma:

    Razvojna ploča: DE1-SoC (Altera Cyclone V SoC)
    Operativni sistem: Linux
    Komunikacioni protokol: Modbus RTU preko RS-485
    Biblioteka: libmodbus
    Interfejs: /dev/ttyS1

<img src="https://github.com/user-attachments/assets/3b46bd7c-eba3-4b52-b31c-6afa5adafc44" width="600">

### 1. Podešavanje Buildroot okruženja

Buildroot se koristi za generisanje rootfs-a (root fajl sistema), uključujući sve potrebne pakete za komunikaciju preko Modbus RTU protokola i omogućavanje rada sa serijskim portovima na Linuxu. Da bi aplikacije koje koriste libmodbus radile na DE1-SoC ploči, bilo je potrebno prilagoditi određene postavke u Buildroot okruženju. Evo koje postavke je bilo potrebno izmijeniti:

#### 1.1 Omogućavanje podrške za libmodbus biblioteku

U menuconfig Buildroot-a, potrebno je omogućiti libmodbus. Ovo je postignuto korišćenjem sljedećih koraka:

    make menuconfig

Zatim pronađite i uključite opcije:

    Target packages  --->
       Libraries  --->
          Hardware handling  --->
             [*] libmodbus

#### 1.2 Konfigurisanje kros-kompajlera

Za ARM platformu kao što je DE1-SoC, potrebno je konfigurisati Buildroot za korišćenje odgovarajućeg toolchain-a. 
Postavke su sljedeće:

    Toolchain  --->
       [*] Enable ARM toolchain
       [*] External toolchain (gcc-arm-linux-gnueabihf)
           Toolchain path (/path/to/your/toolchain)
           Toolchain prefix ($(ARCH)-linux)
           External toolchain gcc version (13.x)
           External toolchain kernel headers series (6.1.x)
           External toolchain C library (glibc)
           [*] Toolchain has SSP support?
           [*] Toolchain has SSP strong support?
           [*] Toolchain has C++ support?

#### 1.3 Konfigurisanje Build opcija

Za ispravno konfigurisanje build procesa izvršene su sljedeće postavke:

    Build options  --->
       Commands  --->
          (/ho/configs/terasic_de1soc_cyclone5_defconfig) Location to save buildroot config
          ($(TOPDIR)/dl) Download dir
          ($(BASE_DIR)/host) Host dir
          Mirrors and Download locations  --->
          (0) Number of jobs to run simultaneously (0 for auto)
          [*] Build packages with debugging symbols

#### 1.4 Konfigurisanje Linux kernela

Za Linux kernel, postavljene su sljedeće opcije:

    Linux Kernel  --->
       [*] Linux Kernel
       Kernel version (Custom Git repository)  --->
          (https://github.com/altera-opensource/linux-socfpga.git) URL of custom repository
          (socfpga-6.1.38-lts) Custom repository version

#### 1.5 Konfigurisanje U-Boot-a

Za U-Boot, postavljene su sljedeće opcije:

    U-Boot  --->
       [*] U-Boot
       Build system (Kconfig)  --->
       U-Boot Version (2024.01)  --->
       (board/terasic/de1soc_cyclone5/de1-soc-handoff.patch) Custom U-Boot patches
       U-Boot configuration (Using an in-tree board defconfig file)  --->
          (socfpga_de1_soc) Board defconfig
       [*] U-Boot needs dtc
       [*] Install U-Boot SPL binary image
          (spl/u-boot-spl.bin) U-Boot SPL/TPL binary image name(s)

#### 1.6 Generisanje rootfs-a i ugradnja na SD karticu

Nakon što su sve postavke izvršene, pokrećemo Buildroot:

    make

Ova komanda generiše sliku SD kartice koja posjeduje rootfs sa svim potrebnim binarnim fajlovima i bibliotekama, uključujući libmodbus. Generisana slika se zatim prebacuje na SD karticu koja se koristi na DE1-SoC ploči:

    sudo dd if=sdcard.img of=/dev/sdh bs=1M

Rootfs se nalazi na rootfs particiji na SD kartici.

### 2. Razvoj master i slave aplikacija

Master i slave aplikacije su razvijene na osnovu libmodbus biblioteke koja pojednostavljuje implementaciju Modbus RTU komunikacije. Obje aplikacije su dizajnirane da funkcionišu na DE1-SoC ploči i komuniciraju putem RS-485 interfejsa.

#### 2.1 Master aplikacija

Master aplikacija inicira komunikaciju sa slave čvorom i zahtjeva trenutno vrijeme putem Modbus RTU protokola. Nakon što slave odgovori, master prikazuje dobijene podatke o vremenu na terminalu.

    #include <modbus.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    
    int main() {
    
        modbus_t *ctx;
        uint16_t registers[6];
        int rc;
        char button_press;
    
        // Initialize Modbus RTU
        ctx = modbus_new_rtu("/dev/ttyS1", 9600, 'N', 8, 1);
        if (ctx == NULL) {
            fprintf(stderr, "Unable to create the libmodbus context\n");
            return -1;
        }
    
        modbus_set_slave(ctx, 1);
    
        if (modbus_connect(ctx) == -1) {
            fprintf(stderr, "Connection failed\n");
            modbus_free(ctx);
            return -1;
        }
    
        while (1) {
            printf("Press 'b' and Enter to request current time: ");
            scanf(" %c", &button_press);
    
            if (button_press == 'b') {
                // Read the time from the first 6 registers of the slave
                rc = modbus_read_registers(ctx, 0, 6, registers);
                if (rc == -1) {
                    fprintf(stderr, "Failed to read registers\n");
                    break;
                }
    
                // Print the received time
                printf("Received time: %04d-%02d-%02d %02d:%02d:%02d\n",
                       registers[0], registers[1], registers[2],
                       registers[3], registers[4], registers[5]);
            }
        }
    
        modbus_close(ctx);
        modbus_free(ctx);
    
        return 0;
    }

#### 2.2 Slave aplikacija

Slave aplikacija sluša na serijskom portu i odgovara na zahtjeve master-a slanjem trenutnog vremena, koje pakuje u Modbus registre.
    
    #include <modbus.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <time.h>
    #include <errno.h>
    
    int main() {
    
        modbus_t *ctx;
        modbus_mapping_t *mb_mapping;
        uint8_t query[MODBUS_RTU_MAX_ADU_LENGTH];
        int rc;
    
        // Initialize Modbus RTU
        ctx = modbus_new_rtu("/dev/ttyS1", 9600, 'N', 8, 1);
        if (ctx == NULL) {
            fprintf(stderr, "Unable to create the libmodbus context\n");
            return -1;
        }
    
        modbus_set_slave(ctx, 1);
    
        if (modbus_connect(ctx) == -1) {
            fprintf(stderr, "Connection failed\n");
            modbus_free(ctx);
            return -1;
        }
    
        // Allocate and initialize the memory for the Modbus mapping
        mb_mapping = modbus_mapping_new(0, 0, 6, 0);
        if (mb_mapping == NULL) {
            fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
            modbus_free(ctx);
            return -1;
        }
    
        while (1) {
            // Wait for a query from the master
            rc = modbus_receive(ctx, query);
            if (rc > 0) {
                // Get the current time
                time_t rawtime;
                struct tm *timeinfo;
    
                time(&rawtime);
                timeinfo = localtime(&rawtime);
    
                // Pack the time into registers (Year, Month, Day, Hour, Minute, Second)
                mb_mapping->tab_registers[0] = timeinfo->tm_year + 1900; // Year
                mb_mapping->tab_registers[1] = timeinfo->tm_mon + 1;     // Month
                mb_mapping->tab_registers[2] = timeinfo->tm_mday;        // Day
                mb_mapping->tab_registers[3] = timeinfo->tm_hour;        // Hour
                mb_mapping->tab_registers[4] = timeinfo->tm_min;         // Minute
                mb_mapping->tab_registers[5] = timeinfo->tm_sec;         // Second
    
                // Send the current time back to the master
                modbus_reply(ctx, query, rc, mb_mapping);
            }
        }
    
        // Free the Modbus mapping memory
        modbus_mapping_free(mb_mapping);
        modbus_close(ctx);
        modbus_free(ctx);
    
        return 0;
    }

### 3. Kompajliranje i pokretanje aplikacija

Nakon razvoja koda za master i slave programe, sljedeći koraci su uključivali kros-kompajliranje, prebacivanje binarnih fajlova na DE1-SoC ploču i pokretanje aplikacija.

Kompajliranje master i slave aplikacija:

Kros-kompajliranje se obavlja pomoću ARM toolchain-a.

Master aplikacija:

    arm-linux-gnueabihf-gcc -o modbus_master modbus_master.c -lmodbus

Slave aplikacija:

    arm-linux-gnueabihf-gcc -o modbus_slave modbus_slave.c -lmodbus

Prebacivanje na DE1-SoC ploču:

Nakon kompajliranja, binarni fajlovi se prebacuju na SD karticu:

    cp modbus_master modbus_slave /media/acodmit/rootfs/home/

Pokretanje aplikacija:

Na jednoj DE1-SoC ploči, pokrenuti slave aplikaciju, a zatim master aplikaciju na drugom DE1-Soc uređaju:

    ./modbus_slave
    ./modbus_master

Prikaz rada master i slave aplikacija na slici ispod.

<img src="https://github.com/user-attachments/assets/bf6454a2-ccb7-4f23-b975-399d02ba6c6c" width="1000">

### 4. Komunikacija putem RS485

Povezivanje DE1-SoC ploče putem UART komunikacije ostvaruje se korišćenjem RX i TX pinova. Ova metoda omogućava jednostavnu dvosmjernu serijsku komunikaciju. Ideja projekta bila je da u konačnici DE1-SoC uređaji komuniciraju korišćenjem [RS485 6 Click](https://www.mikroe.com/rs485-6-click?srsltid=AfmBOoopIPb35fW48KlvOwtkO92dJb24rsJSyhWpf-z01FBv7F7HwKhl) modula, koji osim standardnih RX i TX linija zahtijevaju i dodatne pinove RE (Receive Enable) i DE (Driver Enable), čija kontrola je predviđena putem RTS pina ploče.

Međutim, postavljanje RS485 moda komunikacije na način predviđen modbus bibliotekom, rezultuje greškom. Kao rezultat toga, RTS pin ne prelazi na visok nivo u potrebnom vremenskom intervalu, što onemogućava pravilno funkcionisanje komunikacije.

Primjer greške koja se javlja prilikom pokušaja postavljanja RS485 moda kroz libmodbus biblioteku uključuje poruku:

    "Bad file descriptor" ili "Invalid argument"

Mogući uzroci problema mogu biti:

- Izvorni kod libmodbus biblioteke: Vrijednosti makroa i flegova za RS485 režim nisu odgovarajuće za navedenu platformu.
- Kernel drajver: Postoje nedostaci unutar drajvera Linux kernela koji se odnosi na serijske portove i RS485 režim.
- Ioctl pozivi: Provjeriti kako se različiti ioctl pozivi koriste za upravljanje RS485 režimom i da li su svi parametri ispravno postavljeni.

Moguća rješenja uključuju:

- Podešavanje makroa i flegova: Prilagoditi makroe kao što su SER_RS485_ENABLED, SER_RS485_RTS_ON_SEND, i SER_RS485_RTS_AFTER_SEND.
- Rekonfiguracija kernel drajvera: Razmotriti izmjene ili ažuriranja drajvera za serijske portove kako bi se omogućila ispravna kontrola RTS pina.
- Dodavanje novog GPIO pina: Ako je upravljanje RTS pinom problematično, dodati novi GPIO pin za kontrolu RTS signala.

### 5. Zaključak

Implementacijom ove aplikacije postignuta je uspješna komunikacija između master i slave uređaja koristeći Modbus RTU protokol na DE1-SoC ploči. Slave uređaj precizno odgovara na zahtjeve master-a, šaljući trenutno vrijeme i na taj način uspješno realizuje ključne funkcionalnosti predviđene projektom. Iako je trenutna implementacija fokusirana na osnovne aspekte komunikacije, proširenjem sistema kroz integraciju RS-485 interfejsa obezbijedila bi se veća fleksibilnost i robusnost u složenijim industrijskim primjenama.

# IPK-Project1 - xzelin26

## Obsah
- ##### [Teorie](#teorie)
    - [Socket](#socket)
    - [TCP protokol](#tcp-protokol)
    - [UDP protokol](#udp-protokol)
- ##### [Aplikace](#aplikace)
- ##### [Testování](#testování)
    - [TCP](#testování-tcp-části)
- ##### [Bibliografie](#bibliografie)
## Teorie
### Socket
Socket je komunikační koncový bod, který umožňuje procesům komunikovat mezi sebou nebo se síťovými zařízeními. Socket je typicky identifikován adresou IP a číslem portu. Existují různé typy socketů, včetně TCP a UDP socketů, které poskytují různé úrovně spolehlivosti a rychlosti přenosu dat. Používají se pro různé účely, včetně vzdáleného přístupu, síťové komunikace a meziprocesové komunikace. V Linuxu jsou sockety implementovány jako speciální soubory, které procesy mohou číst a zapisovat jako běžné soubory.
### TCP protokol

TCP (Transmission Control Protocol) je jedním z hlavních protokolů používaných v internetovém protokolovém stacku (TCP/IP). Je navržen pro spolehlivý přenos dat mezi zařízeními v síti. TCP zajišťuje doručení dat v pořadí, kontrolu chyb, spolehlivost a tok dat.

Základní vlastnosti TCP zahrnují:

- Spolehlivost: TCP používá mechanismy potvrzování a opakování při přenosu dat, aby zajistil, že žádná data nejsou ztracena nebo poškozena během přenosu.

- Řízení toku: TCP monitoruje a řídí rychlost, kterou jsou data odesílána, aby se zabránilo přetečení bufferů na příjemné straně.

- Ovládání spojení: TCP navazuje a ukončuje spojení mezi komunikujícími zařízeními pomocí tzv. třícestného handshaku.

- Multiplexování: TCP umožňuje, aby více aplikací používalo stejný síťový kanál.

TCP je často používán pro aplikace, které vyžadují spolehlivé a řízené přenosy dat, jako jsou webové prohlížeče (HTTP), e-mailové klienty (SMTP, POP3) nebo přenos souborů (FTP).

### UDP protokol
UDP (User Datagram Protocol) je jednoduchý, bezspojový protokol pro přenos dat v počítačových sítích. Na rozdíl od TCP nenabízí žádnou spolehlivost, kontrolu chyb ani řízení toku dat. Namísto toho UDP poskytuje pouze minimální sadu funkcí pro přímý přenos dat mezi zařízeními.

Hlavní charakteristiky UDP zahrnují:

- Bezspojovost: UDP nevytváří žádné spojení mezi odesílatelí a příjemcem a nezajišťuje žádné potvrzení doručení.

- Žádná spolehlivost: UDP neprovádí kontrolu chyb ani opakování dat v případě jejich ztráty.

- Nízká režie: UDP má nižší režii než TCP, což znamená, že nezahrnuje dodatečné mechanismy pro řízení spojení a spolehlivý přenos dat.

UDP je často preferován v situacích, kdy je důležitější rychlost a minimální režie přenosu dat než spolehlivost. Příklady aplikací používajících UDP zahrnují hlasovou a video komunikaci (VoIP), online hry a některé typy streamování médií.

## Aplikace
Aplikace je implementována v jazyku C. Funguje na principu více procesů. Po zapnutí se rozdělí na dva procesy, jeden sbírá uživatelský vstup a druhý obstarává připojení k serveru jako TCP nebo UDP chat klient.

## Bibliografie
### UDP Protocol:
- RFC 768 - User Datagram Protocol. (1980). [Online]. Dostupné: https://datatracker.ietf.org/doc/html/rfc768.

### TCP Protocol:
- RFC 793 - Transmission Control Protocol. (1981). [Online]. Dostupné: https://datatracker.ietf.org/doc/html/rfc793.

### C Documentation:
- The C Standard Library Reference Guide. (n.d.). [Online]. Dostupné: https://en.cppreference.com/w/c/header.

### IPK24-CHAT protocol
- IPK Project 1: Client for a chat server using IPK24-CHAT protocol. [Online] Dostupné: https://git.fit.vutbr.cz/NESFIT/IPK-Projects-2024/src/branch/master/Project%201.

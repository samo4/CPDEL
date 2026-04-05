# CPDEL

## Completely Pointless DC Electronic Load

> This a 2-channel DC electronic load, built from [cheap chinese DC load modbus modules](./firmware/SOUSIM.md).

## Key takeaways

- don't use http servers for OTA that you don't fully control, because you have no control over TLS chain.. updates to chain can break your OTA
- don't use SPI display if you want full screen animations, it's too slow
- don't use single pole switches for mains...
  .

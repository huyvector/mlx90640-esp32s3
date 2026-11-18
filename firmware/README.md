# Firmware ES3C28P — Glow / Units / SD / DOOM

Binary sao chép từ cache **lần nạp Arduino IDE cuối**, đối chiếu với mã nguồn kèm theo. Chỉ dùng cho LCDWIKI ESP32-S3 N16R8 ES3C28P. `camera.bin` chứa cả game và IWAD; không cần nạp riêng dữ liệu Doom.

## Nạp các vùng riêng

```sh
esptool --chip esp32s3 --port /dev/cu.usbmodem... --baud 460800 write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 camera.bin
```

Chạy từ thư mục chứa các tệp này, thay đúng cổng USB (Windows dùng COM tương ứng). Dùng esptool chính thức hoặc bản đi kèm Arduino-ESP32. Các lần nạp đã thực hiện bằng Arduino IDE.

Bản này dùng **Custom partition**: app 10 MB từ 0x10000; LittleFS/save game từ 0xA10000; NVS vẫn 0x9000/0x5000. Khi nâng từ bản app 3 MB phải nạp cả `partitions.bin` mới. Không dùng `erase-flash` hoặc nạp toàn ảnh merged nếu cần giữ NVS/save game. Tệp dữ liệu ảnh/video nằm trên SD, không nằm trong gói firmware này.

Kiểm tra `SHA256SUMS.txt`, `source-SHA256SUMS.txt`, `build-manifest.json`. Hướng dẫn đầy đủ: [README ES3C28P](../README_ES3C28P.md). Các giấy phép nằm trong [LICENSE-FIRMWARE.md](../LICENSE-FIRMWARE.md) và thư mục nguồn tương ứng.

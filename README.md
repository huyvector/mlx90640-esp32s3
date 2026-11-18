# Thermal Glow · MLX90640 / ESP32-S3

Camera nhiệt cho **LCDWIKI ES3C28P ESP32-S3 N16R8**, màn 2,8 inch và MLX90640.

## [→ Nạp ROM trực tiếp trên trình duyệt](https://huyvector.github.io/mlx90640-esp32s3/)

Dùng Chrome/Edge trên máy tính và cáp USB truyền dữ liệu. Không cần Arduino IDE. Chỉ dành cho ES3C28P ESP32-S3 16 MB flash / 8 MB PSRAM.

- Giao diện glow cyan, Roboto / Material Symbols; cảm ứng có hiệu chuẩn và lưu NVS.
- °C / °F / K; auto/manual −40…300°C; MIN/MAX ổn định.
- 2/4/6/8 FPS, I²C 400 kHz; khoảng 7,8 FPS đã đo trên bo thử.
- Ảnh BMP, video MP4/MJPEG toàn màn hình; xem ảnh, Play/Pause, quản lý và format MicroSD có xác nhận.
- Hiển thị pin theo điện áp; lưu cài đặt; Doom tích hợp trong ROM.

**Cảm biến:** SDA GPIO16, SCL GPIO15, VDD 3V3, GND. Cảm biến trần cần tụ 100 nF + khoảng 10 µF đặt sát chân nguồn. Không cấp 5 V vào cảm biến hoặc BAT.

**Cập nhật:** bỏ chọn Erase device để giữ NVS và dữ liệu game. Nếu đổi từ ROM khác có thể chọn xóa flash, đồng nghĩa mất cài đặt cũ. Không có thao tác tự format thẻ nhớ.

## Tài liệu

- [Hướng dẫn đầy đủ, build và chức năng](README_ES3C28P.md)
- [Binaries, offset và esptool](firmware/README.md)
- [Build manifest và kết quả kiểm tra](firmware/build-manifest.json)
- [SHA-256 firmware](firmware/SHA256SUMS.txt)
- [Giấy phép từng thành phần](LICENSE-FIRMWARE.md)
- [Tài liệu dự án gốc](README_UPSTREAM.md)

Bản release: **2026.11.18**, app SHA-256 `cdd6b2684f885d9584ee3e6699b23d2a31713b9644bf3cf3572bc212768d425e`.
Độ chính xác nhiệt tuyệt đối chưa được đối chiếu nguồn nhiệt chuẩn. Tốc độ ghi tùy thẻ.

Dựa trên [embedded-kiddie/MLX90640](https://github.com/embedded-kiddie/MLX90640), giữ ghi công và giấy phép upstream. Mã nguồn tương ứng của bản firmware có trong repository.

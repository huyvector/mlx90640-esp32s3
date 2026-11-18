# ES3C28P — Thermal Glow / °C °F K / SD / DOOM

Bản dành riêng cho **LCDWIKI ESP32-S3 N16R8 ES3C28P**, LCD ILI9341V 240×320, FT6336G; màn hình sử dụng ngang 320×240. Chuyển từ [embedded-kiddie/MLX90640](https://github.com/embedded-kiddie/MLX90640), commit `b0c377f78f5775a95969783428eb142f027ce64b`.

## Sử dụng camera

Ba mục chính ở dưới: **CAMERA / FILES / SETTINGS**. Nút PHOTO hoặc REC/STOP nằm riêng ở bên phải. Chọn chế độ tại Settings → Capture rồi **APPLY & SAVE**. Chờ SAVE hoàn tất trước khi ngắt nguồn.

- Bus cảm biến và cảm ứng cố định **400 kHz**. Chọn **2 / 4 / 6 / 8 FPS**, mặc định 8; số FPS trên màn hình là đo thực tế.
- Nội suy 1/2/4/6/8, kích thước khối 1/2/4/8; tích hai mức không vượt 8. Cảm biến có 32×24 điểm đo; nội suy làm ảnh mịn hơn.
- Bảng màu Rainbow/Inferno, chấm MIN/MAX và điểm đo tùy chọn. Chạm ảnh để đặt điểm đo.
- MIN/MAX dùng lọc thời gian và giữ vị trí khi điểm đang chọn còn trong khoảng **0,4°C** so với cực trị đã lọc, giảm nhảy do nhiễu nền. Vùng nóng/lạnh khác biệt rõ được theo dõi nhanh hơn. Đây là lớp ổn định thống kê/hiển thị; ảnh nhiệt đầu vào không bị thay bằng dữ liệu giả.
- Giao diện nền xanh đen, viền cyan, nút có chuyển sắc và phản hồi nhấn; số đo trên ảnh có nhãn nền tối bo góc. Hiệu ứng trang và trạng thái quay vẫn có. Phần nền tĩnh được dựng khi cần, tránh tính hiệu ứng toàn màn hình theo từng khung.

## Đơn vị và dải nhiệt

Settings → **Units** chọn Celsius **°C**, Fahrenheit **°F** hoặc Kelvin **K**, rồi APPLY & SAVE. Dữ liệu tính toán/lưu cấu hình dải nhiệt vẫn dùng °C; chỉ đổi đơn vị hiển thị. Số đo, nhãn MIN/MAX/điểm đo, nhiệt độ thân cảm biến và thang màu đều theo đơn vị đã chọn.

Settings → **Temperature** chọn AUTO/MANUAL và kéo hai thanh giới hạn:

| Đơn vị | Dải chỉnh tương ứng |
|---|---|
| °C | −40 … 300°C |
| °F | −40 … 572°F |
| K | 233,15 … 573,15 K |

Bước chỉnh bên trong là 1°C, tương đương 1,8°F hoặc 1 K. Min luôn nhỏ hơn Max. Giới hạn màu chỉ quyết định cách tô ảnh; không phải hiệu chuẩn nhiệt độ. Ảnh/video lưu toàn màn hình có đơn vị được ghi trực tiếp vào hình, nên các tệp cũ giữ nguyên chữ đã ghi.

MLX90640 dùng hệ số hiệu chuẩn EEPROM gốc. Firmware hiện dùng emissivity **0,95**, nhiệt độ phản xạ giả định `Ta − 8°C`. Chưa đối chiếu thiết bị này với nguồn nhiệt chuẩn; không gán sai số thực tế ±x°C cho bo đã lắp. Để cảm biến ổn định nhiệt sau bật nguồn, tránh nhiệt truyền từ LCD/bo và bề mặt phản xạ. `SENSOR` là nhiệt độ thân cảm biến, khác nhiệt độ vật thể. [Datasheet Melexis](https://www.melexis.com/-/media/files/documents/datasheets/mlx90640-datasheet-melexis.pdf).

## Thẻ nhớ và định dạng

Thẻ được kiểm tra nền khi bật máy và khi tháo/cắm lại. Khe không có tín hiệu nhận thẻ riêng được sử dụng trong firmware, nên việc phát hiện có thể mất vài giây.

- FAT12/FAT16/FAT32 đọc được sẽ được nhận bình thường, không tự xóa dữ liệu.
- Thẻ có thể giao tiếp nhưng không có hệ thống tệp được hỗ trợ hoặc hệ thống tệp hỏng sẽ hiện **FORMAT / CANCEL**. Lỗi giao tiếp/không có thẻ được báo riêng.
- Chỉ **FORMAT** sau xác nhận mới tạo lại phân vùng và hệ thống tệp, xóa toàn bộ ảnh, video và tệp khác trên thẻ.
- CANCEL ở thông báo tự động bỏ qua thẻ để người dùng tháo ra. Trong màn quản lý thẻ, EJECT ngừng truy cập tệp và hiện **Safe to remove**; tháo/cắm lại để nhận lại thẻ.
- Files → **biểu tượng thẻ nhớ ở góc trên phải** → FORMAT CARD có thể định dạng cả thẻ đang dùng bình thường. Màn xác nhận luôn xuất hiện; CANCEL tại đây giữ dữ liệu.
- Formatter tạo FAT32 với thẻ từ 64 MB; thẻ nhỏ hơn dùng FAT phù hợp. Kích thước cụm chọn theo dung lượng, tối đa 32 KB.
- Firmware kiểm tra lại danh tính thẻ ngay trước thao tác ghi định dạng. Thay thẻ sau khi mở hộp thoại sẽ làm xác nhận cũ mất hiệu lực.

Quản lý thẻ và format chạy trong task riêng. Đang mở/ghi tệp sẽ không bị unmount/format giữa chừng. Khi format xong, thẻ được mount lại; ảnh/video mới vào `/MLX90640/`, tên `mlxNNNN.bmp` hoặc `mlxNNNN.mp4`. Không ghi đè tên đã tồn tại.

## Ảnh và video

PHOTO lưu **BMP 24 bit, 320×240 toàn màn hình**. REC lưu **MP4 chứa MJPEG**, 320×240, gồm ảnh nhiệt, số đo, thang màu và giao diện đang hiển thị. Mở/hoàn tất MP4 chạy nền; WAIT/SAVE phản hồi trạng thái.

Files có phân trang, chọn nhiều/All, xác nhận Delete và xem trước. Chọn MP4 rồi **PLAY** để tự phát theo thời gian thật; có Pause, Rewind, Previous/Next. BMP có xem toàn màn hình. RAW cũ vẫn được đọc/phát với tốc độ giả định của định dạng cũ.

Khi mở ảnh/video, firmware dừng lấy/tính nhiệt và tạm dừng ADC/thống kê nền cho đến khi trở lại CAMERA. Trình phát hỗ trợ MP4/MJPEG một track do firmware này tạo. Clip tối đa 7.200 khung và khoảng 2 GB; thời gian quay phụ thuộc FPS thực. Mất nguồn khi chưa hoàn tất clip có thể để lại tệp chưa phát được.

Đã đo trên thẻ thử sau format: ghi liên tục khoảng **7,8 FPS**, không bỏ khung trong clip 311 khung / 4.716.070 byte; phát tự động đi qua các khung 40/79/118 tại khoảng 5/10/15 giây và phát hết. Các số này phụ thuộc cảnh, thẻ và chất lượng nguồn; những thẻ/cấu hình trước từng chỉ đạt khoảng 3,7 FPS.

## Lưu cài đặt và căn cảm ứng

APPLY & SAVE lưu nội suy, khối, FPS, màu, marker/điểm đo, dải nhiệt, auto/manual, Photo/Video và đơn vị vào **NVS trong flash**, không cần SD. Back hủy phần chỉnh chưa áp dụng. Điểm đo lưu khi nhấc tay. Cấu hình có phiên bản/checksum và kiểm tra giới hạn; chỉ ghi khi thay đổi. Schema cũ được chuyển sang schema mới, đơn vị mặc định °C và FPS vượt giới hạn đưa về 8.

Device → **Touch alignment → CALIBRATE**: giữ đúng tâm từng dấu cộng đến khi xanh rồi nhấc tay, đủ sáu điểm. Mẫu hợp lệ được dùng để tính ánh xạ affine và lưu vào NVS; mẫu không nhất quán bị từ chối. Các thanh offset vẫn có để tinh chỉnh. Hệ số dự phòng ban đầu lấy từ sáu điểm đã đo trên chính bo này, không phải hệ số chung cho mọi tấm cảm ứng.

Touch có task riêng, lọc trung vị ba mẫu, hàng đợi nhấn/nhả và chờ nhấc tay khi đổi trang. Back/± phản hồi khi nhấn; quay/xóa vẫn yêu cầu nhấn và nhả đúng nút. Đọc MLX chia burst tối đa 256 từ, giữ nhất quán RAM và nhường bus cho touch; mỗi ảnh cần đủ hai subpage mới. Không ghi EEPROM hiệu chuẩn của MLX.

## Game ẩn DOOM

Settings → Device → **Info**, chạm **5 lần liên tiếp** ở vùng nội dung. Back và ba nút dưới không tính; khoảng cách giữa các lần chạm tối đa 1,5 giây. Không có mục game trên menu chính.

Engine và **DOOM Shareware 1.9 Episode 1** được nhúng trong flash, không cần SD. Mã nền [DoomGeneric](https://github.com/ozkl/doomgeneric), commit `e257a09527062bfb986b6e5e0780839b6cbd5b71`. IWAD gốc 4.196.020 byte, MD5 `f0cefca49926d00903cf57551d901abe`, giữ nguyên dữ liệu.

- D-pad bên trái: tiến/lùi và quay; có thể đi chéo/đi kết hợp quay.
- Bên phải: WEAPON, USE/OK, FIRE. MENU, MAP và EXIT ở trên.
- Điều khiển một điểm chạm theo cách bố trí CYD; luân phiên di chuyển và bắn.
- Camera/tác vụ nền tạm dừng trong lúc chơi. EXIT đưa về camera bằng khởi động lại, giữ NVS/cài đặt.
- Lưu game/cấu hình game dùng LittleFS trong phân vùng flash riêng, không dùng thẻ SD. Phân vùng nội bộ được khởi tạo ở lần mở đầu tiên.
- Game hiện không có âm thanh. Đo thực tế trên bo: khoảng **12,6–13,9 FPS**, PSRAM trống khoảng 2,3 MB trong đoạn theo dõi.

## Đấu nối

| Chức năng | Chân trên bo |
|---|---|
| MLX SDA, chân 1 cảm biến | GPIO16 / SDA P4 |
| MLX VDD, chân 2 | 3V3 P4 |
| MLX GND, chân 3 | GND P4 |
| MLX SCL, chân 4 | GPIO15 / SCL P4 |
| Nguồn từ mạch ngoài | BAT+/GND, pin Li-ion 1S |
| Đo pin | GPIO9, bộ chia 200kΩ/200kΩ có sẵn |
| SD_MMC | CLK38, CMD40, D0=39, D1=41, D2=48, D3=47 |

P4: 1=3V3, 2=GND, 3=SCL, 4=SDA. P3: GPIO2/3/14/21, không có GND. Đối chiếu dấu pin1 và hình nhìn đáy cảm biến trong datasheet. Cảm biến trần cần 100nF song song khoảng 10µF sát VDD/GND; bo có pull-up SDA/SCL 4,7kΩ. Nguồn BAT là pin 1S, không cấp 5V vào BAT.

Chỉ báo phần trăm pin ước lượng điện áp; chưa hiệu chỉnh bằng đồng hồ. Không có nút chụp GPIO, nút nguồn phần mềm hoặc trạng thái đang sạc.

## Build / nạp

Arduino IDE mở `MLX90640.ino`:

- Arduino-ESP32 **3.3.5**, ESP32S3 Dev Module, CPU 240MHz.
- Flash **16MB**, QIO 80MHz, PSRAM **OPI**.
- USB Hardware CDC and JTAG, CDC On Boot Enabled.
- **Partition Scheme: Custom** — dùng `partitions.csv` ngay trong sketch. Ứng dụng 10 MB để chứa game.
- Erase All Flash Before Sketch Upload **Disabled**.
- LovyanGFX **1.2.19**, Adafruit BusIO **1.17.4**. Driver MLX được kèm trong `src/mlx90640`.

```sh
arduino-cli compile --fqbn 'esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,PartitionScheme=custom' /duong/dan/MLX90640
```

Vùng NVS vẫn ở 0x9000, không thay vị trí khi chuyển sang Custom. Hướng dẫn nạp binary ở [firmware/README.md](firmware/README.md). Không nạp toàn ảnh merged nếu cần giữ NVS/save game.

Bộ icon [Google Material Symbols Rounded](https://developers.google.com/fonts/docs/material_symbols) và font Roboto được nhúng sẵn. Tạo lại tài nguyên bằng `ui_assets/generate_material_assets.py` với Pillow, fonttools, aggdraw; không cần mạng khi tạo lại. File SVG, TTF và giấy phép có trong `ui_assets/`. Bố cục tham khảo chức năng [camera FLIR](https://docs.flir.com/T810664/en-US/latest/s06.html) và ảnh giao diện glow người dùng cung cấp; frontend hiện dùng LovyanGFX.

## Chẩn đoán và kiểm tra

Serial 115200: `i` thông tin, `h` về camera, `p` chụp, `v` quay/dừng, `l` phát clip vừa ghi, `s` lưu, `R` khởi động lại sau hoàn tất ghi; `1`…`4` chọn 2/4/6/8 FPS, `t` bật/tắt log touch, `d` mở căn touch. `g` mở game để chẩn đoán; trong game `h` trở về camera, `w/s/a/d`, `f`, Space, `m`, `t`, `q/e` là phím điều khiển.

PERF in nguồn/hiển thị/FPS video, thời gian đọc/tính/vẽ/LCD, JPEG/SD gần nhất, số khung bỏ và lỗi I²C. Khi xem media/game, thống kê camera được tạm dừng. STORAGE báo trạng thái, lần nhận thẻ và kết quả format.

Các test trong `tests/` kiểm tra codec schema/migration/CRC/đơn vị, °C/°F/K, ổn định MIN/MAX và đáp ứng thay đổi thật, điều kiện đồng ý format/đổi thẻ/busy, 5 lần chạm, affine touch và renderer nhiệt bằng ASan/UBSan; muxer/reader MP4 đã được kiểm tra với clip tổng hợp và lỗi ghi/seek. Doom core với IWAD nhúng đã chạy host ASan và chạy/điều khiển/thoát trên bo.

Chưa kiểm chuẩn nhiệt độ tuyệt đối, chưa xác nhận tụ sát cảm biến đã lắp và chưa đối chiếu ADC pin bằng đồng hồ. Nhánh thẻ sai hệ thống tệp có kiểm tra logic; thử format phần cứng vừa thực hiện trên thẻ thử đang đọc được.

## Nguồn và giấy phép

Camera gốc MIT; Adafruit BSD; thuật toán Melexis giữ thông báo Apache-2.0; DoomGeneric GPL-2.0-or-later; Material Symbols Apache-2.0; Roboto OFL-1.1. Dữ liệu DOOM Shareware thuộc id Software. Xem [LICENSE-FIRMWARE.md](LICENSE-FIRMWARE.md) và các giấy phép kèm theo.

[LCDWIKI ES3C28P](https://www.lcdwiki.com/2.8inch_ESP32-S3_Display) · [Sơ đồ bo](https://www.lcdwiki.com/res/ES3C28P/2.8inch_ESP32-S3_Display_Schematic.pdf)

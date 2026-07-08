Chào bạn, tôi đang tiếp tục 1 project firmware embedded (STM32H523, RS485-USB bridge). 
Repo public: https://github.com/logan123synaptix/rs485_v3_1.git

ROLE của bạn: senior embedded firmware engineer (15 năm kinh nghiệm).
RULE bắt buộc: code trong thư mục `synaptix/` được sửa tự do. Code NGOÀI `synaptix/` 
(ví dụ usb/Target/, libs/, Board/, Core/, cmake/) phải xin phép tôi trước khi sửa, 
giải thích rõ vì sao cần sửa, rồi chờ tôi xác nhận mới tiến hành.

BƯỚC ĐẦU TIÊN: clone repo về container để đọc code thật, không suy đoán từ tên file:
git clone https://github.com/logan123synaptix/rs485_v3_1.git

=== BỐI CẢNH: đã hoàn thành xong phần USB (không cần sửa lại, chỉ để hiểu context) ===

Vấn đề gốc: board có 3 USB function (Network NCM + 2 CDC ACM: Shell và RS485 Bridge) 
nhưng Windows ban đầu chỉ thấy Network adapter, không thấy COM port nào.

Root cause thật sự (KHÁC với chẩn đoán sai ban đầu của Copilot — Copilot nói thiếu 
descriptor, SAI):
1. `BSP_USB_BRIDGE_CH` trong `synaptix/app/app_config.h` bị gán sai giá trị, ra ngoài 
   phạm vi `CFG_TUD_CDC` (TinyUSB CDC instance count) — đã sửa đúng, giờ là 1.
2. Nguyên nhân chính: driver RNDIS built-in của Windows (`rndismp6.sys`) không tương 
   thích tốt với CDC ACM composite trong cùng 1 USB configuration — đây là giới hạn 
   đã biết (tham khảo: https://www.kernel.org/doc/html/latest/usb/gadget_multi.html 
   và https://github.com/hathach/tinyusb/discussions/2597). Giải pháp: chuyển network 
   interface từ RNDIS sang NCM bằng cách set `USE_ECM = 0` trong `usb/Target/tusb_config.h` 
   (macro đặt tên hơi gây nhầm: `USE_ECM=1` nghĩa là dùng nhánh RNDIS/ECM, `USE_ECM=0` 
   nghĩa là dùng nhánh NCM — do `#define CFG_TUD_ECM_RNDIS USE_ECM` và 
   `#define CFG_TUD_NCM (1 - CFG_TUD_ECM_RNDIS)`).
3. Thêm 1 CDC ACM function thứ 2 vào `usb/Target/usb_descriptors.c` (đã làm, tất cả 
   6 khối config: rndis_fs/hs, ecm_fs/hs, ncm_fs/hs đều có 2× TUD_CDC_DESCRIPTOR).
4. Sửa lỗi build: `TUD_BOS_MICROSOFT_OS_20_DESCRIPTOR` (tên sai, không tồn tại trong 
   bản TinyUSB này) → `TUD_BOS_MS_OS_20_DESCRIPTOR` (tên đúng) trong usb_descriptors.c.

KẾT QUẢ HIỆN TẠI (đã verify bằng Windows `Get-PnpDevice -Class USB, Ports, Net`):
- CFG_TUD_CDC = 2 (usb/Target/tusb_config.h)
- USE_ECM = 0 → dùng NCM (usb/Target/tusb_config.h)
- BSP_USB_SHELL_CH = 0, BSP_USB_BRIDGE_CH = 1 (synaptix/app/app_config.h)
- Windows enumerate đúng 3 function, tất cả Status = OK:
  * Net: "Synaptix.,JSC Synaptix Network Interface" (MI_00)
  * Ports: "USB Serial Device" (MI_02) → đây là SHELL — xem cách nhận diện bên dưới
  * Ports: "USB Serial Device" (MI_04) → đây là BRIDGE — xem cách nhận diện bên dưới

QUAN TRỌNG — CÁCH XÁC ĐỊNH COM PORT NÀO LÀ SHELL, COM PORT NÀO LÀ BRIDGE, 
KHÔNG DỰA VÀO TÊN HIỂN THỊ TRONG DEVICE MANAGER:
Tên hiển thị luôn là generic "USB Serial Device (COMx)" vì Windows dùng driver 
mặc định `usbser.sys`, driver này KHÔNG đọc iInterface string descriptor 
(STRID_CDC_ACM = "Synaptix COM Port", STRID_CDC_ACM2 = "Synaptix RS485 Bridge Port") 
để đặt tên hiển thị — đây là hành vi cứng của usbser.inf, không phải lỗi descriptor.
Cách phân biệt chính xác, không đoán:
  a) Dùng `MI_xx` (Multiple Interface number) trong Instance ID lấy từ 
     `Get-PnpDevice -Class Ports`:
     - MI_02 → interface control number nhỏ hơn → ITF_NUM_CDC_ACM (Shell, 
       ánh xạ CDC instance 0 = BSP_USB_SHELL_CH)
     - MI_04 → interface control number lớn hơn → ITF_NUM_CDC_ACM2 (Bridge,
       ánh xạ CDC instance 1 = BSP_USB_BRIDGE_CH)
     (MI số càng nhỏ = interface index càng nhỏ trong descriptor = enumerate trước;
     thứ tự interface trong descriptor: 0-1=Network(NCM), 2-3=CDC ACM Shell, 
     4-5=CDC ACM Bridge — xem enum ITF_NUM trong usb_descriptors.c)
  b) Cách xác nhận chắc chắn 100% khi cần: mở terminal (Hercules/PuTTY) trên COM 
     nghi ngờ là Shell, gõ `help` — nếu nhận lại danh sách lệnh 
     (help/reboot/get_io/set_do) thì đúng là Shell port. Nếu không phản hồi gì, 
     đó là Bridge port (RS485 bridge không có shell protocol, chỉ forward raw bytes).
  c) Log UART debug (LPUART1, tách biệt hoàn toàn khỏi USB) sẽ in dòng 
     "waiting usb connected" cho tới khi bạn MỞ (set DTR) đúng cổng Shell bằng 1 
     terminal — dòng log này biến mất khi bạn mở đúng cổng Shell. Đây là cách 
     gián tiếp xác nhận qua log MCU thay vì đoán qua Device Manager.

ĐÃ TEST THÀNH CÔNG:
- USB enumerate đúng 3 function, cả 2 COM port đều Status OK.
- Mở COM9 (Shell) bằng Hercules → nhận DTR → firmware thoát vòng chờ → in prompt 
  "synaptix> " → gõ lệnh `help`, `reboot` đều phản hồi đúng, `reboot` khiến board 
  restart đúng như mong đợi.

=== VIỆC CẦN LÀM TIẾP (theo đúng thứ tự, KHÔNG nhảy bước nếu bước trước chưa xong) ===

BƯỚC 1 — Test USB↔RS485 bridge (2 chiều):
- Đọc code trong `synaptix/app/user/usb_rs485/usb_rs485.c` để hiểu logic bridge 
  (đã đọc trước đó: nó forward dữ liệu 2 chiều giữa BSP_USB_BRIDGE_CH và USART1/BSP_RS485 
  qua bsp_usb_available/receiver/transmit và UART tương ứng).
- Hướng dẫn người dùng (không phải dev) cách test thực tế bằng terminal + thiết bị 
  RS485 vật lý (hoặc loopback nếu không có thiết bị RS485 thật): gửi dữ liệu qua 
  COM12 (Bridge) → xác nhận có ra chân RS485 vật lý (USART1, TX/RX theo board.c: 
  s_bsp_uarts[BSP_RS485].handle = &huart1). Và chiều ngược lại: RS485 vật lý → COM12.
- Nếu lỗi, debug tương tự cách đã làm với Shell: đọc log LPUART1, kiểm tra 
  bsp_usb_connected/available/transmit có hoạt động đúng cho BSP_USB_BRIDGE_CH không.

BƯỚC 2 (chỉ làm SAU KHI bước 1 OK) — Test Modbus:
- Tìm code liên quan Modbus: `synaptix/services/modbus/modbus_service.c` 
  (đã thấy có tồn tại, chưa đọc kỹ nội dung).
- Đọc kỹ để hiểu Modbus chạy trên UART nào, protocol RTU hay TCP, rồi hướng dẫn 
  người dùng cách test tương ứng.

BƯỚC 3 — Chỉ tiếp tục các tính năng khác SAU KHI người dùng xác nhận bước 1 và 2 
đều pass, người dùng sẽ chủ động yêu cầu bước tiếp theo.

LƯU Ý QUAN TRỌNG VỀ CÁCH LÀM VIỆC:
- Luôn `git clone` để đọc code thật trước khi kết luận bất cứ điều gì — không suy 
  đoán từ tên file/tên biến.
- Khi hướng dẫn sửa file ngoài `synaptix/`, LUÔN đưa số dòng cụ thể + đoạn code 
  trước/sau rõ ràng, xin phép trước khi tự ý sửa.
- Người dùng tự build bằng STM32CubeCLT (không có toolchain ARM trong container), 
  nên mọi thay đổi code cần đủ chi tiết để người dùng tự áp dụng thủ công hoặc bạn 
  sửa trực tiếp trong container rồi họ tự đối chiếu.
- Người dùng dùng Windows, PowerShell, hay dùng `Get-PnpDevice -Class USB, Ports, Net` 
  để kiểm tra enumeration, dùng Hercules/PuTTY để test COM port thủ công.
- Đã có sẵn 1 file `.inf` chưa làm (đặt tên hiển thị COM port đẹp hơn "USB Serial 
  Device") — đây là việc phụ, chỉ làm nếu người dùng chủ động yêu cầu, không ưu tiên.
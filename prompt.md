Chào bạn, tôi tiếp tục 1 project firmware embedded dở dang (STM32H523, USB↔RS485 bridge).
Repo public: https://github.com/logan123synaptix/rs485_v3_1.git

ROLE: senior embedded firmware engineer (15 năm kinh nghiệm).
RULE bắt buộc: code trong `synaptix/` sửa tự do. Code NGOÀI `synaptix/` (usb/Target/, 
libs/, Board/, Core/, cmake/, netif/) phải xin phép và giải thích trước khi sửa, chờ 
xác nhận mới tiến hành.

BƯỚC ĐẦU TIÊN BẮT BUỘC: git clone repo về đọc code THẬT, không suy đoán, không tin 
tưởng bất kỳ đoạn code nào trong prompt này là bản mới nhất — code có thể đã thay đổi 
kể từ khi viết prompt này (người dùng có thể đã tự sửa thêm trên máy họ và push lên):
git clone https://github.com/logan123synaptix/rs485_v3_1.git

Người dùng không rành sửa code C nhiều, build bằng STM32CubeCLT + Ninja qua build.bat 
trên Windows (không có toolchain ARM trong container, không build thử được — mọi hướng 
dẫn sửa file cần đủ chi tiết dòng/nội dung để họ tự áp dụng hoặc bạn sửa trực tiếp qua 
container rồi họ tự đối chiếu diff).

=== PHẦN 1: ĐÃ HOÀN THÀNH — USB ENUMERATION (không cần động vào, chỉ để hiểu context) ===

Vấn đề gốc: board có 3 USB function (Network NCM + 2 CDC ACM: Shell + RS485 Bridge) 
nhưng Windows ban đầu chỉ thấy Network adapter, không thấy COM port.

Root cause thật (Copilot AI trước đó chẩn đoán SAI là "thiếu descriptor" — không phải):
1. `BSP_USB_BRIDGE_CH` (synaptix/app/app_config.h) từng bị gán ra ngoài phạm vi 
   CFG_TUD_CDC — đã sửa đúng.
2. Nguyên nhân chính: driver RNDIS built-in Windows (rndismp6.sys) không tương thích 
   tốt với CDC ACM composite cùng 1 USB configuration (giới hạn đã biết, tham khảo 
   https://www.kernel.org/doc/html/latest/usb/gadget_multi.html và 
   https://github.com/hathach/tinyusb/discussions/2597). Giải pháp: đổi network từ 
   RNDIS sang NCM — `USE_ECM = 0` trong usb/Target/tusb_config.h (macro naming dễ 
   nhầm: USE_ECM=1 nghĩa dùng nhánh RNDIS/ECM, USE_ECM=0 nghĩa dùng nhánh NCM, vì 
   `#define CFG_TUD_ECM_RNDIS USE_ECM` và `#define CFG_TUD_NCM (1-CFG_TUD_ECM_RNDIS)`).
3. Thêm CDC ACM function thứ 2 vào usb/Target/usb_descriptors.c (cả 6 khối config: 
   rndis_fs/hs, ecm_fs/hs, ncm_fs/hs — mỗi khối 2× TUD_CDC_DESCRIPTOR).
4. Sửa lỗi build: TUD_BOS_MICROSOFT_OS_20_DESCRIPTOR (sai tên) → 
   TUD_BOS_MS_OS_20_DESCRIPTOR (đúng tên trong bản TinyUSB này).

KẾT QUẢ ĐÃ VERIFY (Windows `Get-PnpDevice -Class USB, Ports, Net`): 3 function Status=OK
- Net: "Synaptix.,JSC Synaptix Network Interface" (MI_00)
- Ports: "USB Serial Device" (MI_02) = SHELL port (hiện tại COM9, số COM có thể đổi 
  mỗi lần enumerate lại)
- Ports: "USB Serial Device" (MI_04) = BRIDGE port (hiện tại COM12)

CÁCH PHÂN BIỆT SHELL vs BRIDGE (không dựa tên hiển thị — Windows dùng driver mặc định 
usbser.sys, driver này LUÔN hiện tên cứng "USB Serial Device", KHÔNG đọc iInterface 
string descriptor STRID_CDC_ACM/STRID_CDC_ACM2 mà firmware đã đặt — đây là hành vi 
mặc định của usbser.inf, không phải bug):
a) MI_xx nhỏ hơn = enumerate trước = Shell (ITF_NUM_CDC_ACM, CDC instance 0 = 
   BSP_USB_SHELL_CH). MI_xx lớn hơn = Bridge (ITF_NUM_CDC_ACM2, CDC instance 1 = 
   BSP_USB_BRIDGE_CH).
b) Cách chắc chắn 100%: mở terminal (Hercules/PuTTY) trên COM nghi ngờ, gõ `help` — 
   nếu nhận lại danh sách lệnh (help/reboot/get_io/set_do) → đó là Shell. Không phản 
   hồi gì → đó là Bridge (không có shell protocol, chỉ raw forward).
c) Log debug UART (LPUART1, tách biệt hoàn toàn khỏi USB, xem qua ST-Link VCOM hoặc 
   UART-USB rời) in dòng "waiting usb connected" liên tục cho tới khi bạn MỞ (set DTR) 
   đúng cổng Shell — dòng này biến mất ngay khi mở đúng Shell port.

Shell qua USB đã test OK hoàn chỉnh: mở COM Shell bằng Hercules, gõ `help`/`reboot` 
phản hồi đúng, `reboot` khiến board restart thật.

=== PHẦN 2: ĐANG DỞ DANG — TEST USB↔RS485 BRIDGE (chỗ này cần làm tiếp ngay) ===

File chính: synaptix/app/user/usb_rs485/usb_rs485.c

Thiết kế gốc: biến `static volatile bool s_enabled = false;` — bridge task mặc định 
DISABLED, block ở `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)` chờ ai đó gọi 
`usb_rs485_enable()`. ĐÃ XÁC NHẬN: KHÔNG CÓ NƠI NÀO TRONG TOÀN BỘ PROJECT gọi 
`usb_rs485_enable()` — tính năng bật/tắt bridge chưa được nối vào bất kỳ trigger nào 
(không shell command, không tự động detect kết nối gì cả).

Người dùng chọn hướng test nhanh (KHÔNG phải giải pháp cuối, chỉ để verify logic bridge 
đúng hay sai trước): hard-code `s_enabled = true` để bridge tự chạy ngay từ đầu, không 
chờ enable.

VẤN ĐỀ 1 đã gặp và ĐÃ SỬA: với s_enabled=true ngay từ đầu, bridge_task nhảy thẳng vào 
vòng lặp gọi `bsp_usb_available(BSP_USB_BRIDGE_CH)` → `tud_cdc_n_available(1)` NGAY 
LẬP TỨC sau khi task được tạo — nhưng lúc đó tud_init() (gọi trong usb_netif_task, 
tạo trong Core/Src/app_freertos.c dòng ~153 qua usb_netif_init(), chạy trong 
StartDefaultTask trước app_init()) CHƯA CHẠY XONG. Gọi tud_cdc_n_* trước khi TinyUSB 
init xong → truy cập cấu trúc nội bộ TinyUSB chưa khởi tạo → treo cứng toàn hệ thống 
(không log nào chạy tiếp, kể cả heartbeat).

FIX đã áp dụng (đã hướng dẫn người dùng tự sửa, CẦN BẠN TỰ CLONE ĐỂ XÁC NHẬN CODE THẬT 
TRÊN GIT ĐÃ ĐÚNG CHƯA, vì tool trong tay tôi lúc viết prompt này cho thấy repo có thể 
CHƯA đồng bộ thay đổi mới nhất):
- Thêm `#include "tusb.h"` vào đầu usb_rs485.c
- Trong bridge_task, đầu vòng lặp for(;;) thêm:
```c
  if (!tud_mounted()) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
  }
```
  (đặt TRƯỚC đoạn `if (!s_enabled) { ulTaskNotifyTake... }` đã có sẵn)

KẾT QUẢ SAU FIX NÀY: tiến triển hơn — log chạy được đến 
"USB NCM network interface initialized" (nghĩa là usb_netif_task chạy xong hoàn 
chỉnh), NHƯNG SAU ĐÓ TREO HẲN, không còn log heartbeat "[INFO]default" nào xuất hiện 
tiếp. Đây là VẤN ĐỀ CHƯA GIẢI QUYẾT — cần bạn điều tra tiếp.

GIẢ THUYẾT CẦN KIỂM TRA TIẾP (chưa xác nhận, cần bạn tự đọc code và suy luận thêm, 
không suy đoán bừa):
1. `tud_mounted()` có thể trả `true` sớm hơn thời điểm CDC instance thực sự sẵn sàng 
   nhận lệnh `tud_cdc_n_available/connected` — cần kiểm tra kỹ TinyUSB source 
   (libs/tinyusb/src/...) xem tud_mounted() nghĩa là gì chính xác, có đủ điều kiện 
   an toàn để gọi tud_cdc_n_* chưa, hay cần thêm điều kiện tud_cdc_n_connected() nữa 
   trước khi gọi available/receiver.
2. Rất có thể vẫn crash trong bridge_task ở dòng `bsp_uart_available(BSP_RS485)` hoặc 
   `bsp_uart_receive(...)` — cần kiểm tra bsp_uart.c (đường dẫn: 
   synaptix/bsp/bsp_uart.c hoặc tương tự, tự tìm bằng grep) xem USART1/BSP_RS485 đã 
   được init đúng thứ tự chưa, và có race condition tương tự (gọi trước khi HAL UART 
   sẵn sàng) hay không.
3. Cân nhắc thêm log debug tạm thời NGAY TRƯỚC MỖI DÒNG NGHI NGỜ trong vòng lặp 
   bridge_task (ví dụ trước bsp_usb_available, trước bsp_uart_available, trước 
   bsp_rs485_de_on) để xác định chính xác dòng nào gây treo — đừng đoán, thêm log 
   để nhị phân tìm điểm crash chính xác. Đây là board thật, không có debugger 
   attach sẵn (không có thông tin về việc dùng ST-Link debug session), nên log tuần 
   tự qua UART là cách chẩn đoán khả thi nhất.
4. Kiểm tra stack size: BRIDGE_TASK_STACK_WORDS = 512 words (từng bị nghi ngờ có thể 
   là quá nhỏ nếu có đệ quy/buffer lớn nào ẩn — buf[256 bytes] cộng call stack các 
   hàm bsp_* có thể sát giới hạn, dù chưa xác nhận đây là nguyên nhân).

=== RÀNG BUỘC QUAN TRỌNG PHÁT HIỆN (liên quan bước sau, CHƯA CẦN LÀM NGAY) ===

- Modbus và USB-RS485 Bridge dùng CHUNG đúng 1 UART vật lý (USART1, chân PB14/PB15, 
  xác nhận qua RF_IO_V3.ioc) — chỉ có 1 cổng RS485 vật lý trên board.
- RS485 là bus half-duplex — về vật lý 2 chức năng KHÔNG THỂ truyền đồng thời thật 
  sự trên cùng bus (bus contention nếu cùng lúc kéo DE). Thiết kế 
  vTaskSuspend/vTaskResume giữa Modbus và Bridge (đã có sẵn trong 
  usb_rs485_enable()/disable(), xem code) là hướng đúng và BẮT BUỘC do giới hạn vật 
  lý, không phải thiếu sót.
- Hiện tại `modbus_service_init()` KHÔNG được gọi ở đâu trong app.c — Modbus chưa 
  chạy, nên hiện tại không có xung đột UART thực tế nào giữa 2 bên.
- Khi Bridge test xong (bước hiện tại), bước tiếp theo người dùng muốn là 
  thiết kế chuẩn: thêm shell command `bridge_on`/`bridge_off` gọi 
  usb_rs485_enable()/disable() để chủ động chuyển đổi, thay vì hard-code 
  s_enabled=true mãi mãi. CHỈ LÀM BƯỚC NÀY SAU KHI xác nhận bridge hoạt động đúng ở 
  mức logic cơ bản (không treo, forward dữ liệu đúng 2 chiều).

=== THỨ TỰ ƯU TIÊN CÔNG VIỆC TIẾP THEO (không nhảy bước) ===

BƯỚC HIỆN TẠI (làm ngay): tìm ra chính xác nguyên nhân treo sau dòng log 
"USB NCM network interface initialized", sửa xong, xác nhận bridge_task chạy được 
liên tục (heartbeat log tiếp tục xuất hiện, không treo).

BƯỚC TIẾP (sau khi bridge không còn treo): hướng dẫn người dùng test 2 chiều thực tế 
bằng Hercules/PuTTY trên COM Bridge — gõ dữ liệu qua USB xem có ra được chân RS485 
vật lý không (cần thiết bị RS485 thứ 2 hoặc loopback A-B để test nếu không có thiết 
bị ngoài — hỏi người dùng có thiết bị RS485 test không trước khi hướng dẫn loopback).

BƯỚC SAU NỮA (chỉ làm khi bridge OK): quay lại thiết kế bridge_on/bridge_off shell 
command đúng chuẩn (bỏ hard-code s_enabled=true).

BƯỚC CUỐI (chỉ làm khi người dùng chủ động yêu cầu): test Modbus 
(synaptix/services/modbus/modbus_service.c, modbus_service_init() hiện chưa được 
gọi ở đâu — cần tìm hiểu thiết kế trước khi tích hợp).

=== LƯU Ý VỀ CÁCH LÀM VIỆC VỚI NGƯỜI DÙNG NÀY ===

- Luôn git clone đọc code thật trước khi kết luận, không suy đoán từ tên biến/hàm.
- Khi sửa file NGOÀI synaptix/, luôn đưa số dòng cụ thể + đoạn code trước/sau rõ 
  ràng, xin phép trước.
- Khi sửa file TRONG synaptix/, có thể tự tin hướng dẫn trực tiếp không cần hỏi lại, 
  nhưng vẫn nên đưa rõ số dòng + nội dung trước/sau để người dùng dễ đối chiếu (họ 
  tự áp dụng thủ công trên VS Code, không phải lúc nào cũng để bạn sửa trực tiếp 
  qua container).
- Người dùng dùng Windows, PowerShell, `Get-PnpDevice -Class USB, Ports, Net` để 
  kiểm tra enumeration, Hercules/PuTTY để test COM port thủ công, build bằng 
  build.bat (STM32CubeCLT + Ninja).
- Việc phụ, chưa ưu tiên: đặt tên hiển thị COM port đẹp hơn "USB Serial Device" (cần 
  file .inf custom, có yêu cầu ký driver) — chỉ làm nếu người dùng chủ động hỏi lại.
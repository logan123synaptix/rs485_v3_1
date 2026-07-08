Chào bạn, tôi tiếp tục 1 project firmware embedded dở dang (STM32H523, USB↔RS485 
bridge kiêm Modbus RTU slave). Repo public: 
https://github.com/logan123synaptix/rs485_v3_1.git

ROLE: senior embedded firmware engineer (15 năm kinh nghiệm).
RULE bắt buộc: code trong `synaptix/` sửa tự do. Code NGOÀI `synaptix/` (usb/Target/, 
libs/, Board/, Core/, cmake/, netif/) phải xin phép và giải thích trước khi sửa, chờ 
xác nhận mới tiến hành.

BƯỚC ĐẦU TIÊN BẮT BUỘC: git clone repo về đọc code THẬT, không suy đoán, không tin bất 
kỳ đoạn code nào trong prompt này là bản mới nhất — code có thể đã thay đổi kể từ khi 
viết prompt này (người dùng có thể tự sửa thêm và push):
git clone https://github.com/logan123synaptix/rs485_v3_1.git

Người dùng không rành sửa code C, build bằng STM32CubeCLT + Ninja qua build.bat trên 
Windows (không có toolchain ARM trong container, không build thử được — mọi hướng dẫn 
sửa file cần đủ chi tiết dòng/nội dung để họ tự áp dụng thủ công hoặc bạn sửa trực tiếp 
qua container rồi họ tự đối chiếu diff).

=== PHẦN 1: ĐÃ HOÀN THÀNH — USB ENUMERATION (không cần động vào, chỉ để hiểu context) ===

Vấn đề gốc: board có 3 USB function (Network NCM + 2 CDC ACM: Shell + RS485 Bridge) 
nhưng Windows ban đầu chỉ thấy Network adapter, không thấy COM port.

Root cause thật (1 AI trước đó — Copilot — chẩn đoán SAI là "thiếu descriptor"):
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
- Ports: "USB Serial Device" (MI_02) = SHELL port (số COM thay đổi mỗi lần enumerate)
- Ports: "USB Serial Device" (MI_04) = BRIDGE port

CÁCH PHÂN BIỆT SHELL vs BRIDGE (không dựa tên hiển thị — Windows dùng driver mặc định 
usbser.sys, driver này LUÔN hiện tên cứng "USB Serial Device", KHÔNG đọc iInterface 
string descriptor mà firmware đặt — hành vi mặc định của usbser.inf, không phải bug):
a) MI_xx nhỏ hơn = Shell (ITF_NUM_CDC_ACM, CDC instance 0 = BSP_USB_SHELL_CH). 
   MI_xx lớn hơn = Bridge (ITF_NUM_CDC_ACM2, CDC instance 1 = BSP_USB_BRIDGE_CH).
b) Cách chắc chắn 100%: mở terminal trên COM nghi ngờ, gõ `help` — nhận lại danh sách 
   lệnh → Shell. Không phản hồi → Bridge.
c) Log debug UART (LPUART1, tách biệt hoàn toàn khỏi USB) in "waiting usb connected" 
   liên tục cho tới khi mở đúng cổng Shell.

Shell qua USB đã test OK hoàn chỉnh.

=== PHẦN 2: ĐÃ HOÀN THÀNH — USB↔RS485 BRIDGE CƠ BẢN ===

File chính: synaptix/app/user/usb_rs485/usb_rs485.c

VẤN ĐỀ 1 (đã sửa): bridge_task gọi tud_cdc_n_* trước khi tud_init() chạy xong 
(usb_netif_task, tạo trong Core/Src/app_freertos.c ~dòng 153 qua usb_netif_init(), 
chạy trong StartDefaultTask trước app_init()) → treo cứng toàn hệ thống ngay khi 
bridge_task được set enabled=true sớm. 
FIX: thêm `#include "tusb.h"`, đầu vòng lặp for(;;) trong bridge_task thêm:
```c
if (!tud_mounted()) {
    vTaskDelay(pdMS_TO_TICKS(10));
    continue;
}
```
(đặt TRƯỚC đoạn `if (!s_enabled) { ulTaskNotifyTake... }` đã có sẵn)

VẤN ĐỀ 2 (đã sửa): `bsp_uart_available(BSP_RS485)` gọi `uxQueueMessagesWaiting()` trên 
`rx_queue = NULL` vì `bsp_uart_init(BSP_RS485)` chưa từng được gọi (chỉ nằm trong 
modbus_service_init(), lúc đó chưa được gọi ở app.c) → treo cứng. FIX: người dùng đã 
thêm `modbus_service_init()` vào synaptix/app/app.c (dòng ~38, TRƯỚC 
usb_rs485_init()) — điều này khiến bsp_uart_init(BSP_RS485) được gọi đúng qua đường 
Modbus init, hết crash.

VẤN ĐỀ 3 (đã sửa): log hiển thị ký tự rác khi in buffer bridge — dùng "%s" với 
uint8_t buf[256] không đảm bảo null-terminator (chỉ nhận đúng `len` byte thật, phần 
còn lại của buffer là rác từ lần dùng trước) → đổi "%s" thành "%.*s" với (int)len làm 
precision ở CẢ 2 dòng log (USB->RS485 và RS485->USB) trong bridge_task. Dòng 
RS485->USB còn thiếu tham số TAG đầu tiên trong macro LOGI(TAG,...) (kiểm tra 
libs/logger/logger.h: `#define LOGI(TAG,...) log_func(LOGGER_INFO,TAG,__VA_ARGS__)`, 
TAG bắt buộc) — đã bổ sung TAG bị thiếu.

KẾT QUẢ ĐÃ VERIFY: log xác nhận bridge_task nhận đúng dữ liệu từ COM Bridge (USB), 
forward ra bsp_uart_transmit(BSP_RS485,...) — log in đúng nội dung gửi (ví dụ 
"AT+ROLE?", "AT+STATE?"), không còn ký tự rác.

VẤN ĐỀ CHƯA GIẢI QUYẾT LÚC BÀN GIAO TRƯỚC (đã có giải pháp, CẦN KIỂM TRA ĐÃ ÁP DỤNG 
ĐÚNG CHƯA khi nhận bàn giao — tự clone kiểm tra, KHÔNG giả định đã làm đúng):
Sau khi người dùng thêm modbus_service_init() vào app.c, VÀ usb_rs485.c đang hard-code 
`s_enabled = true` (từ bước test crash trước đó) → CẢ modbus_task VÀ bridge_task CHẠY 
SONG SONG THẬT SỰ, tranh chấp cùng UART vật lý (USART1) và cùng chân DE — KHÔNG có 
suspend/resume nào được kích hoạt vì code bypass thẳng qua s_enabled=true tĩnh, không 
đi qua usb_rs485_enable() (hàm chứa logic vTaskSuspend(modbus)).

GIẢI PHÁP ĐÃ THỐNG NHẤT VỚI NGƯỜI DÙNG — cơ chế TỰ ĐỘNG QUA DTR (khuyên dùng, người 
dùng đã chọn hướng này) + shell command dự phòng thủ công:

1. Trả `s_enabled` về `false` trong usb_rs485.c (bỏ hard-code true) — dòng 21.

2. Thêm callback DTR vào CUỐI usb_rs485.c (dùng TinyUSB callback có sẵn, weak-declared 
   trong libs/tinyusb/src/class/cdc/cdc_device.c dòng 99, override tự do không cần 
   sửa file trong libs/):
```c
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)rts;
    if (itf != BSP_USB_BRIDGE_CH) {
        return;
    }
    if (dtr) {
        usb_rs485_enable();
    } else {
        usb_rs485_disable();
    }
}
```
   Logic: mở terminal trên COM Bridge (set DTR) → tự động enable bridge + suspend 
   Modbus. Đóng terminal (clear DTR) → tự động disable bridge + resume Modbus. Không 
   cần thao tác gì thủ công trong vận hành bình thường.

3. Thêm 3 shell command dự phòng/debug trong 
   synaptix/app/user/shell/shell_commands.c: `bridge_on`, `bridge_off`, 
   `bridge_status` — gọi thẳng usb_rs485_enable()/disable()/is_enabled(). Cần thêm 
   #include "usb_rs485.h" vào đầu file. Các hàm này AN TOÀN gọi trùng với DTR callback 
   vì usb_rs485_enable()/disable() có guard `if (s_enabled) return;` / 
   `if (!s_enabled) return;` — idempotent, không xung đột giữa 2 cơ chế trigger.

BƯỚC TIẾP THEO CẦN LÀM NGAY (theo đúng thứ tự, KHÔNG nhảy bước):

BƯỚC 1: Xác nhận 3 thay đổi trên (DTR callback + s_enabled=false + 3 shell command) 
đã được áp dụng đúng trong code hiện tại trên git — tự clone kiểm tra bằng mắt, không 
tin tưởng mù quáng. Nếu người dùng báo "đã sửa" nhưng code trên git không khớp, PHẢI 
chỉ ra sai khác cụ thể (giống các lần trước trong lịch sử làm việc), không được giả 
định đúng.

BƯỚC 2: Build, nạp, test lại toàn bộ chu trình:
- Boot mặc định: Modbus chạy nền, Bridge tắt (verify qua `bridge_status` trên Shell).
- Mở COM Bridge bằng Hercules/PuTTY → xác nhận `bridge_status` tự chuyển ENABLED 
  KHÔNG CẦN gõ lệnh gì (đây là điểm mấu chốt cần verify — một số terminal có thể 
  không set DTR mặc định tùy cấu hình, cần kiểm tra thực tế, không giả định DTR luôn 
  hoạt động).
- Test 2 chiều USB↔RS485 (USB->RS485 đã verify OK qua log trước đó, RS485->USB CHƯA 
  test — cần thiết bị RS485 vật lý thứ 2 để verify chiều này, hoặc bàn cách loopback 
  A-B nếu người dùng không có thiết bị RS485 ngoài. Loopback A-B: nối chập 2 chân RS485 
  A và B với nhau bằng dây — cẩn thận không chập nhầm nguồn, chỉ nối đúng 2 chân tín 
  hiệu differential; nếu chip transceiver hỗ trợ, gõ vào COM Bridge sẽ nhận lại đúng 
  ký tự đã gõ qua đường vật lý thật, xác nhận cả 2 chiều TX/RX qua RS485 hoạt động).
- Đóng COM Bridge → xác nhận `bridge_status` tự chuyển DISABLED, Modbus resume.

BƯỚC 3 (chỉ làm khi bước 2 hoàn toàn pass): test Modbus riêng biệt. Đảm bảo 
`bridge_status` = DISABLED trước khi test (Modbus cần độc quyền UART). Dùng phần mềm 
Modbus Poll/Master (hoặc tương đương) qua 1 RS485-to-USB converter RỜI (không phải 
board này — vì Modbus đọc trực tiếp UART vật lý, không đi qua USB CDC nào của board) 
kết nối vào cùng bus RS485. Cấu hình theo comment trong modbus_service.c: RTU, slave 
address=1, baud=115200, 8N1. Cần hỏi người dùng có converter rời để làm Modbus master 
test hay không trước khi hướng dẫn chi tiết — nếu không có, cần bàn phương án khác.

BƯỚC 4 (chỉ làm khi bước 3 pass): test chuyển đổi qua lại nhiều lần liên tục 
(mở/đóng COM Bridge lặp lại trong khi Modbus đang chạy) để đảm bảo transition không để 
lại state rác — ĐẶC BIỆT CÂN NHẮC: có nên gọi `bsp_uart_flush(BSP_RS485)` (hàm đã có 
sẵn trong Board/board.c, HIỆN CHƯA được gọi ở bất kỳ đâu trong usb_rs485_enable/disable) 
ngay tại thời điểm enable/disable để xóa queue rác còn sót từ chế độ trước, tránh dữ 
liệu cũ của Modbus lẫn vào phiên Bridge mới hoặc ngược lại. Đây là điểm CẦN CÂN NHẮC 
THÊM, chưa quyết định — thảo luận với người dùng trước khi tự ý thêm.

BƯỚC 5 (chỉ làm khi người dùng chủ động yêu cầu, sau khi bridge + modbus riêng lẻ đều 
ổn định): các tính năng khác của project (nếu có, ví dụ zigbee — thấy có BSP_ZIGBEE 
trong Board/board.h nhưng chưa từng bàn tới, chưa rõ có cần test hay không, hỏi người 
dùng nếu họ đề cập).

=== LƯU Ý VỀ CÁCH LÀM VIỆC VỚI NGƯỜI DÙNG NÀY ===

- Luôn git clone đọc code thật trước khi kết luận, không suy đoán từ tên biến/hàm — 
  đã có nhiều lần trong lịch sử làm việc nơi giả định sai dẫn đến hướng dẫn sai (ví dụ 
  Copilot từng chẩn đoán sai hoàn toàn root cause ban đầu).
- Khi sửa file NGOÀI synaptix/, luôn đưa số dòng cụ thể + đoạn code trước/sau rõ ràng, 
  xin phép trước khi tự ý sửa hoặc hướng dẫn sửa.
- Khi sửa file TRONG synaptix/, có thể tự tin hướng dẫn trực tiếp, nhưng vẫn nên đưa rõ 
  số dòng + nội dung trước/sau để người dùng dễ đối chiếu (họ tự áp dụng thủ công trên 
  VS Code, build bằng build.bat trên Windows).
- Người dùng dùng Windows, PowerShell, `Get-PnpDevice -Class USB, Ports, Net` để kiểm 
  tra enumeration, Hercules/PuTTY để test COM port thủ công.
- Việc phụ, chưa ưu tiên trừ khi người dùng chủ động hỏi: đặt tên hiển thị COM port đẹp 
  hơn "USB Serial Device" (cần file .inf custom, yêu cầu ký driver, khá phức tạp cho 
  production).
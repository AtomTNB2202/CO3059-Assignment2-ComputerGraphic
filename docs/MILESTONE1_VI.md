# Mốc 1: RGB sạch và instance mask

Sau khi triển khai Mốc 2, nút export đổi thành **Export RGB + masks + COCO**.
Các ảnh Mốc 1 vẫn được xuất; xem thêm `MILESTONE2_VI.md` cho nhãn và bounding box.
Sau Mốc 3, nút có tên **Export RGB + masks + COCO + depth** và thêm dữ liệu depth.

## Mục tiêu và phạm vi

Xuất RGB và instance mask (mặt nạ từng vật thể) từ cùng cảnh, camera,
transform và độ phân giải. Chỉ lưu phần nhìn thấy của vật, có xét che khuất.
Bounding box, category, COCO và depth sẽ được thêm ở các mốc sau.
Không thêm dependency; tiếp tục dùng OpenGL, GLM, stb và nlohmann/json sẵn có.

## Chạy trên PowerShell

Đóng bản app cũ trước khi build để tránh Windows khóa executable.

```powershell
Set-Location -LiteralPath 'C:\Users\Anh Quan\Desktop\Project\CG\ComputerGraphicsAssignment1-main'
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Run
```

Trong app, **Open scene** rồi chọn
`assets/models/milestone0_occlusion.json`. Xác nhận **Open and replace**.
Cảnh này do bạn lưu có góc nhìn xiên (yaw 28°, pitch 20°); chương trình xuất
đúng camera đó. Không cần đổi sang Front / 2D trừ khi bạn muốn đổi góc nhìn.

Cuộn xuống dưới danh sách scene ở bảng bên trái, bấm **Export RGB + masks**.
Kích thước ảnh lấy từ vùng nhìn cảnh theo pixel thực của framebuffer,
mỗi chiều giới hạn tối đa 4096. Ảnh không bao gồm hai bảng điều khiển.
Xem thông báo trạng thái hoặc terminal để biết đường dẫn xuất.

```text
out/build/vs18/Debug/dataset/sample_<mã_mẫu>/
    rgb.png
    instance_ids.png
    instance_preview.png
    mask_1.png
    mask_2.png
    sample.json
```

Mỗi lần bấm tạo thư mục mẫu riêng, không ghi đè mẫu trước.
Nếu ghi file lỗi, thư mục `.partial` được giữ để phân biệt kết quả chưa hoàn tất.

Để xuất trực tiếp cảnh đã lưu ở 640×480 bằng cửa sổ OpenGL ẩn:

```powershell
.\out\build\vs18\Debug\ComputerGraphicsAssignment1.exe --export-scene `
  '.\assets\models\milestone0_occlusion.json' `
  '.\output\milestone1\my_sample_01'
```

Thư mục đầu ra phải chưa tồn tại. Đổi thành `my_sample_02` khi chạy lại.
Lệnh này cần GPU/driver OpenGL; không phải chế độ render bằng CPU.

## Đọc kết quả

| File | Ý nghĩa |
|---|---|
| `rgb.png` | Cảnh tô màu, không GUI, grid, gizmo, chữ hoặc viền chọn |
| `instance_ids.png` | RGB 8-bit mỗi kênh mã hóa ID chính xác, nền ID 0 |
| `instance_preview.png` | Màu giả chỉ để xem nhanh vùng của từng vật |
| `mask_<id>.png` | Một kênh: 255 tại phần nhìn thấy của vật, 0 ở nơi khác |
| `sample.json` | Kích thước, quy ước tọa độ, cách giải mã ID, tên vật, số pixel và file mask |

Trong PNG ID, thứ tự kênh là RGB:

```cpp
int id = R | (G << 8) | (B << 16);
```

Vật 1 có màu dữ liệu `(1,0,0)`, vật 2 có `(2,0,0)`, nên ảnh gốc trông gần như đen.
Mở `instance_preview.png` để nhìn rõ. Không dùng màu preview để giải mã ID.
Nếu đọc bằng OpenCV sau này, chú ý nó thường trả thứ tự BGR.

Mask không chứa hình dạng bị che phía sau vật khác. Vật ẩn, ngoài ảnh hoặc
bị che hoàn toàn có `visible_pixels: 0`, `mask: null`; không tạo PNG mask rỗng.
`visible` trong manifest là checkbox của editor, không phải kết luận vật đang hiện trong ảnh.
Tên `pyramid.obj` có thể giống nhau; `instance_id` mới phân biệt hai vật.
Từ Mốc 4, scene JSON lưu `instance_id` và `next_instance_id` để giữ ID khi mở lại,
kể cả khi đã xóa vật khiến dãy ID có khoảng trống. Cảnh cũ thiếu trường ID vẫn được
cấp ID theo thứ tự lúc mở. ID vật vẫn khác với category ID.

## File và hàm đã thêm hoặc sửa

| File | Thay đổi và lý do |
|---|---|
| `include/Renderer.h` | `DatasetFrame` chứa hai mảng RGB và ID; khai báo `captureDataset()` |
| `src/core/Renderer.cpp` | `captureDataset()` vẽ RGB rồi ID vào framebuffer có sẵn, đọc toàn ảnh và đảo hàng |
| `include/Application.h` | Cờ `datasetRequested`, hàm ghi mẫu và hàm xuất scene từ command line |
| `src/core/Application.cpp` | `exportSample()` ghi PNG/JSON; `exportScene()` nạp cảnh; `run()` xử lý nút export trước render editor |
| `src/gui/GuiLayer.cpp` | Nút **Export RGB + masks** ở dưới danh sách scene |
| `src/main.cpp` | Tùy chọn `--export-scene` chạy bằng cửa sổ ẩn |
| `src/core/EditorChecks.cpp` | Kiểm tra RGB/mask, che khuất, ID nhiều byte, đảo hàng, grid và selection |
| `CMakeLists.txt` | Bật `/EHsc` cho C++ trên MSVC để xử lý exception và dọn tài nguyên đúng |

Không cần thay `basic.vert` / `basic.frag`: nhánh picking hiện có đã xuất màu ID
không bị ảnh hưởng bởi ánh sáng. `Renderer::draw()` vẫn dùng cùng Model, View,
Projection trong cả hai lượt vẽ.

## Giải thích theo What → Why → Example

### Framebuffer (bộ đệm khung hình)

**What:** nơi nhận kết quả render. Framebuffer chọn vật hiện có chứa texture
màu RGBA8 và renderbuffer độ sâu 24-bit.

**Why:** cần vẽ vào bộ đệm riêng để không thu GUI hoặc trang trí editor.
Không gọi `Renderer::render()` khi xuất vì hàm đó thêm viền chọn và vẽ ra cửa sổ.

**Example:** `captureDataset()` tắt grid, vẽ RGB vào framebuffer chọn vật,
đọc RGB về RAM, rồi xóa/vẽ lại cùng bộ đệm bằng màu ID. RGB đã đọc trước khi
lượt ID ghi đè nên không mất dữ liệu. Gizmo và GUI được vẽ ở luồng editor riêng.

Luồng chính trong code:

```cpp
showGrid = false;
renderScene(surfaces, camera, 0, 0, width, height, false);
glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, frame.rgb.data());
renderScene(surfaces, camera, 0, 0, width, height, true);
glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, frame.instanceIds.data());
```

Hàm lưu và phục hồi framebuffer, viewport và các trạng thái cần thiết của editor.
Khi export qua GUI, lượt render editor kế tiếp khôi phục bộ đệm picking theo kích thước viewport.

### Object picking (chọn vật bằng pixel) và đọc toàn bộ ID

**What:** code cũ ghi ID vật vào màu, rồi `pick(x,y)` đọc một pixel để biết
click thuộc vật nào.

**Why:** mask cần biết vật ở tất cả pixel, không chỉ vị trí click.

**Example:** đổi phạm vi đọc từ `1,1` trong `pick()` sang `width,height` trong
`captureDataset()`. Với mỗi pixel, giải mã ID rồi ghi 255 vào mask đúng vật.
Hai pyramid dùng cùng mesh nhưng có ID riêng.

### Depth test (kiểm tra độ sâu)

**What:** tại pixel có nhiều bề mặt, `GL_LESS` giữ bề mặt gần camera hơn.

**Why:** mask của vật phía sau không được chiếm vùng vật phía trước đang che.

**Example:** nơi pyramid đỏ che pyramid xanh, ID map chỉ lưu ID pyramid đỏ.
Cả RGB và ID dùng cùng depth test, camera và transform; không chỉ dựa vào thứ tự vẽ.

Chapter 5: slide 42–45 (trang PDF 43–46) giải thích depth test;
slide 51–52 (trang PDF 52–53) giải thích framebuffer và texture/renderbuffer.

### Pixel origin (gốc tọa độ ảnh)

**What:** OpenGL trả hàng dưới trước, trong khi ảnh xuất dùng hàng trên trước.

**Why:** đảo riêng RGB mà không đảo ID sẽ khiến nhãn bị lật so với ảnh.

**Example:** code đảo thứ tự hàng của cả hai mảng một lần, rồi tắt đảo ảnh trong
stb khi ghi. Pixel `(x,y)` trong PNG có chỉ số byte đầu `3*(y*width+x)`.
`GL_PACK_ALIGNMENT = 1` tránh đệm hàng làm lệch RGB khi chiều rộng không chia hết cho 4.

Để giữ ID chính xác, lúc capture tắt blend, dither, multisample và sRGB.
Cả RGB và ID dùng cùng framebuffer một mẫu/pixel: cạnh có thể răng cưa,
đổi lại pixel RGB và mask khớp nhau. Wireframe được xuất thành mặt tô kín ở cả hai lượt.

## Bài kiểm tra bạn tự làm

1. Mở cảnh hai pyramid, chọn một vật, bật Grid và xuất mẫu A.
2. Mở `rgb.png`: không có grid, viền cam, tay nắm hoặc chữ.
3. So `rgb.png` với `instance_preview.png`: đỉnh và cạnh hai pyramid trùng nhau,
   không lật trên/dưới. Màu preview không cần giống màu RGB.
4. Mở `mask_1.png`: phần bị pyramid phía trước che phải đen.
5. Tắt Visible của pyramid phía trước, xuất mẫu B: phần của vật phía sau được lộ ra.
6. Mở `sample.json`: ID của vật bị ẩn có 0 pixel, mask là null.
7. Bật lại vật, bỏ chọn và tắt Grid rồi xuất mẫu C. Với cùng camera, kích thước
   và cảnh tĩnh, dữ liệu RGB/ID phải như mẫu A.

Chạy kiểm tra tự động bằng script của project:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Test
```

Test graphics dùng cửa sổ ẩn nhưng cần GPU thật. Log có dòng `DATASET PASS`.
Mốc 1 đã được build và chạy cả hai test `geometry`, `graphics_smoke` thành công.
Đã xuất thử cảnh bạn lưu vào `output/milestone1/saved_scene`, kiểm tra trực quan RGB/preview
và đọc lại PNG để đối chiếu toàn bộ mask với ID map, kích thước và số pixel trong manifest.
Thao tác bấm nút export mới qua GUI vẫn cần bạn chạy thử.

Mẫu kiểm chứng 640×480: nền 266950 pixel, instance 1 có 6485 pixel,
instance 2 có 33765 pixel. Không suy ra category từ các ID này.

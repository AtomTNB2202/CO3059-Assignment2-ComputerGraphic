# Mốc 2: bounding box, class label và COCO segmentation

Sau Mốc 3, nút export đổi thành **Export RGB + masks + COCO + depth**.
COCO và các file của Mốc 2 vẫn được giữ. Xem `MILESTONE3_VI.md` cho depth.

## Mục tiêu

Xuất bounding box (hộp bao phần nhìn thấy), class label (nhãn loại vật) và
instance segmentation (phân đoạn từng vật thể) vào `annotations.json` theo COCO.
Giữ toàn bộ RGB, ID map và mask PNG của Mốc 1. Mốc này chưa triển khai depth hoặc YOLO.

Đề §2.1 trang 2 ghi COCO hoặc YOLO; §2.2 trang 4 liệt kê cả hai.
Ta làm COCO trước theo lộ trình; cần xác nhận với giảng viên có bắt buộc cả YOLO không.

## Chạy và thao tác

Đóng app cũ trước khi build:

```powershell
Set-Location -LiteralPath 'C:\Users\Anh Quan\Desktop\Project\CG\ComputerGraphicsAssignment1-main'
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Run
```

1. **Open scene** → chọn `assets/models/milestone0_occlusion.json` → **Open and replace**.
2. Chọn lần lượt hai pyramid. Trong **Dataset label** ở inspector bên phải,
   kiểm tra `Instance ID` khác nhau; `Category ID` và `Category name` giống nhau.
3. Cảnh cũ của bạn tự nhận category `pyramid`, ID 1 khi mở bằng phiên bản mới.
   Đây là gợi ý từ tên file OBJ, không phải thuật toán nhận dạng vật.
4. Nếu cần, sửa `Category ID`/`Category name` cho tất cả vật cùng loại.
   Ví dụ hai model cốc có tên file khác nhau vẫn cần chung category `cup`.
5. **Save scene...** để lưu nhãn cùng cảnh. Cảnh cũ vẫn mở được dù chưa có trường category.
6. Bấm **Export RGB + masks + COCO** dưới danh sách scene.

Đầu ra nằm trong `out/build/vs18/Debug/dataset/sample_<mã>/`.
Ngoài các file Mốc 1 có thêm:

| File | Dùng để làm gì |
|---|---|
| `annotations.json` | COCO: ảnh, category, bbox, diện tích pixel và segmentation |
| `bbox_preview.png` | RGB có hình chữ nhật vàng để kiểm tra hộp bao |
| `sample.json` | Bổ sung category, bbox và liên kết annotation; không thay thế COCO |

`rgb.png` vẫn sạch; chỉ ảnh preview được vẽ hộp bao. Hộp bao preview không có chữ;
đọc `instance_id` và nhãn trong JSON để phân biệt các hộp.

Có thể xuất trực tiếp bằng cửa sổ OpenGL ẩn, 640×480:

```powershell
.\out\build\vs18\Debug\ComputerGraphicsAssignment1.exe --export-scene `
  '.\assets\models\milestone0_occlusion.json' `
  '.\output\milestone2\my_sample_01'
```

Thư mục đích phải chưa tồn tại. Đổi tên đích khi chạy lại.

## Instance và category: What → Why → Example

**What:** `instance_id` chỉ một vật cụ thể; `category_id` chỉ loại vật.
`category_name` là tên dễ đọc của loại vật đó.

**Why:** chương trình cần biết hai vùng pixel thuộc hai vật khác nhau nhưng
cùng loại pyramid. Dùng ID vật làm ID loại sẽ khiến mỗi pyramid trở thành một loại riêng.

**Example:** cảnh bạn lưu được nạp với:

| Vật | instance_id | category_id | category name |
|---|---|---|---|
| Pyramid phía sau | 1 | 1 | pyramid |
| Pyramid phía trước | 2 | 1 | pyramid |

`category_id` 1 chỉ có nghĩa khi tra bảng `categories`; không mặc định luôn là pyramid
trong mọi cảnh. Tên loại phân biệt chữ hoa/thường. Export từ chối một ID có hai tên
hoặc một tên có hai ID. Thông báo lỗi hiển thị ở status/terminal để bạn sửa trong inspector.

Đây là exporter một mẫu/ảnh: `images[0].id` hiện là 1 và `file_name` là `rgb.png`
tương đối với thư mục mẫu. Không nối JSON của nhiều thư mục bằng cách ghép mảng nguyên trạng:
cần quản lý ID ảnh/annotation và danh mục category chung ở Mốc 4.
Chức năng batch của Mốc 4 hiện đã tự xử lý việc này trong `annotations.json` tổng;
xem `MILESTONE4_VI.md`.

## Bounding box từ pixel: What → Why → Example

**What:** với pixel có ID của vật, lấy `xmin`, `ymin`, `xmax`, `ymax`.
Gốc ảnh ở trên trái; x tăng sang phải, y tăng xuống dưới.

**Why:** tính trực tiếp từ ID map cho hộp bao phần thực sự nhìn thấy,
đã xét che khuất và cắt theo mép ảnh.

**Example:** nếu pixel của một vật có x từ 10 đến 14, y từ 20 đến 22,
COCO bbox là `[10,20,5,3]`, vì tính cả hai pixel ở đầu và cuối:

```cpp
{b.x0, b.y0, b.x1 - b.x0 + 1, b.y1 - b.y0 + 1}
```

Một pixel có bbox rộng/cao bằng 1, không phải 0. `area` là số pixel mask,
không phải chiều rộng nhân chiều cao hộp bao.

Vật bị che hoàn toàn, bị ẩn hoặc nằm ngoài ảnh có 0 pixel:
không tạo annotation COCO, còn manifest giữ `bbox: null`, `mask: null`, `visible_pixels: 0`.
Vật lộ một phần ở mép ảnh chỉ có hộp bao phần còn thấy; không có tọa độ âm hoặc vượt ảnh.

Trong mẫu kiểm chứng 640×480 từ cảnh bạn lưu:

| instance | bbox [x,y,w,h] | area |
|---|---|---|
| 1 | [228,141,186,153] | 6485 |
| 2 | [181,182,250,262] | 33765 |

Ví dụ vật 1 có `xmax = 228+186-1 = 413`, `ymax = 141+153-1 = 293`.
Ảnh bạn export bằng GUI có thể khác độ phân giải nên số tọa độ sẽ khác.

## Segmentation COCO RLE: What → Why → Example

**What:** RLE (run-length encoding, mã hóa độ dài chuỗi) lưu số pixel 0/1 liên tiếp,
bắt đầu bằng số pixel nền, quét từ trên xuống trong mỗi cột rồi sang cột kế tiếp.
COCO nén các số đếm thành chuỗi `counts` và lưu `size: [height,width]`.

**Why:** giữ chính xác mask kể cả lỗ, vùng rời hoặc phần bị che;
không phải đơn giản hóa biên thành polygon. Không cần nhúng PNG vào JSON.

**Example:** chuỗi pixel theo thứ tự quét `[0,0,1,1,1,0,1]` có số đếm `[2,3,1,1]`.
Chuỗi `counts` trong JSON là bản nén của số đếm, không phải chuỗi các chữ số đọc trực tiếp.
Trong code, `makeCoco()` đổi thứ tự quét từ mảng row-major sang column-major:

```cpp
for (int x = 0; x < width; ++x)
    for (int y = 0; y < height; ++y)
        // Pixel: ids[size_t(y) * width + x]
```

Mỗi annotation có `id`, `instance_id` (trường bổ sung), `image_id`, `category_id`,
`bbox`, `area`, `iscrowd: 0`, `segmentation`. Đây là các vật riêng lẻ, không phải vùng crowd.
`annotations[].image_id` phải tồn tại trong `images`; `category_id` phải có trong `categories`.

Đã đối chiếu định dạng và giải mã bằng API chính thức:
[COCO mask API](https://github.com/cocodataset/cocoapi/blob/master/PythonAPI/pycocotools/mask.py),
[COCO API](https://github.com/cocodataset/cocoapi/blob/master/PythonAPI/pycocotools/coco.py).
Ứng dụng C++ tự ghi định dạng; không gọi Python hoặc công cụ tạo dataset để xuất.

## File và hàm thay đổi

| File/hàm | Thay đổi và lý do |
|---|---|
| `include/Scene.h`, `Scene::add()` | Lưu category ID/name riêng với ID vật; gợi ý category khi tạo/import |
| `SceneIO.cpp`: `saveScene()`, `loadScene()` | Lưu nhãn; mở tương thích cảnh cũ và giữ nhãn chỉnh tay |
| `GuiLayer.cpp`: `inspector()`, `gui()` | Chỉnh nhãn trong Dataset label; nút export mới |
| `include/Annotation.h`, `src/scene/Annotation.cpp`: `makeCoco()` | Hàm CPU độc lập: kiểm tra nhãn, tính bbox/area, mã hóa RLE và tạo JSON |
| `Application.cpp`: `exportSample()` | Gọi `makeCoco`, ghi annotation, preview hộp bao, cập nhật manifest |
| `tests/AnnotationTests.cpp`, `EditorChecks.cpp` | Kiểm tra hộp bao, RLE, nhãn và lưu/mở scene |
| `scripts/verify_coco.py` | Đọc lại file bằng pycocotools chính thức, đối chiếu mask/ID/box/link |
| `CMakeLists.txt` | Build module và test annotation, không thêm thư viện runtime |

## Bài kiểm tra trên máy bạn

1. Mở cảnh hai pyramid; kiểm tra hai instance khác ID nhưng chung category.
2. Export, mở `bbox_preview.png`: hộp bao phải ôm phần nhìn thấy.
3. Mở `annotations.json`: có một category pyramid, hai annotation có cùng `category_id`.
4. Tắt Visible vật phía trước rồi export: chỉ còn một annotation.
5. Để cả hai ở cùng x/y, vật phía trước gần hơn và đủ lớn để che hết vật sau:
   vật sau vẫn có thể bật Visible, nhưng không có annotation nếu không lộ pixel nào.
6. Dịch vật ra mép ảnh: bbox dừng tại mép ảnh. Một vật ra ngoài hoàn toàn không có annotation.
7. Thử đổi tên category của riêng một pyramid, giữ nguyên ID loại rồi export:
   chương trình phải báo lỗi tên không khớp. Sửa lại trước khi tiếp tục.
8. Save scene, đóng/mở lại và kiểm tra nhãn còn nguyên.

Test tự động:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Test
```

Mốc 2 đã build và vượt qua `annotations`, `geometry`, `graphics_smoke`.
Đã xuất và đọc lại bằng pycocotools cho `output/milestone2/`:
cảnh gốc (2 annotation), vật ẩn (1), che hoàn toàn (1), hai vật ở mép ảnh (2).
Mask giải mã khớp từng pixel với ID map và PNG nhị phân; bbox/area và liên kết đều đúng.
Thao tác chỉnh category và bấm nút export mới trong GUI vẫn cần bạn thử.

Nếu muốn chạy kiểm tra độc lập bằng Python, cài các gói kiểm chứng vào môi trường riêng:

```powershell
py -3.12 -m venv .venv-coco
.\.venv-coco\Scripts\python.exe -m pip install numpy Pillow pycocotools
.\.venv-coco\Scripts\python.exe scripts\verify_coco.py '.\output\milestone2\saved_scene'
```

Các gói Python chỉ phục vụ kiểm chứng file, không cần để build/chạy ứng dụng C++.

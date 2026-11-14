# Mốc 3: depth map khớp RGB

## Mục tiêu và đầu ra

Xuất depth map (bản đồ độ sâu) cùng kích thước, camera và transform với RGB/mask.
Không thêm dependency cho chương trình C++.

| File | Ý nghĩa |
|---|---|
| `depth.npy` | Mảng float32 `[height,width]`, độ sâu tuyến tính theo trục nhìn camera |
| `depth_preview.png` | Ảnh xám để xem nhanh; gần sáng, xa tối, nền đen |
| `sample.json` | Thêm mục `depth` và `camera` mô tả quy ước, near/far và khoảng giá trị |

Các file RGB/mask/COCO trước đây vẫn nằm trong cùng thư mục mẫu.
Chỉ `depth.npy` là dữ liệu số để dùng làm ground truth; preview chỉ phục vụ quan sát.

## Chạy bằng PowerShell

Đóng app cũ trước khi build.

```powershell
Set-Location -LiteralPath 'C:\Users\Anh Quan\Desktop\Project\CG\ComputerGraphicsAssignment1-main'
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Run
```

1. **Open scene** → chọn `assets/models/milestone0_occlusion.json` → **Open and replace**.
2. Kiểm tra **Visible** của cả hai pyramid đang bật nếu muốn xuất cả hai.
3. Bấm **Export RGB + masks + COCO + depth** dưới danh sách vật ở bảng trái.
4. Xem đường dẫn ở status hoặc terminal. Thư mục mẫu nằm trong
   `out/build/vs18/Debug/dataset/sample_<mã>/`, cạnh executable.
5. Mở `depth_preview.png`: pyramid gần phải sáng hơn pyramid xa, đường biên khớp `rgb.png`.

Mở thư mục mẫu vừa xuất bằng File Explorer: copy đường dẫn thư mục từ status,
dán vào thanh địa chỉ. Không tìm ảnh trong `screenshots/`.

Xuất bằng cửa sổ OpenGL ẩn, ảnh 640×480:

```powershell
.\out\build\vs18\Debug\ComputerGraphicsAssignment1.exe --export-scene `
  '.\assets\models\milestone0_occlusion.json' `
  '.\output\milestone3\my_sample_01'
```

Thư mục đầu ra phải chưa tồn tại; đổi tên khi chạy lại.

## Depth buffer: What → Why → Example

**What:** depth buffer (bộ đệm độ sâu) lưu giá trị dùng để so sánh bề mặt nào gần
camera hơn ở từng pixel. Framebuffer chọn vật của nhóm đã có depth 24-bit.

**Why:** RGB, ID và depth phải thống nhất bề mặt nhìn thấy tại mỗi pixel.
Không cần render một cảnh riêng với vị trí/camera khác để tìm độ sâu.

**Example:** sau lượt vẽ ID vào framebuffer, code đọc cả màu ID và depth:

```cpp
glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, frame.instanceIds.data());
glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, frame.depth.data());
```

Ở vùng hai pyramid chồng lên nhau, cả ID và depth thuộc bề mặt phía trước.
RGB của cùng lần capture dùng cùng hình học và camera. Animation time cũng không
thay đổi giữa các lượt vẽ trong một lần capture.

Chapter 5 slide 42–45 (trang PDF 43–46) giải thích depth test;
slide 51–52 (trang PDF 52–53) giải thích framebuffer và depth attachment.
Tham khảo thêm [OpenGL glReadPixels](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glReadPixels.xhtml).

## Near/far và depth tuyến tính: What → Why → Example

**What:** near plane (mặt phẳng cắt gần) và far plane (mặt phẳng cắt xa) giới hạn
vùng camera render. Project dùng phép chiếu phối cảnh có near `0.05`,
far `max(500, camera.distance*10)` và góc nhìn dọc 45°.
Với depth range `[0,1]`, near được ánh xạ thành 0, far thành 1 trong depth buffer.

**Why:** depth buffer của phép chiếu phối cảnh không tăng tuyến tính theo
khoảng cách. Ví dụ `d=0.99` không có nghĩa là bề mặt ở 99% khoảng near–far.
Cần đảo phép chiếu để có giá trị khoảng cách theo camera.

**Example:** với `d` là giá trị đọc từ depth buffer, `n` và `f` là near/far:

```cpp
double n = nearPlane, f = farPlane, d = bufferDepth;
float z = static_cast<float>((n * f) / (f - d * (f - n)));
```

Tương đương công thức dùng NDC (normalized device coordinates, tọa độ thiết bị chuẩn hóa)
`z_ndc = 2*d-1`, `z = 2*n*f/(f+n-z_ndc*(f-n))`.
Code dùng phép tính trung gian double để giảm sai số tính toán.

`Camera::projection()` và bước chuyển depth cùng lấy near/far từ
`Camera::nearPlane()` / `Camera::farPlane()`, tránh dùng hai bộ thông số khác nhau.

Nguồn về ánh xạ depth: [OpenGL glDepthRange](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDepthRange.xhtml).

## Quy ước dữ liệu

- `depth[y,x]` là giá trị dương dọc trục nhìn camera: `-z_view`.
- Đây là axial depth (độ sâu theo trục), không phải Euclidean range (khoảng cách thẳng
  từ tâm camera đến điểm theo tia nhìn). Hai giá trị bằng nhau ở trục giữa, khác nhau ngoài trục.
- Đơn vị là **scene units (đơn vị cảnh)**, chưa quy đổi thành mét.
  Import OBJ của nhóm chuẩn hóa model vào hộp có cạnh dài nhất bằng 2,
  nên không thể suy ra kích thước vật lý chỉ từ file depth.
- Pixel nền, vật ẩn, phần không được render: **0**. ID nền cũng bằng 0.
- Pixel vật có depth dương; không nhầm nền 0 với một bề mặt sát camera.
- Gốc tọa độ ở trên trái. Code đảo hàng của depth giống RGB/ID trước khi ghi file.
- NPY: little-endian IEEE 754 float32, row-major, shape `[height,width]`.
  Xem [đặc tả NumPy NPY](https://numpy.org/doc/stable/reference/generated/numpy.lib.format.html).

Ví dụ camera ở `(0,0,7)`, nhìn về gốc tọa độ: mặt phẳng nằm ở world Z=1
có depth 6; mặt phẳng ở world Z=-3 có depth 10. Với pyramid, độ sâu thay đổi
trên từng mặt; không lấy khoảng cách tới tâm vật làm depth của mọi pixel.

Giá trị là xấp xỉ từ depth buffer 24-bit. Dữ liệu đọc lại của hai mặt phẳng kiểm chứng
là khoảng `6.000010` và `9.999831` đơn vị. Float32 trong file không tăng độ chính xác
vốn có của depth buffer; bố trí near/far ảnh hưởng độ chính xác, nhất là cảnh rất xa.

## Đọc file và giá trị pixel

Nếu Python có NumPy, đọc trực tiếp:

```python
import numpy as np

d = np.load("depth.npy", allow_pickle=False)
print(d.shape, d.dtype)
print(d[200, 300])  # y=200, x=300; không đảo thứ tự x/y
valid = d[d > 0]
if valid.size:
    print(valid.min(), valid.max())
```

Script `scripts/inspect_depth.py` đọc file, đối chiếu vùng hợp lệ với ID map,
kiểm tra preview và in min/median/max theo từng vật.
Python chỉ cần để kiểm chứng, không cần để chạy ứng dụng C++.

Nếu chưa có môi trường kiểm chứng:

```powershell
py -3.12 -m venv .venv-depth
.\.venv-depth\Scripts\python.exe -m pip install numpy Pillow
```

Đọc mẫu test đã có, kèm một tọa độ pixel:

```powershell
.\.venv-depth\Scripts\python.exe scripts\inspect_depth.py `
  '.\output\milestone3\saved_scene' --pixel 300 250
```

Thay đường dẫn bằng thư mục `dataset/sample_<mã>` để đọc mẫu bạn vừa xuất.
Ở tọa độ nền, script sẽ báo ID 0 và depth 0.

## Preview khác dữ liệu số ra sao?

Preview tự kéo giãn khoảng min–max depth dương trong **từng mẫu**:
nền 0, bề mặt xa nhất 1, gần nhất 255. Nếu tất cả bề mặt có cùng depth thì đều 255.
Mẫu trống cho preview đen và `min_valid`/`max_valid` bằng null.

Không suy ra khoảng cách bằng cách nhìn riêng độ sáng, hoặc so độ sáng giữa hai ảnh
có khoảng min/max khác nhau. Dùng `depth.npy` để so khoảng cách; `sample.json`
ghi rõ min/max và công thức preview. Vật xa nhất có thể trông gần như đen nhưng
vẫn có depth dương; kiểm tra ID map để phân biệt với nền.

## File và hàm đã sửa

| File/hàm | Thay đổi và lý do |
|---|---|
| `Camera.h`, `Camera.cpp` | Tập trung near/far/FOV để projection và depth dùng cùng thông số |
| `Renderer.h`: `DatasetFrame` | Thêm mảng float depth và near/far dùng lúc capture |
| `Renderer.cpp`: `captureDataset()` | Đọc depth, đảo hàng, chuyển tuyến tính, đặt nền 0; lưu/phục hồi depth range và clear depth |
| `include/Depth.h`, `src/scene/Depth.cpp` | Công thức tuyến tính, ghi NPY 1.0 trực tiếp trong C++, tạo preview |
| `Application.cpp`: `exportSample()` | Ghi depth/preview, thêm quy ước và camera vào manifest |
| `GuiLayer.cpp` | Nút export thêm depth vào cùng một mẫu |
| `EditorChecks.cpp` | Kiểm tra GPU bằng hai mặt phẳng ở 6/10 đơn vị, che khuất và nền |
| `scripts/inspect_depth.py` | Đọc lại số depth và kiểm tra file ảnh/ID/preview |
| `CMakeLists.txt` | Biên dịch module depth; không thêm dependency |

## Bài kiểm tra tự làm

1. Export cảnh hai pyramid; mở RGB và depth preview cạnh nhau, kiểm tra đỉnh và cạnh khớp.
2. Vật phía trước phải sáng hơn vật phía sau trong cảnh này.
3. Tắt Visible vật trước rồi export: phần vật sau vừa lộ phải có depth của vật sau;
   phần trống còn lại phải bằng 0.
4. Để kiểm tra số với hình học đơn giản, mở
   `output/milestone3/scenes/planes.json` bằng **Open scene**. Cảnh có hai Rectangle
   tại Z=-3 và Z=1, camera ở Z=7. Giữ camera đó, export rồi chạy script đọc dữ liệu.
   Min/max của hai instance phải xấp xỉ 10 và 6, sai số nhỏ từ GPU.
5. Mở lại cảnh pyramid trước khi tiếp tục bài tập. Không lưu cảnh test đè lên cảnh pyramid.

Kiểm tra tự động:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Test
```

Đã kiểm chứng: build thành công, 3/3 nhóm test qua; log graphics có `DEPTH PASS`.
Đã đọc lại NPY bằng NumPy cho cảnh pyramid, ẩn vật, cảnh trống và hai mặt phẳng.
Depth/preview khớp pixel ID, kích thước và quy ước nền; COCO của cảnh pyramid vẫn hợp lệ.
GPU test dùng ảnh rộng 321 pixel, cao 241 pixel để bắt lỗi đệm hàng/đảo ảnh.
Thao tác bấm nút export mới trong GUI vẫn cần bạn kiểm tra.

# Kịch bản demo Assignment 2 và câu hỏi giải thích code

## Chuẩn bị

- Mở đúng thư mục chứa CMakeLists.txt. Chạy build/test trước giờ demo khi có mạng;
  FetchContent cần tải dependency ở lần cấu hình đầu.
- Giữ sẵn scene pyramid, tabletop và thư mục kết quả đã kiểm chứng.
- Trong gói source nháp, batch mẫu ở `examples/dataset/`. Trong project đang làm,
  batch đó ở `output/milestone4/batch_seed42_a/`.
- Các bước dưới là kịch bản đề xuất khoảng 7 phút, không phải quy định thời lượng của lớp.

## Kịch bản thao tác

| Thời gian | Thao tác | Điều cần nói/chứng minh |
|---|---|---|
| 0:00–0:45 | Mở app và `assets/scenes/tabletop.json` | 5 instance đọc từ OBJ; 4 category, hai cốc chung category |
| 0:45–1:30 | Chọn cốc, đổi Position/Rotation/Scale rồi trả lại | Model matrix đặt vật vào cảnh; camera/view khác transform vật |
| 1:30–2:15 | Xuất một mẫu, mở RGB/ID preview/bbox preview | RGB sạch, mask tách từng vật, bbox bám phần nhìn thấy |
| 2:15–3:15 | Mở scene hai pyramid, tắt/bật Visible vật trước, xuất lại | Che khuất do depth test; ID map và depth chọn cùng bề mặt trước |
| 3:15–4:15 | Mở depth preview, đọc giá trị từ NPY hoặc mẫu planes | Depth tuyến tính theo camera; nền 0; đơn vị cảnh; preview chỉ để xem |
| 4:15–5:30 | Mở tabletop, Generate batch 3 mẫu, seed 42 | Biến thiên camera/material/transform, cùng mã mẫu cho mọi đầu ra |
| 5:30–6:15 | Mở COCO tổng và metadata một mẫu | ID ảnh/annotation không trùng, category ổn định, camera/pose có ghi lại |
| 6:15–7:00 | Mở replay scene hoặc chỉ kết quả test đã chạy | Tái tạo một mẫu; 3 nhóm test, kiểm tra COCO và depth độc lập |

Sau khi tắt Visible cho demo, nhớ bật lại trước khi trình bày cảnh đủ hai vật.
Không lấy `Save screenshot` làm RGB dataset vì screenshot chụp cả editor.

## Lệnh dùng trong demo

```powershell
Set-Location -LiteralPath 'C:\Users\Anh Quan\Desktop\Project\CG\ComputerGraphicsAssignment1-main'
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Test
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Run
```

Đổi đường dẫn đầu tiên nếu giải nén source sang thư mục khác. Không cần build lại
ngay trong phần demo nếu executable đã được build và kiểm tra trước đó.

Batch CLI nếu cần trình bày cách chạy không mở GUI:

```powershell
.\out\build\vs18\Debug\ComputerGraphicsAssignment1.exe --export-batch `
  '.\assets\scenes\tabletop.json' '.\output\demo_batch_01' '.\assets\configs\batch_demo.json'
```

Thư mục đích phải chưa tồn tại; config mẫu sinh 8 ảnh. GUI cho phép đặt 3 ảnh để demo nhanh.

## Câu hỏi lý thuyết theo What → Why → Example

### 1. Chương trình tạo nhãn depth hay dùng AI ước lượng depth?

**What:** nhãn depth lấy từ độ sâu do chính OpenGL rasterize, rồi chuyển về camera depth.
**Why:** synthetic ground truth (nhãn chuẩn tổng hợp) phải biết giá trị từ cảnh đã dựng.
**Example:** mặt phẳng tại Z=1, camera Z=7 nhìn thẳng về gốc cho depth xấp xỉ 6.
Chương trình không chạy mạng học sâu để suy đoán depth từ RGB.

### 2. Model, View, Projection được tạo ở đâu?

**What:** Model là ma trận đặt vật; View là phép đổi sang hệ camera; Projection là phép chiếu.
**Why:** biến đỉnh từ tọa độ mesh sang vị trí trên ảnh.
**Example:** `Transform::matrix()` tạo Model; `Camera::view()` dùng `glm::lookAt`;
`Camera::projection()` dùng `glm::perspective`. `Renderer::draw()` gửi uniform;
`basic.vert` tính `projection * view * model * position`.

### 3. Vì sao object picking có thể dùng để tạo mask?

**What:** lượt picking ghi màu mã hóa ID vào texture của framebuffer.
**Why:** mỗi pixel nhìn thấy biết thuộc vật nào.
**Example:** chọn chuột đọc 1 pixel trong `pick()`; export đọc toàn ảnh trong
`captureDataset()`. Giải mã `ID = R + 256*G + 65536*B`; ID 0 là nền.

### 4. Hai cốc cùng loại thì ID có giống nhau không?

**What:** instance ID chỉ vật cụ thể; category ID chỉ loại vật.
**Why:** phải phân biệt hai mask nhưng giữ nhãn chung để nhận diện loại.
**Example:** Cup A và Cup B là instance 2/3 trong scene mẫu, cùng category 2 là cup.
ID annotation COCO tổng lại là định danh riêng trong toàn bộ dataset.

### 5. Bbox của vật bị che được tính thế nào?

**What:** lấy min/max tọa độ của pixel có ID vật, không lấy hộp chiếu của phần bị che.
**Why:** nhãn bám phần nhìn thấy theo cách xuất hiện tại.
**Example:** x từ 10 đến 14, y từ 20 đến 22 cho `[10,20,5,3]`.
Không có pixel nhìn thấy thì không tạo annotation; vật ở mép ảnh được cắt tự nhiên theo pixel.

### 6. COCO segmentation lưu ảnh PNG trực tiếp trong JSON không?

**What:** JSON lưu compressed RLE (mã hóa độ dài chuỗi) của mask.
**Why:** giữ đúng pixel, gồm cả vùng rời/lỗ, và đọc bằng COCO API.
**Example:** quét từng cột; chuỗi `[0,0,1,1,1,0,1]` có runs `[2,3,1,1]` trước bước nén.
PNG mask được lưu riêng để quan sát/kiểm chứng.

### 7. Tại sao phải chuyển depth buffer về tuyến tính?

**What:** giá trị depth buffer của phối cảnh nằm trong [0,1] và không tuyến tính theo khoảng cách.
**Why:** không thể xem giá trị 0.99 là 99% khoảng near–far.
**Example:** `z = n*f / (f - d*(f-n))`, cùng near/far dùng khi tạo Projection.
File NPY lưu `-z_view` dương; pixel nền đặt 0 dựa theo ID nền.

### 8. Preview depth có thể dùng để so khoảng cách giữa hai ảnh không?

**What:** preview kéo giãn min–max trong từng mẫu sang 1–255, nền 0.
**Why:** làm rõ khác biệt gần/xa khi xem nhưng không giữ đơn vị đo.
**Example:** hai pixel trắng ở hai ảnh có thể có độ sâu số khác nhau.
Đọc `depth.npy` để so khoảng cách; `sample.json` ghi min/max và quy ước.

### 9. Tại sao cùng seed có thể tạo lại batch?

**What:** mỗi mẫu dùng bộ sinh số dựa vào seed và index, bắt đầu từ cảnh gốc.
**Why:** tránh cộng dồn transform và tái tạo được tình huống cần kiểm tra.
**Example:** hai lần seed 42 trên build/GPU hiện tại có 116 file trùng byte;
seed 43 đổi ảnh. Camera và pose thực tế còn được lưu trong scene của mỗi mẫu.

### 10. Thư viện nào làm gì, phần nào nhóm tự xây?

**What:** GLFW tạo cửa sổ/context/input; GLAD nạp hàm OpenGL; GLM tính ma trận;
ImGui/ImGuizmo làm UI; tinyobjloader đọc OBJ; stb đọc/ghi ảnh; nlohmann/json xử lý JSON.
**Why:** tách hạ tầng khỏi pipeline render và logic annotation.
**Example:** `Renderer.cpp` tự điều khiển FBO/ID/depth; `Annotation.cpp` tự tính bbox/RLE;
`Depth.cpp` chuyển depth/ghi NPY; `DatasetBatch.cpp` quản lý batch/COCO tổng.
Python verification chỉ đọc lại kết quả, không tạo RGB hoặc nhãn cho chương trình.

## Điểm cần trả lời đúng giới hạn

- Depth là scene units sau chuẩn hóa model, không tự quy đổi thành mét.
- Chưa triển khai YOLO, PLY, tracking/captioning, bóng đổ hay va chạm vật lý.
- Model tabletop là demo đơn giản kèm source; không trình bày là scan hoặc ảnh chụp thật.
- Report cuối kỳ và repo Assignment 2 còn phải chốt. Không dùng report Assignment 1 để nộp thay.

# Mốc 4: cảnh nhiều loại vật và tạo dataset hàng loạt

## Mục tiêu

Cảnh tĩnh gồm nhiều loại vật nạp từ OBJ, có thể chỉnh thủ công và tạo nhiều mẫu.
Mỗi mẫu có RGB, ID map, mask từng vật, bounding box/COCO, depth và metadata.
Batch có COCO tổng với ID ảnh/annotation duy nhất và bảng category ổn định.
Không thêm dependency cho ứng dụng C++; giữ pipeline OpenGL/GLSL hiện có.

## Chạy và thử bằng giao diện

Đóng bản app cũ trước khi build:

```powershell
Set-Location -LiteralPath 'C:\Users\Anh Quan\Desktop\Project\CG\ComputerGraphicsAssignment1-main'
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Run
```

1. **Open scene** → chọn `assets/scenes/tabletop.json` → **Open and replace**.
2. Cảnh có bàn, 2 cốc, sách và bóng. Toàn bộ là object `Imported`, đọc từ các
   file `assets/models/tabletop/*.obj`. Hai cốc có category `cup`, ID loại 2,
   nhưng hai instance ID riêng. Các loại còn lại là `table` (1), `book` (3), `ball` (4).
3. Thử nút **Export RGB + masks + COCO + depth** trước để kiểm tra một mẫu.
4. Mở **Batch generation** ở bảng trái, phía trên danh sách object.
5. Đặt **Samples = 8**, **Seed = 42**, **Image width = 640**, **Image height = 480**.
6. Giữ các checkbox variation bật và bấm **Generate batch**.
7. Chờ tiêu đề cửa sổ/terminal đếm đủ mẫu. Đường dẫn cuối nằm ở status và terminal.
8. Sau khi xong, cảnh, camera và ánh sáng trong editor được phục hồi.

Batch nằm cạnh executable:

```text
out/build/vs18/Debug/dataset/batch_<mã>/
    annotations.json          COCO tổng nhiều ảnh
    dataset.json              danh sách mẫu, seed và cấu hình
    settings.json             cấu hình batch để chạy lại
    base_scene.json           cảnh gốc trước randomization
    sample_000001/
        rgb.png
        instance_ids.png
        instance_preview.png
        mask_<instance_id>.png
        bbox_preview.png
        annotations.json      COCO riêng một mẫu
        depth.npy
        depth_preview.png
        sample.json           camera, vật, material, số pixel, seed/index
        scene.json            cảnh có thể mở lại trong app
    sample_000002/
    ...
```

Không trộn ảnh từ các thư mục sample khác nhau. COCO tổng tự tham chiếu đúng
`sample_000001/rgb.png`… Mọi đầu ra của một mẫu nằm trong cùng thư mục mang mã mẫu.
Chỉ thư mục batch hoàn tất mới bỏ hậu tố `.partial`.

Các OBJ là model đơn giản do script trong project tạo, không phải thư viện dataset
hoặc engine tạo nhãn. Python chỉ cần nếu muốn tái tạo asset; file OBJ đã được kèm sẵn.
Đây là cảnh demo kỹ thuật, chưa có bóng đổ hoặc mô phỏng va chạm.

## Randomization: What → Why → Example

**What:** randomization (biến thiên ngẫu nhiên) thay các thông số của cảnh để có
nhiều ảnh. Seed (giá trị khởi tạo bộ sinh số) quyết định chuỗi biến thiên.

**Why:** chỉ xuất đi xuất lại cùng cảnh sẽ tạo ảnh trùng. Seed giúp tái tạo mẫu
khi cần giải thích một lỗi nhãn hoặc cảnh che khuất.

**Example:** cùng cảnh, asset, cấu hình, seed 42 và binary/GPU hiện tại tạo lại
cùng batch. Đổi seed 43 tạo góc camera, màu, texture, vị trí hoặc nền khác.
Không cam kết file giống từng byte giữa các GPU/driver hoặc bản build khác nhau.

`makeBatchSample()` tạo mỗi mẫu từ bản sao cảnh gốc:

```cpp
std::seed_seq key{static_cast<unsigned>(s.seed), static_cast<unsigned>(index)};
std::mt19937 random(key);
auto result = base;
```

Không lấy mẫu trước làm cảnh gốc của mẫu sau, nên scale và vị trí không bị cộng dồn.
Mẫu số 1 ứng với index 0; cặp seed/index được ghi trong `sample.json`.
Animation đóng băng tại thời điểm cảnh gốc để batch là cảnh tĩnh.

## Các thông số có thể thay đổi

| Điều khiển | Hành vi mặc định |
|---|---|
| Vary transforms | Dịch X/Z ±0.2 đơn vị, yaw quanh Y ±20°, scale đồng đều ±10% |
| Vary colors/materials | Biến thiên màu quanh màu gốc và shininess |
| Vary textures | Chọn không texture hoặc một ảnh từ texture pool |
| Vary camera | Yaw ±12°, pitch ±5°, khoảng cách ±8% quanh camera gốc |
| Vary lighting | Vị trí đèn thay đổi ±2 theo mỗi trục |
| Vary background | Màu nền thay đổi theo từng mẫu |
| Variation ranges | Chỉnh các khoảng vị trí/xoay/scale/camera trong giới hạn hỗ trợ |

Trong inspector của từng vật có **Randomize transform in batch**.
Bàn trong scene mẫu đã tắt checkbox này để giữ nguyên vị trí/kích thước.
Checkbox này chỉ khóa transform; màu hoặc texture vẫn có thể thay đổi.
Khi đổi scale/yaw của vật khác, code giữ độ cao đáy bounding box thế giới của vật.
Đây là cách giữ vật trên mặt bàn; không phải mô phỏng vật lý hoặc kiểm tra va chạm.
Nên kiểm tra một batch nhỏ trước khi tăng số lượng hoặc khoảng biến thiên.

Các góc xoay X/Z, vị trí Y và texture riêng vẫn có thể chỉnh thủ công như Assignment 1.
Nếu muốn giữ texture chỉnh tay, tắt **Vary textures**. GUI mặc định dùng pool checker PNG/JPG
có sẵn; để dùng ảnh khác cho batch, chỉnh `texture_pool` trong JSON cấu hình rồi chạy CLI.

Batch hỗ trợ 1–1000 mẫu, mỗi chiều ảnh 1–2048 pixel, seed 0–1000000.
Độ phân giải batch lấy từ các ô Image width/height, độc lập kích thước viewport editor.
Mọi RGB/mask/depth trong một mẫu vẫn có cùng kích thước và camera.
Nếu vật bị che hoàn toàn/ra ngoài ảnh, nó có 0 pixel trong metadata và không có
annotation nhìn thấy. Bộ sinh hiện giữ các mẫu đó, không tự loại ảnh hoặc cố bảo đảm
mọi vật luôn hiện; các test che khuất Mốc 1–3 vẫn áp dụng.

## Metadata và mở lại mẫu: What → Why → Example

**What:** metadata (dữ liệu mô tả) ghi camera, cấu hình render và pose (vị trí,
góc xoay, scale) của từng vật; `scene.json` chứa cảnh có thể nạp lại.

**Why:** cần biết ảnh được tạo từ cấu hình nào, thay vì chỉ lưu PNG và mất thông tin.

**Example:** mở `sample_000003/scene.json` trong app để nhìn lại mẫu số 3.
Metadata có position/target/FOV của camera, vị trí đèn, màu nền, material,
model/texture path, ID, category, pose và số pixel nhìn thấy của từng vật.

Mốc 4 cũng lưu instance ID vào scene, gồm cả khoảng trống do xóa object; ID không
còn tự đổi khi mở lại scene mới. Cảnh cũ thiếu ID vẫn được cấp theo thứ tự lúc nạp.
Đường dẫn OBJ/texture trong scene là tham chiếu ngoài; muốn mở lại trên máy khác
cần mang theo asset và giữ/sửa đúng đường dẫn. RGB, mask, depth và COCO dùng độc lập
với asset gốc cho việc đọc dữ liệu.

## COCO tổng: What → Why → Example

**What:** mỗi mẫu có COCO riêng với image ID 1. Khi ghép batch, `appendCocoSample()`
cấp lại ID ảnh/annotation toàn cục, giữ nguyên category và instance ID bên trong mẫu.

**Why:** nối thẳng các mảng COCO một ảnh sẽ tạo ID trùng và nhãn trỏ sai ảnh.

**Example:** hai cốc ở ảnh 1 và ảnh 2 vẫn cùng `category_id=2`; annotation của ảnh 2
có `image_id=2`, ID annotation mới, và file ảnh là `sample_000002/rgb.png`.
Instance ID có thể lặp giữa các ảnh vì chỉ xác định vật trong scene, còn annotation ID
trong COCO tổng phải duy nhất. Bảng categories được kiểm tra giống nhau trong cả batch.

## Chạy batch bằng PowerShell/CLI

Cấu hình mẫu có sẵn ở `assets/configs/batch_demo.json`:

```powershell
.\out\build\vs18\Debug\ComputerGraphicsAssignment1.exe --export-batch `
  '.\assets\scenes\tabletop.json' `
  '.\output\my_dataset_01' `
  '.\assets\configs\batch_demo.json'
```

Thư mục đích phải mới. Đường dẫn ảnh trong `texture_pool` tính tương đối với file
config; config xuất lại trong batch dùng đường dẫn tuyệt đối đã resolve trên máy này.
Để tạo lại cấu hình đã xuất, dùng `base_scene.json` và `settings.json` của batch đó:

```powershell
.\out\build\vs18\Debug\ComputerGraphicsAssignment1.exe --export-batch `
  '.\output\my_dataset_01\base_scene.json' `
  '.\output\my_dataset_repeat' `
  '.\output\my_dataset_01\settings.json'
```

Hoặc mở cảnh từ `base_scene.json` bằng GUI và dùng lại thông số giống batch cũ.
Settings của panel GUI không tự lưu cùng scene; file `settings.json` là bản cấu hình đầy đủ.

## File/hàm thay đổi

| File/hàm | Thay đổi và lý do |
|---|---|
| `include/Batch.h`, `src/scene/Batch.cpp` | Cấu hình, kiểm tra giới hạn, đọc JSON, biến thiên theo seed/index |
| `src/core/DatasetBatch.cpp`: `exportBatch()` | Chụp nhiều mẫu, cache texture, ghép COCO, ghi index và phục hồi editor |
| `Annotation.cpp`: `appendCocoSample()` | Remap ID ảnh/annotation và kiểm tra category chung |
| `Application.cpp`: `exportSample()` | Thêm pose/material/render metadata và `scene.json` |
| `Application.h`, `GuiLayer.cpp`, `main.cpp` | Panel batch, khóa transform từng vật, lệnh `--export-batch` |
| `Scene.h`, `SceneIO.cpp` | Lưu khóa transform, instance ID và next ID; tương thích scene cũ |
| `scripts/create_tabletop_assets.py`, `assets/models/tabletop/` | Tạo và kèm 4 loại model OBJ nguyên gốc |
| `assets/scenes/tabletop.json`, `assets/configs/batch_demo.json` | Cảnh và cấu hình sẵn để chạy |
| `scripts/verify_dataset.py`, các test hiện có | Đối chiếu COCO tổng, các file mẫu, seed/pose và phục hồi cảnh |

## Kiểm chứng đã chạy và bài thử của bạn

Đã build và vượt 3/3 nhóm test. Test graphics kiểm tra seed/index, transform khóa,
độ cao đáy, ID/category, lưu/mở scene và phục hồi editor sau export batch.
Đã xuất hai batch seed 42 (mỗi batch 8 ảnh, 4 category, 40 annotation) và một batch
seed 43 (3 ảnh, 15 annotation). Đã dùng pycocotools đọc COCO và so mask/ID/depth từng pixel.
Toàn bộ 116 file của hai batch seed 42 giống nhau từng byte; seed 43 đổi ảnh.
Các kết quả nằm ở `output/milestone4/` để bạn xem, không cần tạo lại ngay.

Bạn tự thử:

1. Mở tabletop, tạo 8 mẫu như hướng dẫn. Kiểm tra `sample_000001` và `sample_000002`
   có góc nhìn/màu/texture khác nhau, nhưng mỗi RGB vẫn khớp mask và depth riêng.
2. Mở `annotations.json` ở gốc batch: có 8 phần tử images, 4 categories;
   annotation liên kết tới ID ảnh tồn tại.
3. Tạo lại với cùng seed/cấu hình/cảnh gốc. Đổi seed rồi so sự khác nhau.
4. Kiểm tra bàn không di chuyển sau batch và cảnh editor vẫn như trước khi Generate.
5. Mở scene của một mẫu, export một ảnh riêng ở cùng độ phân giải để kiểm tra tái tạo.

Kiểm tra tự động:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Test
```

Kiểm chứng file bằng môi trường Python tùy chọn đã dùng ở Mốc 2:

```powershell
.\.venv-coco\Scripts\python.exe scripts\verify_dataset.py `
  '.\output\milestone4\batch_seed42_a'
```

Nếu chưa có môi trường đó:

```powershell
py -3.12 -m venv .venv-coco
.\.venv-coco\Scripts\python.exe -m pip install numpy Pillow pycocotools
```

Python chỉ phục vụ kiểm chứng hoặc tái tạo asset, không được gọi để render/tạo nhãn.
Thao tác panel batch qua GUI vẫn cần bạn chạy thử. Mốc 5 sẽ đối chiếu rubric,
chuẩn bị demo và tài liệu nộp; report cuối kỳ bằng LaTeX chưa được viết trong mốc này.

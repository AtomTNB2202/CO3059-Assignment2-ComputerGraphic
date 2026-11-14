# Mốc 5: đối chiếu rubric, demo và chuẩn bị nộp bài

## Trạng thái

Các chức năng Mốc 0–4 đã build/test và được người dùng xác nhận thao tác GUI.
Mốc này chuẩn bị tài liệu/demo/source package nháp. Báo cáo Assignment 2 cuối kỳ
tiếng Anh bằng LaTeX chưa được viết; đợi yêu cầu viết report của người dùng.

Nguồn yêu cầu: `cgAssignment2.pdf`, v1.2, §2–3, trang 2–6. Hạn Assignment 2 trong
PDF là 06/12/2026 (§3.1 trang 5); cần theo thông báo LMS nếu lớp cập nhật.
Chưa truy cập được thông báo lớp/LMS trong phiên này.

## Bảng đối chiếu rubric với bằng chứng

Các trọng số bên dưới cộng thành 50% trong bảng điểm hai assignment (§3.2),
không phải kết quả chấm điểm của giảng viên.

| Mục trong đề | Trọng số | Chức năng/bằng chứng đã có | Trạng thái |
|---|---:|---|---|
| Scene Design and Rendering, trang 5 | 15% | Cảnh tabletop có 5 vật/4 loại đọc từ OBJ; chỉnh transform, texture, ánh sáng, camera | Đã chạy và xác nhận GUI |
| Ground Truth Label Generation, trang 5–6 | 15% | ID/mask nhị phân, bbox phần nhìn thấy, category, COCO RLE; pycocotools đọc khớp pixel | Đã kiểm chứng; cách hiểu COCO/YOLO cần hỏi |
| Depth Estimation Labels, trang 6 | 10% | NPY float32 camera depth, preview, cùng kích thước RGB; test mặt phẳng 6/10 đơn vị | Đã kiểm chứng |
| Usability and Dataset Export, trang 6 | 10% | GUI/CLI, batch seed, mã mẫu, COCO tổng, metadata camera/pose, scene replay | Đã chạy và xác nhận GUI |

§2.2 trang 3 nói `.obj` **hoặc** `.ply`: bản hiện tại chọn OBJ. Tracking/captioning
nằm ở phần optional (§2.3 trang 4) và chưa triển khai.

## Các điểm cần xác nhận với giảng viên

1. **COCO/YOLO:** §2.1 trang 2 ghi COCO hoặc YOLO; §2.2 trang 4 liệt kê COCO,
   YOLO và custom schema. Bản hiện tại có COCO; cần hỏi có bắt buộc cả YOLO không.
2. **Metadata:** §2.1 ghi optional nhưng rubric trang 6 yêu cầu. Đã triển khai
   camera, pose, material, render config, seed/index nên có bằng chứng cho phần này.
3. **Thư viện:** trang 6 nêu OpenGL/GLFW/GLEW/GLM và một số thư viện khác;
   chưa nêu rõ GLAD, Dear ImGui, ImGuizmo, stb, tinyobjloader, nlohmann/json.
   Hỏi quyền dùng các support library này, mô tả đúng vai trò theo THIRD_PARTY.md.
4. **Depth:** bản hiện tại công bố axial camera Z, scene units, nền 0; nếu lớp
   quy định Euclidean range hoặc đơn vị vật lý thì cần điều chỉnh theo yêu cầu đó.

Theo hướng dẫn của người dùng, hiện bám cách đọc §2.1 là chọn COCO và giữ thư viện
phụ đúng vai trò đã công bố. Không có xác nhận riêng của giảng viên trong phiên này;
không mô tả cách đọc này như lời duyệt chính thức hoặc tự ghi YOLO là đã làm.

## Chuẩn bị demo

Làm theo `DEMO_ASSIGNMENT2_VI.md`: kịch bản khoảng 7 phút, câu hỏi What → Why → Example,
và lệnh dùng khi trình bày. Thử một batch nhỏ trước, rồi mở COCO/metadata/depth.

Trước khi demo, tự kiểm tra:

- [ ] Build từ đúng thư mục CMakeLists.txt; GPU chạy OpenGL 3.3.
- [ ] Mở tabletop, thấy bàn/2 cốc/sách/bóng; category cốc giống nhau.
- [ ] RGB không có editor; mask và bbox đúng phần nhìn thấy.
- [ ] Tắt/bật vật trước làm thay đổi mask/depth đúng che khuất.
- [ ] Đọc được số trong NPY, giải thích scene units và preview normalization.
- [ ] Generate batch, mở hai mẫu khác nhau và COCO tổng.
- [ ] Mở scene của một mẫu để xem lại camera/pose.
- [ ] Biết file/hàm chính để giải thích, thay vì chỉ bấm nút.

## Báo cáo và các thành phần nộp

Theo §3 trang 4:

- [ ] Report Assignment 2 **tiếng Anh**, LaTeX source `.tex` và PDF đã biên dịch.
- [ ] Nội dung: kiến trúc/thành phần, tính năng, hướng dẫn dùng, ảnh kết quả thực tế,
  link repo GitHub chứa đúng source hiện tại.
- [ ] Full source, hướng dẫn dependency/cấu hình và các model/texture cần để chạy.
- [ ] Báo cáo tiến độ theo §3.1: trạng thái triển khai, ảnh và khó khăn.
- [ ] Xác nhận tên thành viên/MSSV, repo Assignment 2 và quy định LMS của lớp.

`PROGRESS_ASSIGNMENT2.md` là bản ghi nội bộ để chuẩn bị báo cáo tiến độ.
`report/report.tex` và `report/report.pdf` đang có là **Assignment 1**, không dùng
để nộp thay Assignment 2. Gói source nháp loại các file report cũ này.

## Tạo gói source nháp

Từ thư mục gốc:

```powershell
python scripts/package_assignment2.py --evidence output/milestone4/batch_seed42_a
```

Kết quả mặc định: `output/submission/CGAssignment2-source-draft.zip`.
Script không ghi đè file zip cũ; muốn chạy lại, chọn tên mới:

```powershell
python scripts/package_assignment2.py --evidence output/milestone4/batch_seed42_a `
  --output output/submission/CGAssignment2-source-draft-v2.zip
```

Gói gồm source, CMake, scripts, docs, shaders, assets, license GLAD/THIRD_PARTY,
batch mẫu 8 ảnh, trạng thái phần còn thiếu và manifest SHA-256 từng file.
Không gồm build caches/dependency downloads, môi trường Python hoặc report Assignment 1.
Đường dẫn asset trong bản sao evidence được đổi cho phù hợp cấu trúc zip;
các file RGB/mask/depth không bị thay đổi.

File `.zip` có thể giải nén vào thư mục mới rồi mở `CGAssignment2/CMakeLists.txt`.
Build lần đầu vẫn cần mạng cho FetchContent và Visual Studio C++ workload.
Lệnh build không đổi:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Test
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Run
```

Trong bản giải nén, chạy từ thư mục `CGAssignment2`.
Các kiểm tra build/GPU được thực hiện trên checkout đang làm; không khẳng định
đã thử gói source trên một máy Windows mới hoặc tài khoản khác.

## Khi nào có thể nộp cuối kỳ?

Khi đã có report Assignment 2 TEX/PDF, repo chứa đúng thay đổi, các câu hỏi về
format/thư viện được xử lý và bộ file đáp ứng hướng dẫn LMS. Gói source hiện tại
là bản nháp để xem lại và tập demo. Repo do người dùng chỉ định:
https://github.com/AtomTNB2202/CO3059-Assignment2-ComputerGraphic. Chưa nộp LMS.

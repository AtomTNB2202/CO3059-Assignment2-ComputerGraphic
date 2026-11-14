# Bản ghi tiến độ Assignment 2

Ngày đối chiếu: 14/11/2026. Đây là ghi nhận triển khai để chuẩn bị báo cáo tiến độ,
không thay thế báo cáo cuối kỳ bằng tiếng Anh/LaTeX. Tên nhóm chưa được xác nhận;
repo đã chọn: https://github.com/AtomTNB2202/CO3059-Assignment2-ComputerGraphic.
Không suy ra thành viên từ tên tài khoản Windows.

## Đã triển khai

- C++17, Modern OpenGL 3.3 Core, GLSL; tái sử dụng camera, renderer, mesh và scene Assignment 1.
- Cảnh nhiều vật nạp từ OBJ; scene mẫu bàn, 2 cốc, sách, bóng: 5 instance, 4 category.
- Xuất RGB sạch, ID map 24-bit, mask nhị phân theo phần nhìn thấy, bbox và class label.
- COCO JSON có compressed RLE, area, bbox và liên kết ảnh/category/annotation.
- Depth float32 NPY cùng độ phân giải RGB; axial camera depth, scene units, nền 0.
- Chỉnh scene thủ công và sinh batch có seed; biến thiên transform, camera, màu/material,
  texture, đèn và nền. Giữ bảng category và cấp ID COCO toàn cục cho batch.
- Metadata và replay scene: camera, pose vật, material, light/background, seed/index.
- Build/run/test trên Windows; người dùng đã xác nhận GUI các Mốc 0–4.

## Bằng chứng kiểm chứng

- 3/3 nhóm CTest qua: annotations, geometry, graphics_smoke.
- Depth hai mặt phẳng: kỳ vọng 6 và 10 đơn vị, đọc lại khoảng 6.000010 và 9.999831.
- Batch seed 42: 8 ảnh, 4 category, 40 annotation, mỗi ảnh có 5 vật nhìn thấy.
- Hai lần cùng seed 42: 116 file giống từng byte trên cùng build/GPU.
- Seed 43 thay đổi RGB; batch kiểm chứng có 3 ảnh, 15 annotation.
- pycocotools giải mã COCO khớp pixel ID/mask; NumPy đọc NPY khớp kích thước và nền.
- Mở lại scene một mẫu tái tạo đúng RGB, ID map, depth và COCO.
- GPU ghi nhận trong log: NVIDIA GeForce RTX 4060 Laptop GPU, OpenGL 3.3.

Kết quả local: `output/milestone1` đến `output/milestone4`; log CTest ở
`out/build/vs18/Testing/Temporary/LastTest.log`; log kiểm chứng batch ở
`output/milestone4/verification.log`. Gói source nháp kèm batch 8 ảnh tại `examples/dataset/`.

## Còn lại trước khi nộp

1. Ghi đúng phạm vi định dạng: hiện chọn COCO theo §2.1 "COCO or YOLO"; chưa có YOLO.
   Giữ danh sách vai trò thư viện phụ, không mô tả engine black-box là tự triển khai.
2. Xác nhận thành viên/MSSV cho report và duy trì source trong repo đã chọn.
3. Viết report cuối kỳ tiếng Anh, LaTeX: kiến trúc, giải thích chức năng, hướng dẫn chạy,
   ảnh kết quả thực tế, link GitHub; biên dịch và kiểm tra đủ `.tex` và `.pdf`.
4. Tập demo theo `DEMO_ASSIGNMENT2_VI.md`, chạy gói source trên môi trường nộp/demo,
   rồi nộp đủ thành phần lên LMS. Chưa nộp LMS.

## Giới hạn hiện tại

OBJ được hỗ trợ; PLY chưa có. COCO đã có, YOLO chưa có. Cảnh tĩnh, không tracking
hoặc captioning. Không mô phỏng va chạm/bóng đổ; model demo đơn giản do project tạo.
Depth tính bằng scene units sau chuẩn hóa OBJ, chưa quy đổi ra mét. Tái tạo từng byte
được kiểm chứng trên cùng build/GPU; driver hoặc nền tảng khác có thể rasterize khác.

# Export và preview

## Cửa sổ Export

Bấm **Export...** trên toolbar hoặc dưới Scene list. Chọn cảnh hiện tại hoặc Batch,
chọn thư mục và các đầu ra cần lưu, rồi bấm **Export**.

Khung **Image preview** bên phải render trực tiếp trong bộ nhớ, chưa ghi file.
Chọn RGB, Depth, Segmentation hoặc Bounding boxes để kiểm tra trước khi xuất.
Ảnh dùng đúng Width/Height của tác vụ xuất và chỉ thu nhỏ khi hiển thị.
Trong Batch, chọn **Sample** để xem đúng biến thiên của mẫu đó theo seed hiện tại.
**Refresh preview** dựng lại ảnh. Animation tạm dừng khi cửa sổ Export mở.

Thiết lập batch (số mẫu, seed, kích thước và biến thiên) vẫn ở **Scene > Batch generation**.
Nút **Generate batch...** mở cửa sổ Export với Batch được chọn sẵn.

**Assignment 2 / all outputs** bật lại toàn bộ đầu ra. COCO và YOLO luôn đi kèm RGB.
Metadata `sample.json` và cảnh `scene.json` luôn được lưu. Đầu ra bỏ chọn có đường
 dẫn `null` trong manifest; chúng không được ghi ra ổ đĩa. `verify_dataset.py`
kiểm tra preset đầy đủ, không dành cho tập đầu ra rút gọn.

Mục **Extra PNG files (saved to disk)** chỉ chọn các ảnh PNG phụ cần ghi ra đĩa;
không liên quan tới việc bật/tắt preview trong cửa sổ.

Mỗi lần xuất tạo thư mục mới `sample_...` hoặc `batch_...`. Khi hoàn tất, dùng
**Open folder** hoặc **Open first image**. Hủy batch giữ dữ liệu chưa hoàn thành
trong thư mục `.partial` và khôi phục cảnh editor.

## Chế độ hiển thị viewport

Trong **Inspector > Scene view**, chọn:

| Chế độ | Nội dung |
|---|---|
| Scene / edit | Chỉnh sửa bình thường: grid, selection, gizmo |
| RGB | Ảnh cảnh sạch, không có grid/gizmo |
| Depth | Gần sáng, xa tối, nền đen |
| Segmentation | Mỗi instance một màu ổn định |
| Bounding boxes | RGB kèm khung bao phần nhìn thấy của từng vật |

Viewport cập nhật theo cảnh và camera hiện tại. Vẫn dùng orbit/pan/zoom hoặc WASD;
chuyển về **Scene / edit** để click chọn vật thể và kéo gizmo. Có thể chọn vật trong
Scene list và chỉnh inspector khi xem các chế độ khác.

Viewport dùng tỷ lệ khung nhìn; preview trong Export dùng đúng tỷ lệ ảnh xuất.
Muốn chỉnh sửa lại cảnh đã lưu, **Open scene** tới `sample_.../scene.json`.
Depth preview được chuẩn hóa riêng theo khoảng sâu đang nhìn thấy; giá trị số thực
camera-forward Z nằm trong `.npy` và tính bằng scene units.

## Xuất YOLO detection

Bật **YOLO detection labels (.txt)** trong Export. Kết quả có thư mục `yolo/`:

- `images/`: ảnh RGB; `labels/`: nhãn `.txt` cùng tên với ảnh.
- Mỗi dòng: `class_index x_center y_center width height`, tọa độ chuẩn hóa 0–1.
- `classes.json`: ánh xạ mã lớp 0, 1, ... sang category ID và tên gốc. Ánh xạ
  sắp theo category ID và giữ nguyên trong cả batch, kể cả lớp không hiện trong ảnh.
- `dataset.yaml`: đường dẫn ảnh và tên lớp. Chưa chia train/validation;
  cần tạo tập validation riêng và điền `val` trước khi huấn luyện. Nếu chuyển
  thư mục sang máy khác, sửa `path` trong YAML.

Bbox chỉ bao phần nhìn thấy; vật bị che hoàn toàn không có dòng nhãn. Ảnh không
có vật nhìn thấy có file nhãn rỗng. Đây là nhãn detection; segmentation vẫn dùng
COCO/mask. Batch có thư mục YOLO chung ở gốc; từng sample cũng có bản riêng.

Định dạng theo [Ultralytics YOLO detection](https://docs.ultralytics.com/datasets/detect/).

## Đọc giá trị depth

Rê chuột lên ảnh trong Export preview, Dataset viewer hoặc viewport khi chọn
**Inspector > Scene view > Depth**. Tooltip hiển thị pixel `(x, y)` (gốc trên trái)
và depth Z theo scene units. Depth là độ sâu theo trục nhìn của camera, không phải
khoảng cách Euclid; không mặc định là mét. Nền có giá trị 0 và được ghi rõ
**Background / no surface**. Viewport đo tại độ phân giải preview (tối đa 1024
pixel cạnh dài); Export preview và Dataset viewer dùng độ phân giải ảnh xuất.

## Xem dataset đã lưu

Bấm **Dataset...** trên toolbar, rồi **Open dataset folder...** và chọn thư mục
chứa `dataset.json` (batch) hoặc `sample.json` (một sample). Sau khi export cũng
có nút **View dataset**. Dùng **Previous**, **Next** hoặc thanh **Sample** để đổi ảnh;
**View** chọn RGB, Depth, Segmentation hoặc Bounding boxes.

Trình xem đọc dữ liệu trên đĩa, không thay cảnh đang chỉnh sửa. Depth số đọc trực
tiếp từ NPY float32; nếu chỉ lưu PNG depth thì xem được màu nhưng không có số đo.
Đầu ra đã bỏ chọn khi xuất được ghi rõ là không có. Trình xem hỗ trợ cấu trúc
dataset của app, không phải trình duyệt dataset YOLO/COCO bất kỳ.

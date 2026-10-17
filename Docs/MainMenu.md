# หน้าเริ่มเกม

แผนที่เริ่มต้นคือ `/Game/Maps/MainMenu` ใช้ `MainMenuGameMode` และ UI C++ `MainMenuWidget` ไม่ต้องสร้าง Widget Blueprint เพิ่ม

- ชื่อเกมชั่วคราว `PLACEHOLDER` ปรับได้ใน `MainMenuWidget.cpp` ที่ข้อความ `Title`
- ธีมสีเข้ม–ทองจาก `LootUITheme.h` พื้นหลังตาราง Blueprint พร้อมเส้นหลัก/ย่อยและเส้นสแกนเบา ๆ จัดชื่อเกมขนาด 76 กับปุ่ม PLAY GAME / QUIT ขนาด 28 เรียงชิดซ้าย เว้นพื้นที่ฝั่งขวา ไม่มีกรอบรอบกลุ่มเมนูหรือขีดใต้ชื่อ ปุ่ม Play ใหญ่กว่า Quit พร้อมเอฟเฟกต์ hover/focus ไม่มีข้อความเสริมหรือแปลนอาคารด้านขวา
- `Play Game` เปิด `/Game/Office_Pack_Vol_1/Maps/DemoMap` โดยใช้ `BP_ArenaGameMode` ของโปรเจกต์ และคืนการควบคุมเมาส์ให้เกม
- `Quit` ปิดเกม ใน PIE จะหยุดรอบทดสอบ
- ด้านขวามีภาพประตูเซฟแบบ Blueprint วงรหัสและมือหมุนเคลื่อนไหวช้า ๆ พร้อมแสงสแกน ไม่มีข้อความเพิ่ม และซ่อนภาพเมื่อพื้นที่จอไม่พอเพื่อไม่ให้ทับชื่อหรือเมนู
- ใช้เมาส์ หรือปุ่มขึ้น/ลงแล้วกด Enter/Space เพื่อเลือกได้
- ป้องกันการกด Play ซ้ำระหว่างโหลด และแสดงข้อผิดพลาดหากหาแผนที่ไม่พบ

เปิด MainMenu ใน Unreal แล้วกด Play เพื่อทดลอง หากอยู่ในแผนที่อื่น การกด Play จะเริ่มจากแผนที่ที่เปิดอยู่ตามพฤติกรรมปกติของ Editor

`GameDefaultMap` และ `EditorStartupMap` ใน `DefaultEngine.ini` ตั้งเป็น MainMenu แล้ว ส่วน `DefaultGame.ini` ระบุ MainMenu, DemoMap และ Lobby สำหรับการ cook แผนที่

`Scripts/CreateMainMenuMap.py` ใช้สร้างแผนที่เมนูหลัง Build C++ ไฟล์แผนที่ถูกสร้างไว้แล้ว ไม่ต้องรันสคริปต์ก่อนเล่น

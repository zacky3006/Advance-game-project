# สลับกล้องด้วย C

ตัวละครที่สืบทอดจาก `ArenaGameCharacter` กด C เพื่อสลับ Third Person → First Person และกดซ้ำเพื่อกลับ โดยไม่ต้องสร้าง Input Action เพิ่ม

- First Person ใช้กล้องที่ระดับสายตา หมุนตามเมาส์ และตัวละครหันตามทิศกล้อง ระดับกล้องปรับตามการย่อตัว
- ซ่อน mesh ตัวละครเฉพาะเจ้าของใน First Person เพื่อไม่ให้หัวบังกล้อง ผู้เล่นอื่นยังเห็นตัวละคร
- เมื่อกลับ Third Person จะคืนค่าการหมุนตัวละครและการมองเห็น mesh ก่อนสลับ
- ใช้ F เก็บของและ UI Loot เดิมได้ทั้งสองมุมมอง
- ยังไม่มีโมเดลแขนหรือแอนิเมชัน First Person โดยเฉพาะ

หลังเพิ่มกล้อง C++ ใหม่ ให้เซฟฉาก ปิด Unreal Editor แล้ว Build `ArenaGameEditor` ใน Rider ก่อนเปิดใหม่ หาก Blueprint เดิมจับปุ่ม C อยู่ ให้ย้าย binding นั้นเพื่อไม่ให้ชนกัน

ทดสอบ: กด C สองครั้ง หมุนกล้อง เดินหน้า/ถอยหลัง ย่อตัว และลองเก็บไอเทมด้วย F ในทั้งสองมุมมอง

Jumping is disabled for the heist character: the native jump action bindings are
removed and CanJumpInternal returns false on both server and clients. Existing
Blueprint-callable jump helpers are retained because the player/AI Blueprints
reference them. Space still works in cutscene/menu UI. Walking, sprinting,
sneaking and falling from ledges use their existing movement behavior.

Validation passed in two-player listen-server PIE: CanJump was false for host and client; direct Jump requests did not change height or vertical velocity, and both characters still walked normally.

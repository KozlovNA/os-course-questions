# Вывод на экран в UEFI: GOP, UGA, сравнение с VBE, использование ОС при загрузке

> В вопросе «eso graphics extensions» я понял как **VESA BIOS Extensions (VBE)**, то есть legacy-путь BIOS. Если имелось в виду другое, скажите.

## 1. Какие протоколы вывода есть в UEFI

| Протокол | Уровень | Что даёт | Статус |
|---|---|---|---|
| `EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL` (ConOut) | текст | `OutputString`, курсор, цвета, режимы `QueryMode` в символах | часть UEFI, консоль прошивки |
| **GOP**: `EFI_GRAPHICS_OUTPUT_PROTOCOL` | графика | режимы, `Blt`, **адрес и размер линейного фреймбуфера** | UEFI 2.0+, основной |
| **UGA**: `EFI_UGA_DRAW_PROTOCOL` | графика | `GetMode`/`SetMode`/`Blt`, **без адреса фреймбуфера** | Intel Framework / EFI 1.10, в UEFI-спецификацию не вошёл, заменён GOP |

Оба графических протокола ставятся на handle графического контроллера драйвером (по `EFI_DRIVER_BINDING_PROTOCOL`, например, на PCI-устройство) и находятся через `LocateProtocol`/`HandleProtocol` по GUID.

### Устройство GOP

```
EFI_GRAPHICS_OUTPUT_PROTOCOL
 ├─ QueryMode(This, ModeNumber, &SizeOfInfo, &Info)   перечисление режимов
 ├─ SetMode(This, ModeNumber)                         смена режима
 ├─ Blt(This, BltBuffer, Op, SrcX,SrcY, DstX,DstY, W,H, Delta)
 │     Op: VideoFill | VideoToBltBuffer | BufferToVideo | VideoToVideo
 │     пиксель Blt: {Blue, Green, Red, Reserved}
 └─ Mode ──► EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE
              MaxMode, Mode, Info*, SizeOfInfo,
              FrameBufferBase (физический адрес), FrameBufferSize
              Info ──► Version, HorizontalResolution, VerticalResolution,
                       PixelFormat, PixelInformation{Red,Green,Blue,Reserved маски},
                       PixelsPerScanLine
```

Формат пикселя (`PixelFormat`):
- `RedGreenBlueReserved8BitPerColor`: 32 бита, байты R,G,B,X.
- `BlueGreenRedReserved8BitPerColor`: 32 бита, байты B,G,R,X.
- `PixelBitMask`: раскладка в масках `PixelInformation`.
- `PixelBltOnly`: **прямого доступа к фреймбуферу нет**, только `Blt`.

Главная ловушка: **строка фреймбуфера занимает `PixelsPerScanLine*4` байт, а не `HorizontalResolution*4`**. Из-за выравнивания строка может быть шире экрана.

## 2. Примеры и бенчмарки (`uefi_gfx/`)

| Файл | Что делает | Проверено |
|---|---|---|
| `gop_model.c` | хостовая модель: структуры GOP со `static_assert` на смещения по спецификации, форматы пикселей, ошибка pitch, «handoff» ОС, бенчмарк `Blt` | **запущено** (gcc 14, WSL2) |
| `gop_app.c`, `Makefile`, `run_qemu.sh` | настоящее UEFI-приложение (gnu-efi): ищет GOP/UGA, перечисляет режимы, рисует через `Blt` и прямой записью во фреймбуфер | **не запускалось**: в среде нет QEMU, OVMF, gnu-efi |
| `os_view.sh` | показывает, что видит ОС после загрузки (`/proc/iomem` BOOTFB, `efifb`/`simpledrm`, BGRT) | запущено, но машина не UEFI (WSL2), поэтому вывод пустой |

### Результаты `gop_model` (запуск: `make gop_model && ./gop_model`)

```
Раскладка структур по спецификации: OK   (sizeof(ModeInfo)=36, PixelFormat@12, PixelsPerScanLine@32, Mode: FrameBufferBase@24, size=40)
Handoff для ОС: 1024x768, pitch=4352 байт (width*4=4096)
Красный: RGBX=0x000000FF  BGRX=0x00FF0000  BitMask(маски 0x3FF00000/…)=0x0FF00000
Ошибка pitch (stride=width*4): на место попало 1 из 768 строк, 767 промахов
С правильным pitch (PixelsPerScanLine*4): 768 из 768

Blt BufferToVideo 1024x768 x1500 (RAM-модель)
  BGRX->BGRX, memcpy по строкам : 0.521 s  (9051 МБ/с)
  BGRX->BGRX, попиксельный pack : 1.230 s  (3836 МБ/с, ×2.4 медленнее)
  BGRX->RGBX, обязательно pack  : 1.144 s  (4126 МБ/с)
```

Выводы:
- Раскладка структур, которую ОС и загрузчики читают по бинарному ABI, совпадает со спецификацией.
- Разница форматов существенна: один и тот же красный это `0x000000FF` или `0x00FF0000`. Нужно смотреть `PixelFormat` или маски.
- Ошибка в pitch ломает картинку почти целиком.
- Совпадение формата фреймбуфера с форматом `Blt` даёт `memcpy` вместо конвертации, это ×2.4 в модели.

**Ограничение:** это оперативная память, не MMIO видеокарты. На реальном устройстве фреймбуфер обычно отображён как write-combining (WC) или даже uncached. Запись идёт быстро (пакетами), а **чтение из фреймбуфера очень медленное**. Поэтому `VideoToVideo`/скролл через чтение и «рисовать в теневой буфер в RAM и копировать целиком» разумнее, чем читать пиксели обратно. Величину разрыва я здесь не измерял.

## 3. GOP против VBE

| Критерий | VBE (BIOS, INT 10h AX=4Fxx) | GOP (UEFI) |
|---|---|---|
| Вызов | прерывание в **real mode** (или V86, эмулятор x86 вроде `v86d`/`x86emu`) | обычный вызов функции в 64-битном режиме |
| Архитектура | только x86 | x86, ARM64, RISC-V и др.: нет привязки к x86-инструкциям |
| Режимы | номер режима + `4F01h` (структура ModeInfoBlock), сетка режимов ограничена таблицей VBE | `QueryMode` по индексу, разрешения прошивка формирует по EDID/драйверу |
| Линейный фреймбуфер | с VBE 2.0 (1994), бит 14 при `4F02h` | адрес и размер сразу в `Mode->FrameBufferBase/Size` |
| Формат пикселя | описывается через RedMaskSize/FieldPosition | `PixelFormat` или маски |
| Драйверная модель | Option ROM с 16-битным кодом на карте | Option ROM на EFI (PE, 64-бит) + `EFI_DRIVER_BINDING_PROTOCOL` |
| Абстракция рисования | нет, только «поставить режим, получить адрес» | есть `Blt` (в т.ч. для `BltOnly`-устройств, у которых фреймбуфер не виден CPU) |
| Работа из ОС после старта | возможна (в V86/эмуляции), но ломается в long mode | **пропадает** после `ExitBootServices` (см. ниже) |
| Развитие | последняя версия VBE 3.0 (1998), не развивается | часть UEFI |

Кроме того, **GOP лучше UGA**: GOP отдаёт физический адрес фреймбуфера и описывает формат пикселя, а UGA даёт только `Blt`, и ОС нечем рисовать напрямую. Это ключевой аргумент для загрузчиков и ранней консоли ядра.

**Обратная сторона GOP:** режим нельзя сменить после `ExitBootServices`. VBE в BIOS-эпоху позволял это делать (через V86) из драйверов вроде `uvesafb`. В UEFI смену режима после загрузки делает уже нативный драйвер GPU (i915, amdgpu, nouveau и др.).

## 4. Как ОС использует эти артефакты на стадии загрузки

```
Прошивка (DXE) ─► драйвер GPU ставит GOP ─► показывает логотип (BGRT в ACPI)
      │
Загрузчик (GRUB / systemd-boot / bootmgfw / rEFInd)
      │  LocateProtocol(GOP), выбор режима (SetMode), чтение Mode->Info
      ▼
 передача ОС «handoff»: base, size, width, height, pitch, формат/маски
      │  ExitBootServices()  ─► Blt/SetMode исчезают, ФРЕЙМБУФЕР остаётся
      ▼
 Ядро: пишет прямо в физический адрес (efifb/simpledrm/…), маппит как WC
      ▼
 нативный DRM-драйвер видеокарты «забирает» экран (aperture remove), фреймбуфер прошивки освобождается
```

Конкретно:

1. **Выбор режима и передача параметров.**
   - Linux с EFI stub (`drivers/firmware/efi/libstub/gop.c`) сам ищет GOP, выбирает режим и заполняет `struct screen_info`: `orig_video_isVGA = VIDEO_TYPE_EFI`, `lfb_base`, `lfb_size`, `lfb_width/height`, `lfb_linelength` (= `PixelsPerScanLine * 4`), `red_size/red_pos` и т.д. по `PixelFormat`/маскам.
   - GRUB делает то же (`gfxpayload`), Windows Boot Manager передаёт данные в loader block, для Multiboot2 есть framebuffer tag.
2. **Ранняя графическая консоль ядра.** По этим данным работают `efifb` или `simpledrm`/`sysfb`. Они пишут в фреймбуфер напрямую, потому что после `ExitBootServices` `Blt` недоступен. Отсюда правило про `pitch`. Поддерживаются форматы «32 бита» и «по маскам», а `PixelBltOnly` ядро использовать не может (нет адреса), при нём остаётся текстовый VGA или ничего.
3. **Карта памяти.** Область фреймбуфера в UEFI memory map числится как MMIO. ОС должна её отобразить (Linux: `ioremap_wc`, то есть write-combining), иначе запись будет очень медленной.
4. **Передача экрана нативному драйверу.** Когда загружается `i915`/`amdgpu`, он вызывает механизм aperture (`aperture_remove_conflicting_devices`/`sysfb_disable`), и старый `efifb`/`simpledrm` отключается, чтобы не рисовать в область, которую драйвер теперь программирует сам.
5. **BGRT.** Прошивка кладёт в ACPI-таблицу BGRT указатель на BMP с логотипом и координаты. ОС (Windows, Linux `plymouth` с bgrt-темой) рисует поверх/рядом с ним, чтобы не было «мигания» при переходе от прошивки к ОС.

Как проверить на реальной UEFI-машине под Linux: `./os_view.sh`. Типичный вывод: строки `efifb:` (`framebuffer at ..., mode is 1920x1080x32, linelength=..., Truecolor: size=8:8:8:8, shift=...`) в `dmesg` и область `BOOTFB` в `/proc/iomem`. В этой среде (WSL2) их нет, поэтому скрипт даёт пустой вывод.

## 5. Как собрать и запустить настоящее приложение

```
sudo apt install gnu-efi qemu-system-x86 ovmf
cd uefi_gfx && make && ./run_qemu.sh
```
Приложение выведет UGA (обычно «нет» в OVMF), список режимов GOP с форматом и `PixelsPerScanLine`, нарисует три цветные полосы через `Blt` и градиент прямой записью, а затем напечатает handoff (`FrameBufferBase`, `Size`, `pitch`). Файлы `gop_app.c` и `Makefile` я **не компилировал**, ожидайте мелких правок под ваши пути gnu-efi.

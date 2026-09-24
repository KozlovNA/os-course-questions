// UEFI-приложение (gnu-efi): ищет GOP и UGA, перечисляет режимы, рисует полосы и печатает то,
// что загрузчик передаст ОС. НЕ ЗАПУСКАЛОСЬ в этой среде (нет QEMU/OVMF/gnu-efi). Сборка: make; запуск: ./run_qemu.sh
#include <efi.h>
#include <efilib.h>

static EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
// UGA (Intel Framework / EFI 1.10), в спецификацию UEFI не входит
static EFI_GUID uga_guid = { 0x982c298b, 0xf4fa, 0x41cb, { 0xb8, 0x38, 0x77, 0xaa, 0x68, 0x8f, 0xb8, 0x39 } };

static const CHAR16 *fmt_name(EFI_GRAPHICS_PIXEL_FORMAT f) {
  switch (f) {
    case PixelRedGreenBlueReserved8BitPerColor: return L"RGBX8";
    case PixelBlueGreenRedReserved8BitPerColor: return L"BGRX8";
    case PixelBitMask:                          return L"BitMask";
    case PixelBltOnly:                          return L"BltOnly (нет прямого доступа к фреймбуферу)";
    default:                                    return L"?";
  }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *st) {
  InitializeLib(image, st);
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
  VOID *uga = NULL;

  Print(L"UGA: %s\n", EFI_ERROR(uefi_call_wrapper(BS->LocateProtocol, 3, &uga_guid, NULL, &uga)) ? L"нет" : L"есть (устаревший)");
  if (EFI_ERROR(uefi_call_wrapper(BS->LocateProtocol, 3, &gop_guid, NULL, (VOID **)&gop))) {
    Print(L"GOP: нет\n");
    return EFI_UNSUPPORTED;
  }

  Print(L"GOP: MaxMode=%u текущий=%u\n", gop->Mode->MaxMode, gop->Mode->Mode);
  for (UINT32 i = 0; i < gop->Mode->MaxMode; i++) {
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info; UINTN sz;
    if (EFI_ERROR(uefi_call_wrapper(gop->QueryMode, 4, gop, i, &sz, &info))) continue;
    Print(L" режим %2u: %ux%u pitch(px)=%u формат=%s%s\n", i, info->HorizontalResolution,
          info->VerticalResolution, info->PixelsPerScanLine, fmt_name(info->PixelFormat),
          i == gop->Mode->Mode ? L"  <-- текущий" : L"");
    FreePool(info);
  }

  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *cur = gop->Mode->Info;
  UINT32 w = cur->HorizontalResolution, h = cur->VerticalResolution;

  // 1) Через Blt (работает при любом формате, в том числе BltOnly). Порядок пикселя Blt: B,G,R,Reserved
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL bars[3] = { {0,0,255,0}, {0,255,0,0}, {255,0,0,0} };   // R, G, B
  for (UINT32 i = 0; i < 3; i++)
    uefi_call_wrapper(gop->Blt, 10, gop, &bars[i], EfiBltVideoFill, 0, 0, i * (w / 3), 0, w / 3, h / 2, 0);

  // 2) Прямая запись: именно так это делает ОС после ExitBootServices (Blt тогда уже недоступен)
  if (cur->PixelFormat != PixelBltOnly) {
    UINT32 *fb = (UINT32 *)(UINTN)gop->Mode->FrameBufferBase;
    for (UINT32 y = h / 2; y < h; y++)
      for (UINT32 x = 0; x < w; x++)
        fb[(UINTN)y * cur->PixelsPerScanLine + x] = (x * 255 / w) * 0x010101;    // серый градиент; pitch = PixelsPerScanLine, не w!
  }

  Print(L"\nHandoff для ОС: FrameBufferBase=0x%lx Size=0x%lx %ux%u pitch=%u байт\n",
        gop->Mode->FrameBufferBase, gop->Mode->FrameBufferSize, w, h, cur->PixelsPerScanLine * 4);
  Print(L"Нажмите любую клавишу...\n");
  UINTN idx; uefi_call_wrapper(BS->WaitForEvent, 3, 1, &ST->ConIn->WaitForKey, &idx);
  return EFI_SUCCESS;
}

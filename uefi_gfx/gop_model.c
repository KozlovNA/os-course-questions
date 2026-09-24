// Хостовая модель GOP: структуры из UEFI spec (раздел "Graphics Output Protocol"),
// форматы пикселей, ошибка pitch, "handoff" ОС после ExitBootServices и бенчмарк.
// Это RAM, не настоящий MMIO/WC-видеопамять: цифры показывают стоимость алгоритмов, не железа.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

typedef enum { PixelRedGreenBlueReserved8BitPerColor, PixelBlueGreenRedReserved8BitPerColor,
               PixelBitMask, PixelBltOnly, PixelFormatMax } GOP_PIXEL_FORMAT;
typedef struct { uint32_t Red, Green, Blue, Reserved; } GOP_PIXEL_BITMASK;
typedef struct {
  uint32_t Version, HorizontalResolution, VerticalResolution;
  GOP_PIXEL_FORMAT PixelFormat; GOP_PIXEL_BITMASK PixelInformation; uint32_t PixelsPerScanLine;
} GOP_MODE_INFO;
typedef struct {
  uint32_t MaxMode, Mode; GOP_MODE_INFO *Info; uint64_t SizeOfInfo;
  uint64_t FrameBufferBase; uint64_t FrameBufferSize;
} GOP_MODE;                                   // UINTN = 8 байт на x86-64
typedef struct { uint8_t Blue, Green, Red, Reserved; } GOP_BLT_PIXEL;   // порядок BGRA у Blt

// Проверки раскладки по спецификации (x86-64)
_Static_assert(sizeof(GOP_MODE_INFO)==36,"ModeInfo size");
_Static_assert(offsetof(GOP_MODE_INFO,PixelFormat)==12,"PixelFormat off");
_Static_assert(offsetof(GOP_MODE_INFO,PixelsPerScanLine)==32,"PixelsPerScanLine off");
_Static_assert(offsetof(GOP_MODE,Info)==8,"Info off");
_Static_assert(offsetof(GOP_MODE,FrameBufferBase)==24,"FB base off");
_Static_assert(sizeof(GOP_MODE)==40,"Mode size");
_Static_assert(sizeof(GOP_BLT_PIXEL)==4,"Blt pixel");

// ---- "то, что ОС получает от загрузчика" (аналог struct screen_info в Linux / efifb) ----
typedef struct { uint64_t base, size; uint32_t w, h, pitch_bytes, bpp; uint8_t rpos,gpos,bpos; } Handoff;

static uint32_t shift_of(uint32_t m){ return m? (uint32_t)__builtin_ctz(m):0; }
static Handoff make_handoff(const GOP_MODE *m, void *fb){
  const GOP_MODE_INFO *i=m->Info; Handoff h={(uint64_t)(uintptr_t)fb, m->FrameBufferSize,
    i->HorizontalResolution,i->VerticalResolution,i->PixelsPerScanLine*4u,32,0,0,0};
  if(i->PixelFormat==PixelRedGreenBlueReserved8BitPerColor){h.rpos=0;h.gpos=8;h.bpos=16;}
  else if(i->PixelFormat==PixelBlueGreenRedReserved8BitPerColor){h.rpos=16;h.gpos=8;h.bpos=0;}
  else {h.rpos=shift_of(i->PixelInformation.Red);h.gpos=shift_of(i->PixelInformation.Green);h.bpos=shift_of(i->PixelInformation.Blue);}
  return h;
}
static inline uint32_t pack(const Handoff*h,uint8_t r,uint8_t g,uint8_t b){return (uint32_t)r<<h->rpos|(uint32_t)g<<h->gpos|(uint32_t)b<<h->bpos;}
static inline void put(const Handoff*h,uint32_t x,uint32_t y,uint32_t px,int use_pitch){
  size_t stride = use_pitch? h->pitch_bytes : (size_t)h->w*4;      // ошибка: брать width вместо PixelsPerScanLine
  *(uint32_t*)((uint8_t*)(uintptr_t)h->base + y*stride + x*4) = px; }

static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
// Blt(BufferToVideo) для BltOnly-совместимого драйвера: конвертация BGRA-буфера в формат фреймбуфера
static void blt_buffer_to_video(const Handoff*h,const GOP_BLT_PIXEL*src,uint32_t w,uint32_t hgt,int generic){
  for(uint32_t y=0;y<hgt;y++){
    uint32_t *dst=(uint32_t*)((uint8_t*)(uintptr_t)h->base+(size_t)y*h->pitch_bytes);
    if(!generic && h->rpos==16 && h->bpos==0) memcpy(dst,src+(size_t)y*w,(size_t)w*4);   // BGRX: та же раскладка
    else for(uint32_t x=0;x<w;x++){ GOP_BLT_PIXEL p=src[(size_t)y*w+x]; dst[x]=pack(h,p.Red,p.Green,p.Blue); }
  } }

int main(void){
  const uint32_t W=1024,H=768,PPS=1088;                    // PixelsPerScanLine > width, как бывает в реальности
  size_t fbsz=(size_t)PPS*H*4; void*fb=calloc(1,fbsz);
  printf("== Раскладка структур по спецификации: OK (static_assert прошли при компиляции)\n");

  GOP_MODE_INFO info={0,W,H,PixelBlueGreenRedReserved8BitPerColor,{0},PPS};
  GOP_MODE mode={1,0,&info,sizeof info,(uint64_t)(uintptr_t)fb,fbsz};
  Handoff h=make_handoff(&mode,fb);
  printf("== Handoff для ОС: %ux%u, pitch=%u байт (width*4=%u), rpos=%u gpos=%u bpos=%u\n",h.w,h.h,h.pitch_bytes,W*4,h.rpos,h.gpos,h.bpos);

  // Тест 1: форматы пикселей. Красный (255,0,0)
  printf("== Красный в разных форматах: ");
  Handoff hr=make_handoff(&(GOP_MODE){1,0,&(GOP_MODE_INFO){0,W,H,PixelRedGreenBlueReserved8BitPerColor,{0},PPS},40,(uint64_t)(uintptr_t)fb,fbsz},fb);
  GOP_MODE_INFO im={0,W,H,PixelBitMask,{0x3FF00000,0x000FFC00,0x000003FF,0},PPS};          // 10-10-10 (BitMask)
  Handoff hm=make_handoff(&(GOP_MODE){1,0,&im,40,(uint64_t)(uintptr_t)fb,fbsz},fb);
  printf("RGBX=0x%08X  BGRX=0x%08X  BitMask(10:10:10)=0x%08X (маски Red=0x3FF00000: 255 сдвинуто на 20)\n",pack(&hr,255,0,0),pack(&h,255,0,0),pack(&hm,255,0,0));

  // Тест 2: ошибка pitch. Рисуем диагональ, считаем ошибочные пиксели
  uint32_t ok=0,bad=0; memset(fb,0,fbsz);
  for(uint32_t y=0;y<H;y++) put(&h,y%W,y,0xFFFFFFFF,0);   // неверно: stride = width*4
  for(uint32_t y=0;y<H;y++){ uint32_t px=*(uint32_t*)((uint8_t*)fb+(size_t)y*h.pitch_bytes+(y%W)*4); if(px==0xFFFFFFFF) ok++; else bad++; }
  printf("== Ошибка pitch (stride=width*4 вместо PixelsPerScanLine*4): попало на место %u из %u, промах %u -> картинка «косая»\n",ok,H,bad);
  memset(fb,0,fbsz); ok=bad=0;
  for(uint32_t y=0;y<H;y++) put(&h,y%W,y,0xFFFFFFFF,1);
  for(uint32_t y=0;y<H;y++){ uint32_t px=*(uint32_t*)((uint8_t*)fb+(size_t)y*h.pitch_bytes+(y%W)*4); px==0xFFFFFFFF?ok++:bad++; }
  printf("== С правильным pitch: попало %u, промах %u\n",ok,bad);

  // Бенчмарк: Blt BufferToVideo, 1024x768
  GOP_BLT_PIXEL*buf=malloc((size_t)W*H*4); for(size_t i=0;i<(size_t)W*H;i++) buf[i]=(GOP_BLT_PIXEL){i&255,(i>>8)&255,(i>>3)&255,0};
  const int REP=1500; double t;
  t=now(); for(int r=0;r<REP;r++) blt_buffer_to_video(&h,buf,W,H,0); double t_fast=now()-t;
  t=now(); for(int r=0;r<REP;r++) blt_buffer_to_video(&h,buf,W,H,1); double t_gen=now()-t;
  t=now(); for(int r=0;r<REP;r++) blt_buffer_to_video(&hr,buf,W,H,0); double t_rgb=now()-t;
  double mb=(double)W*H*4*REP/1e6;
  printf("\n== Бенчмарк Blt BufferToVideo 1024x768 x%d (RAM-модель)\n",REP);
  printf("BGRX -> BGRX, memcpy по строкам    : %.3f s  (%.0f МБ/с)\n",t_fast,mb/t_fast);
  printf("BGRX -> BGRX, попиксельный pack    : %.3f s  (%.0f МБ/с, x%.1f медленнее)\n",t_gen,mb/t_gen,t_gen/t_fast);
  printf("BGRX -> RGBX, обязательно pack     : %.3f s  (%.0f МБ/с)\n",t_rgb,mb/t_rgb);
  // Скролл: VideoToVideo (memmove) 16 строк
  t=now(); for(int r=0;r<REP*4;r++) memmove(fb,(uint8_t*)fb+(size_t)16*h.pitch_bytes,(size_t)(H-16)*h.pitch_bytes); double t_sc=now()-t;
  printf("Скролл на 16 строк (VideoToVideo)  : %.3f s за %d раз (в RAM; на UC/WC-MMIO чтение из фреймбуфера в разы дороже)\n",t_sc,REP*4);
  return 0; }

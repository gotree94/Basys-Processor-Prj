/* ============================================================================
 * app_util.c - 공통 유틸: UART 저수준 + 미니 printf + 지연 + 문자열
 * ----------------------------------------------------------------------------
 * newlib 의존을 피하기 위해 최소 printf / mem* 를 직접 구현.
 * ==========================================================================*/
#include "app.h"
#include <stdarg.h>

/* ---- UART 저수준 -------------------------------------------------------- */
void uart_putc(char c)
{
  while (!(UART_STAT & 0x1)) { /* TX ready 대기 */ }
  UART_DATA = (uint32_t)(uint8_t)c;
}

void uart_puts(const char *s)
{
  while (*s) {
    if (*s == '\n') uart_putc('\r');
    uart_putc(*s++);
  }
}

/* ---- 미니 printf (%c %s %d %u %x %p %%) -------------------------------- */
static void print_u(uint32_t v, uint32_t base, int upper)
{
  char buf[11];
  int i = 0;
  const char *dig = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  if (v == 0) { uart_putc('0'); return; }
  while (v) { buf[i++] = dig[v % base]; v /= base; }
  while (i--) uart_putc(buf[i]);
}

void uart_printf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  for (const char *p = fmt; *p; p++) {
    if (*p != '%') { uart_putc(*p); if (*p=='\n') uart_putc('\r'); continue; }
    p++;
    switch (*p) {
      case 'c': uart_putc((char)va_arg(ap, int)); break;
      case 's': uart_puts(va_arg(ap, const char *)); break;
      case 'd': { int32_t v = va_arg(ap, int32_t);
                  if (v < 0) { uart_putc('-'); print_u((uint32_t)(-v), 10, 0); }
                  else print_u((uint32_t)v, 10, 0); } break;
      case 'u': print_u(va_arg(ap, uint32_t), 10, 0); break;
      case 'x': print_u(va_arg(ap, uint32_t), 16, 0); break;
      case 'X': print_u(va_arg(ap, uint32_t), 16, 1); break;
      case 'p': uart_puts("0x"); print_u(va_arg(ap, uint32_t), 16, 0); break;
      case '%': uart_putc('%'); break;
      default : uart_putc('%'); uart_putc(*p); break;
    }
  }
  va_end(ap);
}

/* ---- 사이클 카운터 / 지연 ---------------------------------------------- */
uint32_t rdcycle(void)
{
  uint32_t c;
  asm volatile ("rdcycle %0" : "=r"(c));
  return c;
}

void app_delay_ms(uint32_t ms)
{
  uint32_t start = rdcycle();
  uint32_t ticks = ms * (CPU_HZ / 1000u);
  while ((rdcycle() - start) < ticks) { /* busy wait */ }
}

void app_report(const char *name, int r)
{
  if (r == 0) uart_printf("[PASS] %s\n", name);
  else        uart_printf("[FAIL] %s (code=%d)\n", name, r);
}

/* ---- freestanding 문자열 (컴파일러 내장 호출 대비) --------------------- */
void *memcpy(void *dst, const void *src, unsigned n)
{
  uint8_t *d = dst; const uint8_t *s = src;
  while (n--) *d++ = *s++;
  return dst;
}

void *memset(void *dst, int c, unsigned n)
{
  uint8_t *d = dst;
  while (n--) *d++ = (uint8_t)c;
  return dst;
}

void *memmove(void *dst, const void *src, unsigned n)
{
  uint8_t *d = dst; const uint8_t *s = src;
  if (d < s) { while (n--) *d++ = *s++; }
  else { d += n; s += n; while (n--) *--d = *--s; }
  return dst;
}

# stdio

A small standard input/output library. It is portable C; the only
platform-specific part is two one-character hooks.

## Why hooks

The bare MERA-400 processor has no console, and CROOK (its operating system)
transfers whole records. So the printf family never touches a device: every
output character goes through

```c
int __stdout_putc(int c);   /* returns c, or EOF on error */
int __stdin_getc(void);     /* returns a character, or EOF */
```

* **Platform `none`** (`console.c`): weak definitions that use a
  memory-mapped console word whose byte address is the linker symbol
  `__console_port` (the top word of memory by default: `0xFFFE` in the small
  model, `0x1FFFE` in the large one). Writing the word outputs its low byte;
  reading it gives the next input byte, or `0xFFFF` at the end of input. The
  Python simulator in `tests/sim` models this word. A program may define its
  own `__stdout_putc` to replace it.
* **Platform `crook`**: `src/crook/stdout.c` collects characters into a line
  and sends it as one record at each `'\n'` (or on `fflush`). End messages
  with `'\n'`.

## What is supported

`printf fprintf sprintf snprintf vprintf vfprintf vsprintf vsnprintf puts
putchar fputs fputc putc fflush`, with the conversions `%d %i %u %ld %lu %x
%X %o %c %s %p %%`, flags `- + space # 0`, width and precision (also `*`),
and `%f %e %g` for the native double (digits are reliable to about 11
significant digits). `sscanf` understands `%d %s %c`. `stdout` and `stderr`
are minimal `FILE` objects that both feed the hook.

Arguments are fetched with the compiler's real `va_arg`.

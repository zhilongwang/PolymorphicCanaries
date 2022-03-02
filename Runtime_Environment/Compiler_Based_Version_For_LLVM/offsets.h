#ifndef __OFFSETS_H__
#define __OFFSETS_H__

#if     defined(__i386__)
/* TODO: 32-bit */
#error  "Unsupported architecture"
#elif   defined(__x86_64__)
#define CHECK_CANARY_OFFSET	"0x2a0"   /* offset of cmp_canary addr in TLS */
#define CANARY1_OFFSET		"0x2a8"  /* offset of canary1 addr in TLS       */
#define CANARY2_OFFSET		"0x2b0"  /* offset of canary2 addr in TLS       */
#else
#error  "Unsupported architecture"
#endif

#endif /* __DYNAGUARD_OFFSETS_H__ */

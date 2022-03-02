#ifndef __OFFSETS_H__
#define __OFFSETS_H__

#if     defined(__i386__)
/* TODO: 32-bit */
#error  "Unsupported architecture"
#elif   defined(__x86_64__)
#define CAB_PAGES                 2       /* CAB size (in pages)              */
#define CMP_CANARY_OFFSET        "0x2a0"  /* offset of cmp_canary A addr. in TLS */
#define REDUNDANT_CANARY_OFFSET  "0x28"  /* offset of cmp_canary A addr. in TLS */
#else
#error  "Unsupported architecture"
#endif

#endif /* __DYNAGUARD_OFFSETS_H__ */

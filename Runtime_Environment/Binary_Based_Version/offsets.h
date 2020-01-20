#ifndef __OFFSETS_H__
#define __OFFSETS_H__

#if     defined(__i386__)
/* TODO: 32-bit */
#error  "Unsupported architecture"
#elif   defined(__x86_64__)
#define CAB_PAGES                 2       /* CAB size (in pages)              */
#define CMP_CANARY_OFFSET        "0x2a0"  /* offset of cmp_canary A addr. in TLS */
#define REDUNDANT_CANARY_OFFSET  "0x28"  /* offset of cmp_canary A addr. in TLS */
#define CAB_TLS_OFFSET_STR       "0x2a0"  /* offset of CAB addr. in TLS       */
#define CAB_IDX_TLS_OFFSET_STR   "0x2a8"  /* offset of CAB index in TLS       */
#define CAB_SZ_TLS_OFFSET_STR    "0x2b0"  /* offset of CAB size in TLS        */
#define CAN_TLS_OFFSET_STR       "0x2b8"  /* offset of DG canary in TLS       */
/* same as above but numeric */
#define CAB_OFFSET                0x2a0
#define CAB_IDX                   0x2a8
#define CAB_SZ                    0x2b0
#define CAN_OFFSET                0x2b8
#else
#error  "Unsupported architecture"
#endif

#endif /* __DYNAGUARD_OFFSETS_H__ */

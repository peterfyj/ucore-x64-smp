#ifndef __GLUE_MP_H__
#define __GLUE_MP_H__

#ifdef LIBSPREFIX
#include <libs/types.h>
#else
#include <types.h>
#endif

typedef struct lcpu_info_s
{
	int      lapic_id;
	int      idx;
	
	uint64_t freq;
} lcpu_info_s;

#define lapic_id_get     (*lapic_id_get_ptr)
#define lcpu_idx_get     (*lcpu_idx_get_ptr)
#define lcpu_count_get   (*lcpu_count_get_ptr)
#define lapic_ipi_issue  (*lapic_ipi_issue_ptr)

extern unsigned int lapic_id_get(void);
extern unsigned int lcpu_idx_get(void);
extern unsigned int lcpu_count_get(void);
extern int          lapic_ipi_issue(int lapic_id);

#define LAPIC_COUNT 256

/* Process Local Storage Related Macros */

#define PLS __attribute__((section(".pls")))

#define __stringify_1(x...) #x
#define __stringify(x...)  __stringify_1(x)

#define __pls_seg gs
#define __pls_prefix "%%" __stringify(__pls_seg) ":"
#define __pls_arg(x) __pls_prefix "%P" #x

#define pls_from_op(op, var, constraint)		\
	({											\
	typeof(var) __ret;							\
	switch (sizeof (var)) {						\
	case 1:										\
		asm (op"b "__pls_arg(1)",%0"			\
			 : "=q" (__ret)						\
			 : constraint);						\
		break;									\
	case 2:										\
		asm (op"w "__pls_arg(1)",%0"			\
			 : "=q" (__ret)						\
			 : constraint);						\
		break;									\
	case 4:										\
		asm (op"l "__pls_arg(1)",%0"			\
			 : "=q" (__ret)						\
			 : constraint);						\
		break;									\
	case 8:										\
		asm (op"q "__pls_arg(1)",%0"			\
			 : "=q" (__ret)						\
			 : constraint);						\
		break;									\
	}											\
	__ret;										\
	})

#define pls_read(var)    pls_from_op("mov", var, "m" (var))

#endif

#ifndef _LX_EMUL__SHADOW__LINUX__MODULEPARAM_H_
#define _LX_EMUL__SHADOW__LINUX__MODULEPARAM_H_

#include_next<linux/moduleparam.h>

#undef module_param_named_unsafe

#ifdef _XE_MODULE_H_
#define module_param_named_unsafe(name, name_storage, type, perm) \
	typeof(name_storage) * module_param_xe_##name(void) { return &name_storage; }
#else
#define module_param_named_unsafe(name, name_storage, type, perm) \
	typeof(name_storage) * module_param_##name(void) { return &name_storage; }
#endif

#endif /* _LX_EMUL__SHADOW__LINUX__MODULEPARAM_H_ */


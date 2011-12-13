#ifndef __KERN_MODULE_MOD_MANAGER_H__
#define  __KERN_MODULE_MOD_MANAGER_H__

int add_module(const char *name);
int del_module(const char *name);
int module_loaded(const char *name);
void print_loaded_module();

#endif

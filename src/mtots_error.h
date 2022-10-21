#ifndef mtots_error_h
#define mtots_error_h

/* For setting a runtime error.
 * Stack information will also be recorded in the VM
 *
 * However, this function does not actually interrupt or
 * pause execution - the caller of this function should also
 * return a proper value indicating that an error has happened
 *
 * This method is defined in mtots_vm.c */
void runtimeError(const char *format, ...);

#endif/*mtots_error_h*/

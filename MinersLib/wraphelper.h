/**
 * parnianminer code
 *
 * Copyright 2026 Amir Reza Zamani <amirrezazamani@gmail.com> - https://parniancoin.com
 * Author: Amir Reza Zamani <amirrezazamani@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. It is distributed WITHOUT ANY WARRANTY.
 * See the LICENSE file or <https://www.gnu.org/licenses/gpl-3.0.html>.
 */

/*
* Wrappers to emulate dlopen() on other systems like Windows
*/
#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
#include <windows.h>
static void *wrap_dlopen(const char *filename) {
	return (void *)LoadLibrary(filename);
}
static void *wrap_dlsym(void *h, const char *sym) {
	return (void *)GetProcAddress((HINSTANCE)h, sym);
}
static int wrap_dlclose(void *h) {
	/* FreeLibrary returns nonzero on success */
	return (!FreeLibrary((HINSTANCE)h));
}
#else
/* assume we can use dlopen itself... */
#include <dlfcn.h>
static void *wrap_dlopen(const char *filename) {
	return dlopen(filename, RTLD_NOW);
}
static void *wrap_dlsym(void *h, const char *sym) {
	return dlsym(h, sym);
}
static int wrap_dlclose(void *h) {
	return dlclose(h);
}
#endif
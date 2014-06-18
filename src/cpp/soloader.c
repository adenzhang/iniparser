#include <stdio.h>
#include <dlfcn.h>

int main(int argc, char **argv) 
{
   void *lib_handle;
   void *fn = NULL;
   int x;
   char *error;

   if( argc < 2) {
	   fprintf(stderr, "cmd soname\n");
	   return 1;
   }

   lib_handle = dlopen(argv[1], RTLD_NOW|RTLD_GLOBAL);
   if (!lib_handle) 
   {
      fprintf(stderr, "%s\n", dlerror());
      return 1;
   }

   if( argc > 2) {
	fn = dlsym(lib_handle, argv[2]);
	if ((error = dlerror()) != NULL)  
	{
		fprintf(stderr, "%s\n", error);
		return 1;
	}
	printf("loaded function %s\n",argv[2]);
   }

   dlclose(lib_handle);
   return 0;
}

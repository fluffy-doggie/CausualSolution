
#pragma comment(lib, "glfw3dll.lib")
#pragma comment(lib, "tbox.lib")

extern int __main_01(int argc, char *argv[]);
extern int __main_02(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	return __main_02(argc, argv);
}
// OUTPUT: main\(\)
// OUTPUT: Script\[.*\]: Run time error 1: "Forced exit"

#include "test"

public OnGameModeInit() {
	TestExit();
}

f() {
	#emit halt 1
}

main() {
	print("main()");
	f();
	print("FAIL");
}

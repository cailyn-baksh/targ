#include "targ.hpp"
#include "unix.hpp"

struct SampleParser : public targ::UnixParser {
	targ::Switch assemblyOut{this, 'S', "Compile but do not assemble"};
	targ::Switch verbose{this, 'v', "verbose", "Show verbose output"};

	targ::Option<std::string> lang{this, 'x', "Set the language"};
	targ::Option<std::string> arch{this, "arch", "Set the target architecture"};
	targ::Option<std::string> outFilePath{this, 'o', "output", "Set the output file"};
	targ::Option<std::vector<std::string>> multiple{this, 'm', "multiple", "multiple arguments"};

	SampleParser() {
		outFilePath = "a.out";
	}
};

int main(int argc, char **argv) {
	SampleParser p = targ::parse<SampleParser>(argc, argv);
}

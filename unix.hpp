/*
 * A base targ parser for unix-style arguments
 */
#ifndef _TARG_UNIX_HPP_
#define _TARG_UNIX_HPP_

#include <typeinfo>

#include "targ.hpp"

namespace targ {
	class UnixParser : public AbstractParser {
	protected:
		bool parseOptions = true;

	public:
		const std::string shortOptPrefix = "-";
		const std::string longOptPrefix = "--";

		virtual bool metaparser(std::string arg) {
			if (arg == "--") {
				// Stop parsing options
				parseOptions = false;
				return true;
			}

			return false;
		}

		virtual bool shouldTest(AbstractArgument *arg) {
			if (!parseOptions && typeid(arg->tag) == typeid(positional_tag)) {
				// Don't parse options
				return false;
			}

			return true;
		}
	};
}

#endif  // _TARG_UNIX_HPP_

#ifndef _TARG_HPP_
#define _TARG_HPP_

#include <concepts>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace targ {
	class AbstractArgument;

	struct any_tag {};
	struct option_tag : public any_tag {};
	struct positional_tag : public any_tag {};

	/*
	 * An exception thrown when parsing is invalid. This exception is used to
	 * signal invalid user input.
	 */
	class ParsingError : public std::runtime_error {
	public:
		ParsingError(std::string msg) : std::runtime_error(msg) {}
		const char *what() const throw() { return std::runtime_error::what(); }
	};

	/*
	 * Abstract parser class. All parsers should inherit from this class.
	 *
	 * Specialized program argument parsers are created by subclassing this
	 * class and adding member variables of an AbstractArgument type. Default
	 * values should be set in the constructor.
	 */
	class AbstractParser {
		friend class AbstractArgument;

		template <typename T>
		friend T parse(int argc, char **argv) requires std::derived_from<T, AbstractParser>;

	protected:
		std::string prgmName;

		// is it possible to populate this at compile time?
		std::vector<AbstractArgument *> args;

	public:
		const std::string shortOptPrefix;
		const std::string longOptPrefix;

		/*
		 * Parse meta arguments; that is 'arguments' which inform the parser on
		 * how to parse future arguments.
		 *
		 * arg		The meta-argument.
		 * Returns true if the argument was consumed, false otherwise.
		 */
		virtual bool metaparser(std::string arg) { return false; }

		/*
		 * Discriminate arguments out of args. This is intended to be used in
		 * conjunction with metaparser to selectively parse certain types of
		 * arguments dependent on meta args.
		 *
		 * arg		A pointer to the argument which the parser wants to test.
		 * Returns true if the parser should test for this argument, and false
		 * if it shouldn't.
		 */
		virtual bool shouldTest(AbstractArgument *arg) { return true; }
	};

	/*
	 * Any type of argument to be parsed from the command line.
	 */
	class AbstractArgument {
	protected:
		std::string help;
		AbstractParser *parser;

		/*
		 * Add this to the specified parser
		 *
		 * parser	The parser to add this argument to
		 */
		void addToParser(AbstractParser *parser) {
			parser->args.push_back(this);
		}

	public:
		const any_tag tag{};

		/*
		 * Parse argument from beginning of argv
		 *
		 * argc		Count of elements after argv
		 * argv		Array of strings to parse the argument from
		 * Returns the number of elements consumed from argv. Returning 0
		 * indicates no arguments were parsed.
		 * Throws ParsingError when this option is present but malformed.
		 */
		virtual int parseArg(int argc, char **argv) = 0;
	};

	template <typename T>
	class PositionalArgument : public AbstractArgument {
	protected:
		std::string name;
		T value;

	public:
		const any_tag tag = positional_tag();

		PositionalArgument<T> &operator=(const T &v) {
			value = v;
			return *this;
		}

		virtual int parseArg(int argc, char **argv) {
			return 0;
		}
	};

	/*
	 * An option
	 *
	 * Some type specializations of this class change how option arguments are
	 * parsed from the command line.
	 * For most types, the next command line argument is cast to T (and so a
	 * user conversion operator must be defined if it isn't already).
	 * For booleans, the option is treated as a switch. By default the option's
	 * value is false, and it is true if the switch is specified on the command
	 * line.
	 * For any vector type, zero or more arguments are parsed after the option.
	 * For any optional type, zero or one arguments are parsed after the option
	 */
	template <typename T>
	class Option : public AbstractArgument {
	protected:
		char shortName;
		std::string longName;
		T value;

		constexpr bool doesStrMatchOption(std::string str) {
			return ((str.starts_with(parser->shortOptPrefix) && str[parser->shortOptPrefix.length()] == shortName)
				|| (str.starts_with(parser->longOptPrefix) && str.substr(parser->longOptPrefix.length()) == longName));
		}

	public:
		const any_tag tag = option_tag();

		/*
		 * Construct a new option
		 *
		 * parser	The parser to add the option to
		 * s		The short option name
		 * help		A help string
		 */
		Option(AbstractParser *parser, char s, std::string help) : shortName(s) {
			this->help = help;
			this->parser = parser;

			addToParser(parser);
		}

		/*
		 * Construct a new option
		 *
		 * parser	The parser to add the option to
		 * l		The long option name
		 * help		A help string
		 */
		Option(AbstractParser *parser, std::string l, std::string help) : longName(l) {
			this->help = help;
			this->parser = parser;

			addToParser(parser);
		}

		/*
		 * Construct a new option
		 *
		 * parser	The parser to add the option to
		 * s		The short option name
		 * l		The long option name
		 * help		A help string
		 */
		Option(AbstractParser *parser, char s, std::string l, std::string help) : shortName(s), longName(l) {
			this->help = help;
			this->parser = parser;

			addToParser(parser);
		}

		Option<T> &operator=(const T &v) {
			value = v;
			return *this;
		}

		virtual int parseArg(int argc, char **argv) {
			if (doesStrMatchOption(argv[0])) {
				if constexpr (std::same_as<T, bool>) {
					// handle switches
					value = true;
					return 1;
				} else if constexpr (std::same_as<T, std::vector<typename T::value_type>>) {
					// handle option with multiple args
				} else if constexpr (std::same_as<T, std::optional<typename T::value_type>>) {

				} else {
					// default option type
					if (argc > 1) {
						value = static_cast<T>(argv[1]);
						return 2;
					} else {
						throw ParsingError(std::string("Option ") + longName + " expects one argument!");
					}
				}
			}

			return 0;
		}
	};

	/*
	 * An option which takes no parameters. Its presence or absence indicates a
	 * true or false value.
	 */
	typedef Option<bool> Switch;

	/*
	 * Parse program options.
	 *
	 * T	The parser class. Must be a subclass of AbstractArgument
	 *
	 * argc		The number of command line arguments
	 * argv		The command line arguments
	 */
	template <typename T>
	T parse(int argc, char **argv) requires std::derived_from<T, AbstractParser> {
		T parser;

		// Initialize environment vars
		parser.prgmName = argv[0];

		for (int i=0; i < argc; ) {
			// metaparsing
			if (parser.metaparser(argv[i])) {
				// metaparser parsed something; move on to next argument
				++i;
				continue;
			}

			for (AbstractArgument *arg : parser.args) {
				if (parser.shouldTest(arg)) {
					int argsConsumed = arg->parseArg(argc - i, &argv[i]);

					if (argsConsumed != 0) {
						// An argument was parsed
						i += argsConsumed;
						break;
					}
				}

			}
		}

		return parser;
	}
}

#endif  // _TARG_HPP_

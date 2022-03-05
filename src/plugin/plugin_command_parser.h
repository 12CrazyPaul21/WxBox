/*
 * Command Statement BNF:
 *     statement ::= prefixexp methodcall
 *     prefixexp ::= ">>"
 *     methodcall ::= method | method ":" arglist
 *     method ::= methodname | modulename "." methodname
 *     arglist ::= expr {"," expr}
 *     expr ::= false | true | Numeral | LiteralString
 *
 * note:
 *     - escape characters are not supported
 *     - numeral only support decimal positive integer
 */

#ifndef __WXBOX_PLUGIN_COMMAND_PARSER_H
#define __WXBOX_PLUGIN_COMMAND_PARSER_H

namespace wxbox {
    namespace plugin {

        //
        // Lexical
        //

        constexpr char COMMAND_PREFIX_EXP[]          = ">>";
        constexpr char COMMAND_DOT_SEPARATOR[]       = ".";
        constexpr char COMMAND_ARG_BEGIN_SEPARATOR[] = ":";
        constexpr char COMMAND_ARG_SPLIT_SEPARATOR[] = ",";
        constexpr char COMMAND_STRING_SEPARATOR[]    = "\"\'";
        constexpr char COMMAND_BOOLEAN_TRUE[]        = "true";
        constexpr char COMMAND_BOOLEAN_FALSE[]       = "false";

        enum class CommandLexTag
        {
            Illegal,
            Eof,

            PrefixSeparator,
            DotSeparator,
            ArgBeginSeparator,
            ArgSplitSeparator,

            Identifier,
            NumeralLiteral,
            StringLiteral,
            BooleanLiteral
        };

        struct CommandLexToken
        {
            CommandLexTag tag;
        };

        struct CommandLexIllegal : public CommandLexToken
        {
            std::string value;
        };

        struct CommandLexIdentifier : public CommandLexToken
        {
            std::string value;
        };

        struct CommandLexNumeralLiteral : public CommandLexToken
        {
            long long value;
        };

        struct CommandLexStringlLiteral : public CommandLexToken
        {
            std::string value;
        };

        struct CommandLexBooleanLiteral : public CommandLexToken
        {
            bool value;
        };

        template<CommandLexTag tag>
        struct CommandLexTagTrait
        {
            using ContainerPtrType = std::shared_ptr<CommandLexToken>;
        };

        using CommandLexTokenPtr = std::shared_ptr<CommandLexToken>;

        class PluginCommandScanner final
        {
            PluginCommandScanner() = delete;

          public:
            using CommandChar                   = uint16_t;
            static constexpr CommandChar CH_EOF = 0xFFFF;

            PluginCommandScanner(const char* statement)
              : buffer(statement)
              , p(buffer.c_str())
            {
                cursor = p;
                end    = p;

                if (end) {
                    while (*end++)
                        ;
                }
            }

            inline bool eof()
            {
                return !cursor || cursor >= end || *cursor == '\0';
            }

            inline void reset()
            {
                cursor = p;
            }

            inline CommandChar peek()
            {
                if (!cursor || cursor >= end) {
                    return CH_EOF;
                }

                return *cursor ? *cursor : CH_EOF;
            }

            inline CommandChar getch()
            {
                if (!cursor || cursor >= end) {
                    return CH_EOF;
                }

                auto c = (uint8_t)*cursor++;
                return c ? c : CH_EOF;
            }

            inline CommandChar getch_skip_blank()
            {
                CommandChar c;

                do {
                    c = getch();
                } while (c != CH_EOF && !IS_MULTI_BYTES_CHAR_CODE(c) && ::isspace(c));

                return c;
            }

            inline void ungetch()
            {
                if (!cursor || cursor <= p) {
                    return;
                }

                --cursor;
            }

          private:
            std::string       buffer;
            const char* const p;
            const char*       cursor;
            const char*       end;
        };

        class PluginCommandLexParser final
        {
            PluginCommandLexParser() = delete;

          public:
            PluginCommandLexParser(const char* statement)
              : scanner(statement)
            {
            }

            CommandLexTokenPtr next_token();

            void reset()
            {
                scanner.reset();
            }

          private:
            PluginCommandScanner scanner;
        };

        //
        // Parse Result
        //

        enum class CommandParameterType
        {
            NumeralLiteral = 0,
            StringLiteral,
            BooleanLiteral
        };

        enum class CommandMethodType
        {
            ModuleMethod = 0,
            GlobalMethod
        };

        enum class CommandParseStatus
        {
            Success = 0,
            NotCommand,
            LexIllegal,
            RepeatPrefixSeparator,
            MethodNameNotSpecified,
            InvalidModuleRef,
            MissArgBeginSeparator,
            IllegalParameterList
        };

        struct CommandParameter
        {
            CommandParameterType type;
            long long            numeralval;
            std::string          strval;
            bool                 boolval;

            CommandParameter()
              : type(CommandParameterType::NumeralLiteral)
              , numeralval(0)
              , strval("")
              , boolval(false)
            {
            }

            SETUP_MOVE_METHOD(CommandParameter, other)
            {
                type       = other.type;
                numeralval = other.numeralval;
                strval     = std::move(other.strval);
                boolval    = other.boolval;
            }
        };

        struct CommandExecuteInfo
        {
            CommandMethodType             type;
            std::string                   moduleName;
            std::string                   methodName;
            std::vector<CommandParameter> argLists;

            CommandExecuteInfo()
              : type(CommandMethodType::GlobalMethod)
            {
            }

            SETUP_MOVE_METHOD(CommandExecuteInfo, other)
            {
                type       = other.type;
                moduleName = std::move(other.moduleName);
                methodName = std::move(other.methodName);
                argLists   = std::move(other.argLists);
            }
        };

        using CommandExecuteInfoPtr = std::shared_ptr<CommandExecuteInfo>;

        struct CommandParseResult
        {
            CommandParseStatus    status;
            std::string           error;
            CommandExecuteInfoPtr commandInfo;
        };

        using CommandParseResultPtr = std::shared_ptr<CommandParseResult>;

        //
        // Methods
        //

        template<CommandLexTag tag>
        auto BuildCommandLexToken() -> typename CommandLexTagTrait<tag>::ContainerPtrType
        {
            auto ptr = std::make_shared<CommandLexTagTrait<tag>::ContainerPtrType::element_type>();
            ptr->tag = tag;
            return ptr;
        }

        template<CommandLexTag tag>
        auto CastCommandLexToken(CommandLexTokenPtr ptr) -> typename CommandLexTagTrait<tag>::ContainerPtrType
        {
            return std::static_pointer_cast<CommandLexTagTrait<tag>::ContainerPtrType::element_type>(ptr);
        }

        CommandParseResultPtr ParseCommandStatement(const char* const statement);
    }
}

#define RegisterCommandLexTag(TYPE, CONTAINER) RegisterPluginVirtualMachineTrait(CommandLexTagTrait, TYPE, CONTAINER)

// register plugin virtual machine lex tag
RegisterCommandLexTag(wxbox::plugin::CommandLexTag::Illegal, CommandLexIllegal);
RegisterCommandLexTag(wxbox::plugin::CommandLexTag::Eof, CommandLexToken);
RegisterCommandLexTag(wxbox::plugin::CommandLexTag::PrefixSeparator, CommandLexToken);
RegisterCommandLexTag(wxbox::plugin::CommandLexTag::DotSeparator, CommandLexToken);
RegisterCommandLexTag(wxbox::plugin::CommandLexTag::ArgBeginSeparator, CommandLexToken);
RegisterCommandLexTag(wxbox::plugin::CommandLexTag::ArgSplitSeparator, CommandLexToken);
RegisterCommandLexTag(wxbox::plugin::CommandLexTag::Identifier, CommandLexIdentifier);
RegisterCommandLexTag(wxbox::plugin::CommandLexTag::NumeralLiteral, CommandLexNumeralLiteral);
RegisterCommandLexTag(wxbox::plugin::CommandLexTag::StringLiteral, CommandLexStringlLiteral);
RegisterCommandLexTag(wxbox::plugin::CommandLexTag::BooleanLiteral, CommandLexBooleanLiteral);

#define CheckCommandLexTokenIsParameter(token) (token->tag == wxbox::plugin::CommandLexTag::NumeralLiteral || token->tag == wxbox::plugin::CommandLexTag::StringLiteral || token->tag == wxbox::plugin::CommandLexTag::BooleanLiteral)

#endif  // #ifndef __WXBOX_PLUGIN_COMMAND_PARSER_H
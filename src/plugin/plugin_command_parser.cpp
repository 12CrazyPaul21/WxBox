#include <plugin/plugin.h>

//
// Global Variables
//

static const char* CommandParseFailedMessage[] = {
    "",                                                                         // Success
    "not a command",                                                            // NotCommand
    "lexical illegal",                                                          // LexIllegal
    "syntax error : repeat command prefix separator",                           // RepeatPrefixSeparator
    "syntax error : method name not specified",                                 // MethodNameNotSpecified
    "syntax error : no method name was specified when referencing the module",  // InvalidModuleRef
    "syntax error : miss \":\" to pass parameter",                              // MissArgBeginSeparator
    "syntax error : illegal parameter list",                                    // IllegalParameterList
};

//
// Inner Methods
//

static inline std::shared_ptr<wb_plugin::CommandLexIllegal> BuildCommandLexIllegalToken(const std::string& error)
{
    auto ptr   = std::make_shared<wb_plugin::CommandLexIllegal>();
    ptr->tag   = wb_plugin::CommandLexTag::Illegal;
    ptr->value = error;
    return ptr;
}

static inline wxbox::plugin::CommandExecuteInfoPtr BuildCommandExecuteInfo()
{
    return std::make_shared<wxbox::plugin::CommandExecuteInfo>();
}

static inline wxbox::plugin::CommandParseResultPtr BuildCommandParseFailedResult(wb_plugin::CommandParseStatus status, wxbox::plugin::CommandLexTokenPtr token = nullptr)
{
    auto ptr = std::make_shared<wxbox::plugin::CommandParseResult>();
    if (token && token->tag == wb_plugin::CommandLexTag::Illegal) {
        ptr->status = wb_plugin::CommandParseStatus::LexIllegal;
        ptr->error  = std::string(CommandParseFailedMessage[(int)wb_plugin::CommandParseStatus::LexIllegal]) + " : " + wb_plugin::CastCommandLexToken<wb_plugin::CommandLexTag::Illegal>(token)->value;
    }
    else {
        ptr->status = status;
        ptr->error  = CommandParseFailedMessage[(int)status];
    }
    ptr->commandInfo = nullptr;
    return ptr;
}

static inline wxbox::plugin::CommandParseResultPtr BuildCommandParseResult(wb_plugin::CommandExecuteInfoPtr commandInfo)
{
    auto ptr         = std::make_shared<wxbox::plugin::CommandParseResult>();
    ptr->status      = wb_plugin::CommandParseStatus::Success;
    ptr->error       = CommandParseFailedMessage[(int)wb_plugin::CommandParseStatus::Success];
    ptr->commandInfo = std::move(commandInfo);
    return ptr;
}

//
// PluginCommandLexParser
//

wxbox::plugin::CommandLexTokenPtr wxbox::plugin::PluginCommandLexParser::next_token()
{
    PluginCommandScanner::CommandChar c = scanner.getch_skip_blank();

    if (c == PluginCommandScanner::CH_EOF) {
        return wb_plugin::BuildCommandLexToken<wb_plugin::CommandLexTag::Eof>();
    }

    // parse "prefixexp"
    if (c == wb_plugin::COMMAND_PREFIX_EXP[0]) {
        if (scanner.getch() == wb_plugin::COMMAND_PREFIX_EXP[0]) {
            return wb_plugin::BuildCommandLexToken<wb_plugin::CommandLexTag::PrefixSeparator>();
        }

        return BuildCommandLexIllegalToken("parse <PrefixSeparator> failed, must be \">>\"");
    }

    // parse "dot"
    if (c == wb_plugin::COMMAND_DOT_SEPARATOR[0]) {
        return wb_plugin::BuildCommandLexToken<wb_plugin::CommandLexTag::DotSeparator>();
    }

    // parse ":"
    if (c == wb_plugin::COMMAND_ARG_BEGIN_SEPARATOR[0]) {
        return wb_plugin::BuildCommandLexToken<wb_plugin::CommandLexTag::ArgBeginSeparator>();
    }

    // parse ","
    if (c == wb_plugin::COMMAND_ARG_SPLIT_SEPARATOR[0]) {
        return wb_plugin::BuildCommandLexToken<wb_plugin::CommandLexTag::ArgSplitSeparator>();
    }

    // parse numeral literal, only support decimal positive integer
    if (isdigit(c)) {
        std::stringstream ss;

        do {
            ss.put((char)c);
            c = scanner.getch();
        } while (c != PluginCommandScanner::CH_EOF && isdigit(c));

        if (c != PluginCommandScanner::CH_EOF) {
            scanner.ungetch();
        }

        bool success = false;
        auto token   = wb_plugin::BuildCommandLexToken<wb_plugin::CommandLexTag::NumeralLiteral>();

        try {
            token->value = std::stoi(ss.str().c_str());
            success      = true;
        }
        catch (const std::exception& /*e*/) {
        }

        if (!success) {
            return BuildCommandLexIllegalToken("stoi argument out of range");
        }

        return token;
    }

    // parse string literal, escape characters are not supported
    if (c == wb_plugin::COMMAND_STRING_SEPARATOR[0] || c == wb_plugin::COMMAND_STRING_SEPARATOR[1]) {
        std::stringstream ss;

        auto separator = c;

        for (;;) {
            c = scanner.getch();
            if (c == PluginCommandScanner::CH_EOF || c == separator) {
                break;
            }
            ss.put((char)c);
        }

        if (c == separator) {
            auto token   = wb_plugin::BuildCommandLexToken<wb_plugin::CommandLexTag::StringLiteral>();
            token->value = ss.str();
            return token;
        }
        else {
            return BuildCommandLexIllegalToken("string literal must be contained in \"\" or \'\'");
        }
    }

    // parse word
    if (c == '_' || isalpha(c)) {
        std::stringstream ss;

        do {
            ss.put((char)c);
            c = scanner.getch();
        } while (c != PluginCommandScanner::CH_EOF && (c == '_' || isalnum(c)));

        if (c != PluginCommandScanner::CH_EOF) {
            scanner.ungetch();
        }

        // check boolean
        if (!ss.str().compare(wb_plugin::COMMAND_BOOLEAN_TRUE)) {
            auto token   = wb_plugin::BuildCommandLexToken<wb_plugin::CommandLexTag::BooleanLiteral>();
            token->value = true;
            return token;
        }
        else if (!ss.str().compare(wb_plugin::COMMAND_BOOLEAN_FALSE)) {
            auto token   = wb_plugin::BuildCommandLexToken<wb_plugin::CommandLexTag::BooleanLiteral>();
            token->value = false;
            return token;
        }

        // is identifier
        auto token   = wb_plugin::BuildCommandLexToken<wb_plugin::CommandLexTag::Identifier>();
        token->value = ss.str();
        return token;
    }

    return BuildCommandLexIllegalToken("unknown token");
}

//
// Export Methods
//

wxbox::plugin::CommandParseResultPtr wxbox::plugin::ParseCommandStatement(const char* const statement)
{
    if (!statement) {
        return BuildCommandParseFailedResult(CommandParseStatus::NotCommand);
    }

    PluginCommandLexParser parser(statement);
    auto                   token = parser.next_token();

    // check command prefix
    if (token->tag != CommandLexTag::PrefixSeparator) {
        return BuildCommandParseFailedResult(CommandParseStatus::NotCommand);
    }

    //
    // parse method name
    //

    token = parser.next_token();
    if (token->tag != wb_plugin::CommandLexTag::Identifier) {
        if (token->tag == wb_plugin::CommandLexTag::PrefixSeparator) {
            return BuildCommandParseFailedResult(CommandParseStatus::RepeatPrefixSeparator, token);
        }
        return BuildCommandParseFailedResult(CommandParseStatus::MethodNameNotSpecified, token);
    }

    std::vector<std::string> methodInfo;
    methodInfo.push_back(wb_plugin::CastCommandLexToken<wb_plugin::CommandLexTag::Identifier>(token)->value);

    token = parser.next_token();
    if (token->tag == CommandLexTag::DotSeparator) {
        token = parser.next_token();
        if (token->tag != CommandLexTag::Identifier) {
            return BuildCommandParseFailedResult(CommandParseStatus::InvalidModuleRef, token);
        }
        methodInfo.push_back(wb_plugin::CastCommandLexToken<wb_plugin::CommandLexTag::Identifier>(token)->value);
        token = parser.next_token();
    }

    auto commandInfo = BuildCommandExecuteInfo();
    if (methodInfo.size() == 2) {
        // module method
        commandInfo->type       = CommandMethodType::ModuleMethod;
        commandInfo->moduleName = methodInfo[0];
        commandInfo->methodName = methodInfo[1];
    }
    else {
        // global method
        commandInfo->type       = CommandMethodType::GlobalMethod;
        commandInfo->methodName = methodInfo[0];
    }

    //
    // parse parameters
    //

    if (token->tag != CommandLexTag::ArgBeginSeparator) {
        if (token->tag != CommandLexTag::Eof) {
            return BuildCommandParseFailedResult(CommandParseStatus::MissArgBeginSeparator, token);
        }
        return BuildCommandParseResult(commandInfo);
    }

    // collect all remaining tokens
    std::vector<CommandLexTokenPtr> synatxList;
    do {
        token = parser.next_token();
        synatxList.emplace_back(token);

        if (token->tag == CommandLexTag::Illegal) {
            return BuildCommandParseFailedResult(CommandParseStatus::LexIllegal, token);
        }

    } while (token->tag != CommandLexTag::Eof);

    auto p = synatxList.begin();
    if (p == synatxList.end() || (*p)->tag == CommandLexTag::Eof) {
        return BuildCommandParseResult(commandInfo);
    }

    for (;;) {
        // check whether parameter is validate
        if (!CheckCommandLexTokenIsParameter((*p))) {
            return BuildCommandParseFailedResult(CommandParseStatus::IllegalParameterList, token);
        }

        // collect parameter
        CommandParameter arg;
        switch ((*p)->tag) {
            case CommandLexTag::NumeralLiteral:
                arg.type       = CommandParameterType::NumeralLiteral;
                arg.numeralval = CastCommandLexToken<CommandLexTag::NumeralLiteral>((*p))->value;
                break;
            case CommandLexTag::StringLiteral:
                arg.type   = CommandParameterType::StringLiteral;
                arg.strval = CastCommandLexToken<CommandLexTag::StringLiteral>((*p))->value;
                break;
            case CommandLexTag::BooleanLiteral:
                arg.type    = CommandParameterType::BooleanLiteral;
                arg.boolval = CastCommandLexToken<CommandLexTag::BooleanLiteral>((*p))->value;
                break;
        }
        commandInfo->argLists.emplace_back(std::move(arg));

        // check whether the parameter list ends
        if (++p == synatxList.end() || (*p)->tag == CommandLexTag::Eof) {
            break;
        }

        if ((*p)->tag != CommandLexTag::ArgSplitSeparator) {
            return BuildCommandParseFailedResult(CommandParseStatus::IllegalParameterList, token);
        }

        if (++p == synatxList.end() || (*p)->tag == CommandLexTag::Eof) {
            return BuildCommandParseFailedResult(CommandParseStatus::IllegalParameterList, token);
        }
    }

    return BuildCommandParseResult(commandInfo);
}
#ifndef __WXBOX_UTILS_CONFIG_HPP
#define __WXBOX_UTILS_CONFIG_HPP

namespace wxbox {
    namespace util {
        namespace config {

            class SafeYamlNode;

            //
            // SafeYamlNode Iterator
            //

            class SafeYamlNodeIterator final : public YAML::iterator
            {
                using base       = YAML::iterator;
                using value_type = SafeYamlNode;

              public:
                SafeYamlNodeIterator()
                  : base()
                {
                }

                SafeYamlNodeIterator(const base& it)
                  : base(it)
                {
                }

                SafeYamlNodeIterator(const base&& it)
                  : base(std::move(it))
                {
                }

                value_type operator*() const;
            };

            class SafeYamlNodeConstIterator final : public YAML::const_iterator
            {
                using base       = YAML::const_iterator;
                using value_type = const SafeYamlNode;

              public:
                SafeYamlNodeConstIterator()
                  : base()
                {
                }

                SafeYamlNodeConstIterator(const base& it)
                  : base(it)
                {
                }

                SafeYamlNodeConstIterator(const base&& it)
                  : base(std::move(it))
                {
                }

                value_type operator*() const;
            };

            //
            // SafeYamlNode default value traits
            //

            template<typename T>
            struct SafeYamlNodeDefaultValue
            {
                static T default_value() { T; }
            };

            template<typename T>
            struct SafeYamlNodeDefaultValue<T*>
            {
                static T* default_value() { return nullptr; }
            };

            template<>
            struct SafeYamlNodeDefaultValue<std::string>
            {
                static std::string default_value() { return ""; }
            };

#define DefineSafeYamlNodeNumberDefaultValue(T)   \
    template<>                                    \
    struct SafeYamlNodeDefaultValue<T>            \
    {                                             \
        static T default_value() { return T(0); } \
    };

            DefineSafeYamlNodeNumberDefaultValue(int);
            DefineSafeYamlNodeNumberDefaultValue(short);
            DefineSafeYamlNodeNumberDefaultValue(long);
            DefineSafeYamlNodeNumberDefaultValue(long long);
            DefineSafeYamlNodeNumberDefaultValue(unsigned);
            DefineSafeYamlNodeNumberDefaultValue(unsigned short);
            DefineSafeYamlNodeNumberDefaultValue(unsigned long);
            DefineSafeYamlNodeNumberDefaultValue(unsigned long long);

            DefineSafeYamlNodeNumberDefaultValue(char);
            DefineSafeYamlNodeNumberDefaultValue(signed char);
            DefineSafeYamlNodeNumberDefaultValue(unsigned char);

            DefineSafeYamlNodeNumberDefaultValue(float);
            DefineSafeYamlNodeNumberDefaultValue(double);
            DefineSafeYamlNodeNumberDefaultValue(long double);

            //
            // SafeYamlNode
            //

            class SafeYamlNode final : public YAML::Node
            {
                friend class Config;

              public:
                SafeYamlNode()
                  : YAML::Node()
                {
                }

                SafeYamlNode(const YAML::Node& node)
                  : YAML::Node(node)
                {
                }

                SafeYamlNode(YAML::Node&& node)
                  : YAML::Node(std::move(node))
                {
                }

                //
                // Indexing
                //

                template<typename Key>
                const SafeYamlNode operator[](const Key& key) const
                {
                    return YAML::Node::operator[](std::forward<const Key&>(key));
                }

                template<typename Key>
                SafeYamlNode operator[](const Key& key)
                {
                    return YAML::Node::operator[](std::forward<const Key&>(key));
                }

                const SafeYamlNode operator[](const YAML::Node& key) const
                {
                    return YAML::Node::operator[](std::forward<const YAML::Node&>(key));
                }

                SafeYamlNode operator[](const YAML::Node& key)
                {
                    return YAML::Node::operator[](std::forward<const YAML::Node&>(key));
                }

                const SafeYamlNode operator[](const SafeYamlNode& key) const
                {
                    return YAML::Node::operator[](std::forward<const SafeYamlNode&>(key));
                }

                SafeYamlNode operator[](const SafeYamlNode& key)
                {
                    return YAML::Node::operator[](std::forward<const SafeYamlNode&>(key));
                }

                //
                // Assignment
                //

                template<typename T>
                SafeYamlNode& operator=(const T& rhs)
                {
                    YAML::Node::operator=(std::forward<const T&>(rhs));
                    return *this;
                }

                SafeYamlNode& operator=(const Node& rhs)
                {
                    YAML::Node::operator=(std::forward<const Node&>(rhs));
                    return *this;
                }

                SafeYamlNode& operator=(const SafeYamlNode& rhs)
                {
                    YAML::Node::operator=(std::forward<const SafeYamlNode&>(rhs));
                    return *this;
                }

                //
                // Iterator
                //

                inline SafeYamlNodeConstIterator begin() const
                {
                    return IsSequence() ? YAML::Node::begin() : end();
                }

                inline SafeYamlNodeIterator begin()
                {
                    return IsSequence() ? YAML::Node::begin() : end();
                }

                inline SafeYamlNodeConstIterator end() const
                {
                    return YAML::Node::end();
                }

                inline SafeYamlNodeIterator end()
                {
                    return YAML::Node::end();
                }

                //
                // Access
                //

                template<typename T>
                inline T safe_as() const
                {
                    bool noerror = true;
                    T    result  = SafeYamlNodeDefaultValue<T>::default_value();

                    try {
                        if (!IsNull()) {
                            result = YAML::Node::as<T>();
                        }
                    }
                    catch (const std::exception& /*e*/) {
                        noerror = false;
                    }

                    return result;
                }
            };

            //
            // Config
            //

            class Config
            {
              public:
                Config()
                  : dirty(false)
                  , configPath("")
                {
                }

                Config(const std::string configPath)
                  : dirty(true)
                  , configPath(configPath)
                {
                }

                ~Config()
                {
                    close();
                }

                static std::string join_key_path(const std::vector<std::string>& keyPath)
                {
                    return "/" + wxbox::util::string::JoinString(keyPath, "/");
                }

                virtual SafeYamlNode default_config(const std::string& keyPath) const
                {
                    WXBOX_UNREF(keyPath);

                    SafeYamlNode value;

                    //
                    // custom fill default configuration parameters in derived class
                    //

                    // ......

                    return value;
                }

                inline SafeYamlNode get(const std::vector<std::string>& keyPath) const
                {
                    bool                      noeror = true;
                    std::vector<SafeYamlNode> subpath;

                    if (keyPath.empty()) {
                        return SafeYamlNode();
                    }

                    std::lock_guard<std::mutex> lock(mutex);

                    try {
                        for (auto p = keyPath.begin(); p != keyPath.end(); p++) {
                            auto key = *p;

                            if (subpath.empty()) {
                                if (!root[key].IsDefined()) {
                                    root[key] = SafeYamlNode();
                                }
                                subpath.emplace_back(root[key]);
                            }
                            else {
                                subpath.emplace_back(subpath[subpath.size() - 1][key]);
                            }

                            auto& node = subpath[subpath.size() - 1];
                            if (node.IsDefined() && !node.IsNull() && !node.IsMap()) {
                                if (p + 1 != keyPath.end()) {
                                    node = SafeYamlNode();
                                }
                            }
                        }
                    }
                    catch (const std::exception& /*e*/) {
                        noeror = false;
                    }

                    auto fullPath = join_key_path(keyPath);
                    auto result   = (noeror ? subpath[subpath.size() - 1] : SafeYamlNode());
                    if (!result.IsDefined()) {
                        auto value = default_config(fullPath);
                        if (!value.IsNull()) {
                            result = value;
                        }
                        else {
                            result = YAML::Null;
                        }
                    }

                    dirty = true;
                    return result;
                }

                inline const SafeYamlNode operator[](const std::vector<std::string>& keyPath) const
                {
                    return get(keyPath);
                }

                inline SafeYamlNode operator[](const std::vector<std::string>& keyPath)
                {
                    return get(keyPath);
                }

                bool load(const std::string& path = "")
                {
                    std::lock_guard<std::mutex> lock(mutex);

                    if (!path.empty()) {
                        configPath = path;
                    }

                    if (!wxbox::util::file::IsPathExists(configPath)) {
                        return true;
                    }

#if WXBOX_IN_WINDOWS_OS
                    std::ifstream stream(configPath, std::ios::in, _SH_DENYWR);
                    if (!stream.is_open()) {
                        return false;
                    }

#else
                    std::ifstream stream(configPath, std::ios::in);
                    if (!stream.is_open()) {
                        return false;
                    }

                    // it's an advisory lock or a mandatory lock
                    int fd = wb_file::ExposeFileStreamFD(stream.rdbuf());
                    if (fd < 0 || flock(fd, LOCK_EX | LOCK_NB) != 0) {
                        stream.close();
                        return false;
                    }
#endif

                    try {
                        root = YAML::Load(stream);
                    }
                    catch (const YAML::ParserException& /*e*/) {
                        // parse failed
                    }
                    catch (const std::exception& /*e*/) {
                        // maybe never
                    }

#if !WXBOX_IN_WINDOWS_OS
                    flock(fd, LOCK_UN | LOCK_NB);
#endif

                    stream.close();

                    bool retval = root.IsNull() || root.IsMap();
                    if (!retval) {
                        root.reset(YAML::Node());
                    }

                    root.SetStyle(YAML::EmitterStyle::Block);
                    return retval;
                }

                bool submit(const std::string& path = "")
                {
                    bool                        retval = false;
                    std::lock_guard<std::mutex> lock(mutex);

                    if (!path.empty()) {
                        configPath = path;
                    }

                    if (configPath.empty()) {
                        return false;
                    }

#if WXBOX_IN_WINDOWS_OS
                    std::ofstream stream(configPath, std::ios::out | std::ios::trunc, _SH_DENYWR);
                    if (!stream.is_open()) {
                        return retval;
                    }

#else
                    std::ofstream stream(configPath, std::ios::out | std::ios::trunc);
                    if (!stream.is_open()) {
                        return retval;
                    }

                    // it's an advisory lock or a mandatory lock
                    int fd = wb_file::ExposeFileStreamFD(stream.rdbuf());
                    if (fd < 0 || flock(fd, LOCK_EX | LOCK_NB) != 0) {
                        stream.close();
                        return retval;
                    }
#endif

                    try {
                        root.SetStyle(YAML::EmitterStyle::Block);
                        stream << root;
                        retval = true;
                    }
                    catch (const std::exception& /*e*/) {
                    }

#if !WXBOX_IN_WINDOWS_OS
                    flock(fd, LOCK_UN | LOCK_NB);
#endif

                    stream.flush();
                    stream.close();
                    dirty = false;
                    return retval;
                }

                void close()
                {
                    if (dirty) {
                        submit();
                    }

                    std::lock_guard<std::mutex> lock(mutex);
                    root.reset(SafeYamlNode());
                    dirty = false;
                }

              protected:
                mutable std::mutex        mutex;
                mutable std::atomic<bool> dirty;
                mutable SafeYamlNode      root;
                std::string               configPath;
            };
        }
    }
}

template<>
struct YAML::convert<wxbox::util::config::SafeYamlNode>
{
    static wxbox::util::config::SafeYamlNode encode(const wxbox::util::config::SafeYamlNode& rhs)
    {
        return rhs;
    }

    static bool decode(const wxbox::util::config::SafeYamlNode& node, wxbox::util::config::SafeYamlNode& rhs)
    {
        rhs.reset(node);
        return true;
    }
};

inline std::vector<std::string> operator"" _conf(const char* path, std::size_t n)
{
    WXBOX_UNREF(n);

    return wxbox::util::string::SplitString(path, "/");
}

#define REGISTER_CONFIG_KEY(CONFIG_NAME) const auto CONFIG_NAME##_KEY = UNWIND_MACRO_STRING_LITERAL(CONFIG_NAME) ""_conf;

#endif  // #ifndef __WXBOX_UTILS_CONFIG_HPP
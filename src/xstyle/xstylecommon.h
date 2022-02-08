#ifndef __X_STYLE_COMMON_H
#define __X_STYLE_COMMON_H

#define DefineXStyleProperty(Type, PropertyName, MemberName, Suffix, DefaultValue) \
    Type MemberName = DefaultValue;                                                \
                                                                                   \
    inline Type Get##Suffix() const                                                \
    {                                                                              \
        return this->MemberName;                                                   \
    }                                                                              \
                                                                                   \
    inline void Set##Suffix(const Type& value)                                     \
    {                                                                              \
        this->MemberName = value;                                                  \
    }

#endif  // #ifndef __X_STYLE_COMMON_H
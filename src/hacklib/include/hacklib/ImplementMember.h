#ifndef IMPLEMENTMEMBER_H
#define IMPLEMENTMEMBER_H

#include <cstdint>

/*
    === conventional declaration ===

    class CPlayer
    {
    public:
        virtual void unk0();
        virtual void unk1();
        virtual double func(int id, double speed);
        virtual void boop();
        int _00[4];
        int id;
        int _14[2];
        D3DXVECTOR3 pos;
        float hp;
    };

    === would be written as ===

    class CPlayer
    {
        IMPLVTFUNC(double, func, 0x8, int, id, double, speed);
        IMPLVTFUNC_OR(void, boop, 3);
        IMPLMEMBER(int, Id, 0x10);
        IMPLMEMBER_REL(D3DXVECTOR3, Pos, 0x8, Id);
        IMPLMEMBER_REL(float, Hp, 0, Pos);
    };

    === generates wonderful declaration ===

    class CPlayer
    {
    public:
        double func(int id, double speed);
        void boop();
        int getId() const;
        void setId(int value);
        D3DXVECTOR3 getPos() const;
        void setPos(D3DXVECTOR3 value);
        float getHp() const;
        void setHp(float value);
    }
*/


#define IMPLMEMBER(type, name, offset)                                                                                 \
protected:                                                                                                             \
    static const uintptr_t o##name = offset;                                                                           \
    typedef type t##name;                                                                                              \
                                                                                                                       \
public:                                                                                                                \
    type const& get##name() const { return *(type*)((uintptr_t)this + o##name); }                                      \
    type& get##name() { return *(type*)((uintptr_t)this + o##name); }                                                  \
    void set##name(type value) { *(type*)((uintptr_t)this + o##name) = value; }


#define IMPLMEMBER_REL(type, name, offset, from) IMPLMEMBER(type, name, offset + o##from + sizeof(t##from))


#define EXPAND(x) x
#define COUNT_ARGS(...) COUNT_ARGS_(__VA_ARGS__, COUNT_ARGS_RSEQ())
#define COUNT_ARGS_(...) EXPAND(COUNT_ARGS_N(__VA_ARGS__))
#define COUNT_ARGS_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define COUNT_ARGS_RSEQ() 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define ARGS_COMMA ,
#define ARGS_NOCOMMA
#define ARGS_FULL(comma, argtype, argname) argtype argname
#define ARGS_TYPES(comma, argtype, argname) comma argtype
#define ARGS_NAMES(comma, argtype, argname) comma argname

#define EXPAND_ARGS(how, ...) EXPAND_ARGS_(how, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)
#define EXPAND_ARGS_(how, N, ...) EXPAND_ARGS__(how, N, __VA_ARGS__)
#define EXPAND_ARGS__(how, N, ...) EXPAND(EXPAND_ARGS_##N(ARGS_COMMA, how, __VA_ARGS__))
#define EXPAND_ARGS_1(...)
#define EXPAND_ARGS_2(comma, how, argtype, argname, ...) how(comma, argtype, argname)
#define EXPAND_ARGS_4(comma, how, argtype, argname, ...)                                                               \
    how(comma, argtype, argname), EXPAND(EXPAND_ARGS_2(ARGS_NOCOMMA, how, __VA_ARGS__))
#define EXPAND_ARGS_6(comma, how, argtype, argname, ...)                                                               \
    how(comma, argtype, argname), EXPAND(EXPAND_ARGS_4(ARGS_NOCOMMA, how, __VA_ARGS__))
#define EXPAND_ARGS_8(comma, how, argtype, argname, ...)                                                               \
    how(comma, argtype, argname), EXPAND(EXPAND_ARGS_6(ARGS_NOCOMMA, how, __VA_ARGS__))
#define EXPAND_ARGS_10(comma, how, argtype, argname, ...)                                                              \
    how(comma, argtype, argname), EXPAND(EXPAND_ARGS_8(ARGS_NOCOMMA, how, __VA_ARGS__))
#define EXPAND_ARGS_12(comma, how, argtype, argname, ...)                                                              \
    how(comma, argtype, argname), EXPAND(EXPAND_ARGS_10(ARGS_NOCOMMA, how, __VA_ARGS__))
#define EXPAND_ARGS_14(comma, how, argtype, argname, ...)                                                              \
    how(comma, argtype, argname), EXPAND(EXPAND_ARGS_12(ARGS_NOCOMMA, how, __VA_ARGS__))
#define EXPAND_ARGS_16(comma, how, argtype, argname, ...)                                                              \
    how(comma, argtype, argname), EXPAND(EXPAND_ARGS_14(ARGS_NOCOMMA, how, __VA_ARGS__))

#define IMPLVTFUNC_OR(rettype, name, ordinal, ...)                                                                     \
public:                                                                                                                \
    rettype name(EXPAND_ARGS(ARGS_FULL, __VA_ARGS__))                                                                  \
    {                                                                                                                  \
        return ((rettype(__thiscall*)(uintptr_t EXPAND_ARGS(ARGS_TYPES, __VA_ARGS__)))(                                \
            *(uintptr_t*)((*(uintptr_t**)this) + ordinal)))((uintptr_t)this EXPAND_ARGS(ARGS_NAMES, __VA_ARGS__));     \
    }

#define IMPLVTFUNC(rettype, name, offset, ...) IMPLVTFUNC_OR(rettype, name, (offset / sizeof(void*)), __VA_ARGS__)


#endif
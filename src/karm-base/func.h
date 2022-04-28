#pragma once

#include <karm-meta/callable.h>
#include <karm-meta/ptr.h>

#include "_prelude.h"

#include "box.h"

namespace Karm::Base {

template <typename T>
concept CallablePtr = Meta::Ptr<T> and Meta::Callable<Meta::RemovePtr<T>>;

template <typename>
struct Func;

template <typename Out, typename... In>
struct Func<Out(In...)> {
    struct _Wrap {
        virtual ~_Wrap() = default;
        virtual Out operator()(In...) = 0;
    };

    template <typename F>
    struct Wrap : _Wrap {
        F _f;
        Wrap(F f) : _f(f) {}
        Out operator()(In... in) override { return _f(std::forward<In>(in)...); }
    };

    Box<_Wrap> _wrap;

    Func() = delete;

    // clang-format off
    // clang-format has trouble with this

    template <typename F>
    requires (Meta::RvalueRef<F &&> and !(CallablePtr<F>))
    Func(F &&f) : _wrap(make_box<Wrap<F>>(std::forward<F>(f))) {}

    template <typename F>
    requires CallablePtr<F>
    Func(F f) : _wrap(make_box<Wrap<F>>(std::forward<F>(f))) {}

    template <typename F>
    requires (Meta::RvalueRef<F &&> and !(CallablePtr<F>))
    Func &operator=(F &&f)
    {
        _wrap = make_box<Wrap<F>>(std::forward<F>(f));
        return *this;
    }

    template <typename F>
    requires CallablePtr<F>
    Func &operator=(F f)
    {
        _wrap = make_box<Wrap<F>>(std::forward<F>(f));
        return *this;
    }

    // clang-format on

    auto operator()(In... in) -> Out {
        return (*_wrap)(std::forward<In>(in)...);
    }

    template <typename F>
    auto operator=(F f) -> Func & {
        _wrap = make_box(Wrap<F>{std::forward<F>(f)});
        return *this;
    }
};

} // namespace Karm::Base

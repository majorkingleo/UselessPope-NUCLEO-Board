#pragma once
#ifndef WLIB_CALLBACK_HPP_INCLUDED
#define WLIB_CALLBACK_HPP_INCLUDED

namespace wlib
{
  template <typename> class Callback;
  template <typename R, typename... Args> class Callback<R(Args...)>
  {
  public:
    using signature_t   = R(Args...);
    virtual ~Callback() = default;

    virtual R operator()(Args...) = 0;
  };
}    // namespace wlib

namespace wlib
{
  template <typename> class Function_Callback;
  template <typename R, typename... Args> class Function_Callback<R(Args...)> final: public Callback<R(Args...)>
  {
    using fnc_t = R (&)(Args...);

  public:
    constexpr Function_Callback(fnc_t fnc)
        : m_fnc{ fnc }
    {
    }

    R operator()(Args... args) override { return this->m_fnc(args...); }

  private:
    fnc_t m_fnc;
  };
}    // namespace wlib

namespace wlib
{
  template <typename, typename> class Memberfunction_Callback;
  template <typename T, typename R, typename... Args> class Memberfunction_Callback<T, R(Args...)> final: public Callback<R(Args...)>
  {
    using mem_fnc_t       = R (T::*)(Args...);
    using const_mem_fnc_t = R (T::*)(Args...) const;

  public:
    constexpr Memberfunction_Callback(T& obj, mem_fnc_t mem_fuc)
        : m_obj(obj)
        , m_mem_fuc(mem_fuc)
    {
    }

    constexpr Memberfunction_Callback(T& obj, const_mem_fnc_t mem_fuc)
        : m_obj(obj)
        , m_mem_fuc(reinterpret_cast<mem_fnc_t>(mem_fuc))
    {
    }

    R operator()(Args... args) override { return (this->m_obj.*m_mem_fuc)(args...); }

  private:
    T&        m_obj;
    mem_fnc_t m_mem_fuc;
  };

  template <typename T, typename R, typename... Args> class Memberfunction_Callback<const T, R(Args...)> final: public Callback<R(Args...)>
  {
    using const_mem_fnc_t = R (T::*)(Args...) const;

  public:
    constexpr Memberfunction_Callback(T const& obj, const_mem_fnc_t mem_fuc)
        : m_obj(obj)
        , m_mem_fuc(mem_fuc)
    {
    }

    R operator()(Args... args) override { return (this->m_obj.*m_mem_fuc)(args...); }

  private:
    T const&        m_obj;
    const_mem_fnc_t m_mem_fuc;
  };
}    // namespace wlib

#endif

#pragma once

namespace Crawler
{

    template <typename Derived> class Singleton
    {
    public:
        static Derived &GetInstance() noexcept(std::is_nothrow_default_constructible<Derived>::value)
        {
            struct TInstantiable final : Derived
            {
                void TClassNotInstantiable() const noexcept override
                {
                }
            } static instance;
            return instance;
        }

    protected:
        Singleton() = default;
        Singleton(const Singleton &) = delete;
        Singleton(Singleton &&) = delete;
        Singleton &operator=(const Singleton &) = delete;
        Singleton &operator=(Singleton &&) = delete;
        virtual ~Singleton() = default;

    private:
        virtual void TClassNotInstantiable() const noexcept = 0;
    };

}
//
// Created by fred on 29/10/2022.
//

#ifndef TESTDB_SERIALIZER_H
#define TESTDB_SERIALIZER_H

#include <memory>
namespace serializer
{
    class Medium
    {
    public:
        virtual ~Medium()=default;
        [[nodiscard]] virtual bool read(char *ptr, size_t len)=0;
        [[nodiscard]] virtual bool write(const char *ptr, size_t len)=0;
    };

    template<typename... Args>
    struct exists_serialize
    {
        template<typename = decltype(serialize(std::declval<Args>()...))>
        static std::true_type test(int);
        static std::false_type test(...);
        constexpr static const bool value = decltype(test(0))::value;
    };

    template<typename... Args>
    struct exists_deserialize
    {
        template<typename = decltype(serialize(std::declval<Args>()...))>
        static std::true_type test(int);
        static std::false_type test(...);
        constexpr static const bool value = decltype(test(0))::value;
    };



    class Serializer
    {
    public:
        explicit Serializer(std::unique_ptr<Medium> medium)
        : medium(std::move(medium))
        {

        }

        template<typename T>
        Serializer &operator<<(const T &data)
        {
            pack(data);
            return *this;
        }

        template<typename T>
        Serializer &operator>>(T &data)
        {
            extract(data);
            return *this;
        }

        template<typename T>
        void pack(const T &data)
        {
            if constexpr(std::is_trivially_copyable_v<T>)
            {
                (void)medium->write(reinterpret_cast<const char* const>(&data), sizeof(T));
            }
            else if constexpr(exists_serialize<Serializer&, const T&>::value)
            {
                serialize(*this, data);
            }
            else
            {
                static_assert(!std::is_same_v<T, T>, "Unsupported type");
            }
        }

        template<typename T>
        bool extract(T &data)
        {
            if constexpr(std::is_trivially_copyable_v<T>)
            {
                return medium->read(reinterpret_cast<char*>(&data), sizeof(T));
            }
            else if constexpr(exists_deserialize<Serializer&, T&>::value)
            {
                return deserialize(*this, data);
            }
            else
            {
                static_assert(!std::is_same_v<T, T>, "Unsupported type");
            }
            return false;
        }

        bool read(char *ptr, size_t len)
        {
            return medium->read(ptr, len);
        }

        bool write(const char *ptr, size_t len)
        {
            return medium->write(ptr, len);
        }

    private:
        std::unique_ptr<Medium> medium;
    };

}


#endif //TESTDB_SERIALIZER_H

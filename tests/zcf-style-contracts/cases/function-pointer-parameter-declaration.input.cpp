template <typename Result, typename Receiver, typename... Args>
constexpr auto
InvokeMember(
    Result (Receiver::*operation)(Args...) const noexcept
    ) -> Result;

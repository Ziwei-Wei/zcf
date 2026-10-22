void
Consume(std::vector<int>& values, std::size_t count);

template <typename Value, // type parameter
          std::size_t Extent // non-type parameter
          >
struct Buffer;

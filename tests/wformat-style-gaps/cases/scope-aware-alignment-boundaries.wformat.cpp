using Result = long;
inline constexpr Result Success      = 0;
inline constexpr Result PointerError = -1;

void
update(
    int& short_name,
    int& much_longer_name,
    int delta
    )
{
    short_name        = delta;
    short_name       += delta;
    much_longer_name -= delta;
    bool ready     = true;
    bool available = false;
}